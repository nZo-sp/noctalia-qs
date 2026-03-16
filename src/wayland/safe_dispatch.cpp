// Workaround for a Qt6 Wayland client bug where wl_output proxy destruction
// during event dispatch causes a NULL dereference crash.
//
// When a monitor is disconnected, Qt synchronously destroys the wl_output
// proxy during registry.global_remove processing.  Pending events in the
// same or subsequent read batch that reference the destroyed output get
// deserialized with a NULL proxy argument.  Qt's wl_output::fromObject()
// then calls wl_proxy_get_listener(NULL) → SIGSEGV.
//
// Fix: override wl_proxy_get_listener with a NULL-safe version.

#include "safe_dispatch.hpp"

#include <dlfcn.h>

#include <qlogging.h>
#include <qloggingcategory.h>
#include <wayland-client-core.h>

namespace qs::wayland {

namespace {
Q_LOGGING_CATEGORY(logSafeDispatch, "quickshell.wayland.safedispatch", QtWarningMsg);
}

void installWaylandSafeDispatch() {
	qCInfo(logSafeDispatch) << "Wayland safe dispatch active: wl_proxy_get_listener"
	                        << "NULL guard enabled to prevent monitor hotplug crashes.";
}

} // namespace qs::wayland

namespace {

using ProxyGetListenerFn = const void* (*)(wl_proxy*);

ProxyGetListenerFn origProxyGetListener() {
	static auto fn = reinterpret_cast<ProxyGetListenerFn>( // NOLINT
	    dlsym(RTLD_NEXT, "wl_proxy_get_listener")
	);
	return fn;
}

} // namespace

extern "C" {

const void* wl_proxy_get_listener(wl_proxy* proxy) {
	if (!proxy) [[unlikely]] {
		return nullptr;
	}
	return origProxyGetListener()(proxy);
}

} // extern "C"
