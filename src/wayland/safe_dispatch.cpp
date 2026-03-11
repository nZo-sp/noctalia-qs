// Workaround for a Qt6 Wayland client bug where wl_output proxy destruction
// during event dispatch causes a use-after-free crash.
//
// When a monitor is disconnected, the compositor sends registry.global_remove
// followed by (or interleaved with) wl_surface/wl_output events in the same
// event batch.  Qt's registry_global_remove handler deletes QWaylandScreen,
// which calls wl_output_release → wl_proxy_marshal_flags(…, DESTROY) →
// wl_proxy_marshal_array_flags(…, DESTROY) → wl_proxy_destroy.
// A later wl_surface.enter(wl_output) event references the destroyed proxy
// → SIGSEGV (NULL dereference when the proxy is missing from the object map).
//
// On distros that build libwayland with LTO (e.g. Fedora), the internal
// wl_proxy_marshal_array_flags → wl_proxy_destroy call is a direct call
// (not through PLT), so overriding wl_proxy_destroy alone is insufficient.
// However, wl_proxy_marshal_flags → wl_proxy_marshal_array_flags DOES go
// through PLT (confirmed by disassembly), so we intercept at that level.
//
// Additional complication: this application loads both Qt and GTK (via GDK).
// Both toolkits bind their own wl_output proxy.  GDK also destroys its proxy
// on global_remove, which can remove it from the object map before libwayland
// deserializes later events that reference the same object ID.
//
// Fix strategy (defense in depth):
//
// 1. Override wl_proxy_marshal_array_flags: for wl_output proxies with
//    WL_MARSHAL_FLAG_DESTROY, strip the flag (the release request is still
//    sent) and replace the listener with a no-op stub.  The proxy stays alive
//    in libwayland's object map permanently (~100 bytes leak per disconnect).
//
// 2. Override wl_proxy_destroy: same treatment for direct destroy calls
//    (non-LTO builds, or direct wl_proxy_destroy callers like GDK).
//
// 3. Override wl_proxy_get_listener: NULL-safe guard.  If any wl_output proxy
//    still ends up destroyed (e.g. through a path we don't intercept), the
//    NULL argument from event deserialization is handled gracefully instead
//    of crashing.

#include "safe_dispatch.hpp"

#include <cstdint>
#include <dlfcn.h>

#include <qlogging.h>
#include <qloggingcategory.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>

namespace qs::wayland {

namespace {
Q_LOGGING_CATEGORY(logSafeDispatch, "quickshell.wayland.safedispatch", QtWarningMsg);
}

void installWaylandSafeDispatch() {
	qCInfo(logSafeDispatch) << "Wayland safe dispatch active: wl_output proxy destruction"
	                        << "is permanently suppressed to prevent use-after-free"
	                        << "crashes caused by monitor hotplug.";
}

} // namespace qs::wayland

namespace {

// Matches the start of libwayland's internal wl_proxy struct (via embedded
// wl_object).  Layout has been stable across all libwayland 1.x releases.
struct WlProxyHeader {
	const void* interface;      // wl_object.interface  (offset 0)
	const void* implementation; // wl_object.implementation  (offset 8 on 64-bit)
};

// No-op event handler that absorbs pending events for a proxy whose real
// listener has been detached.  On x86_64 System V ABI, extra register
// arguments are safely ignored.
extern "C" void qs_wayland_stub_event() {}

// wl_output has 6 events; extra entries for forward-compatibility.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays)
void (*const sStubOutputImpl[])() = {
    qs_wayland_stub_event, qs_wayland_stub_event, qs_wayland_stub_event, qs_wayland_stub_event,
    qs_wayland_stub_event, qs_wayland_stub_event, qs_wayland_stub_event, qs_wayland_stub_event,
};

// --- dlsym helpers ---

template <typename T>
T resolveOriginal(const char* name) {
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	auto* fn = reinterpret_cast<T>(dlsym(RTLD_NEXT, name));
	if (!fn) qFatal("safe_dispatch: dlsym(%s) failed", name);
	return fn;
}

using ProxyDestroyFn = void (*)(wl_proxy*);
using ProxyGetListenerFn = const void* (*)(wl_proxy*);
using MarshalArrayFlagsFn =
    wl_proxy* (*)(wl_proxy*, uint32_t, const wl_interface*, uint32_t, uint32_t, wl_argument*);

ProxyDestroyFn origProxyDestroy() {
	static auto fn = resolveOriginal<ProxyDestroyFn>("wl_proxy_destroy");
	return fn;
}

ProxyGetListenerFn origProxyGetListener() {
	static auto fn = resolveOriginal<ProxyGetListenerFn>("wl_proxy_get_listener");
	return fn;
}

MarshalArrayFlagsFn origMarshalArrayFlags() {
	static auto fn = resolveOriginal<MarshalArrayFlagsFn>("wl_proxy_marshal_array_flags");
	return fn;
}

// --- helpers ---

bool isOutputProxy(wl_proxy* proxy) {
	auto* header = reinterpret_cast<WlProxyHeader*>(proxy); // NOLINT
	return header->interface == &wl_output_interface;
}

void stubOutputProxy(wl_proxy* proxy) {
	auto* header = reinterpret_cast<WlProxyHeader*>(proxy); // NOLINT
	header->implementation = reinterpret_cast<const void*>(sStubOutputImpl);

	qCDebug(qs::wayland::logSafeDispatch)
	    << "Stubbed wl_output proxy" << proxy << "- destruction suppressed";
}

} // namespace

extern "C" {

// --- wl_proxy_get_listener override ---
// Defense-in-depth: if a wl_output proxy is destroyed through a path we
// don't intercept, the event deserialization sets the argument to NULL.
// This override prevents the NULL dereference crash.

const void* wl_proxy_get_listener(wl_proxy* proxy) {
	if (!proxy) [[unlikely]] {
		return nullptr;
	}
	return origProxyGetListener()(proxy);
}

// --- Proxy destruction override ---
// Catches direct wl_proxy_destroy calls (from GDK, non-LTO libwayland, etc).
// wl_output proxies are NEVER destroyed — just stubbed.

void wl_proxy_destroy(wl_proxy* proxy) {
	if (isOutputProxy(proxy)) {
		stubOutputProxy(proxy);
		return;
	}
	origProxyDestroy()(proxy);
}

// --- wl_proxy_marshal_array_flags override ---
// This is the key fix for LTO-built libwayland (e.g. Fedora).
//
// Call chain: Qt's inline wl_output_release() →
//   wl_proxy_marshal_flags() [crosses DSO, always PLT] →
//     wl_proxy_marshal_array_flags() [PLT on Fedora, confirmed by disasm] →
//       inlined wl_proxy_destroy [LTO: direct, bypasses our override]
//
// By intercepting wl_proxy_marshal_array_flags (non-variadic, clean ABI),
// we strip WL_MARSHAL_FLAG_DESTROY for wl_output so the release request
// is sent but the proxy is kept alive with a stub listener permanently.

struct wl_proxy* wl_proxy_marshal_array_flags(
    wl_proxy* proxy,
    uint32_t opcode,
    const wl_interface* interface,
    uint32_t version,
    uint32_t flags,
    wl_argument* args
) {
	if ((flags & WL_MARSHAL_FLAG_DESTROY) && isOutputProxy(proxy)) {
		// Send the release request without destroying the proxy.
		auto* result = origMarshalArrayFlags()(
		    proxy, opcode, interface, version,
		    flags & ~static_cast<uint32_t>(WL_MARSHAL_FLAG_DESTROY), args
		);
		stubOutputProxy(proxy);
		return result;
	}
	return origMarshalArrayFlags()(proxy, opcode, interface, version, flags, args);
}

} // extern "C"
