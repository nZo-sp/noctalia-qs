#pragma once

#include <wayland-client-core.h>
#include <wayland-client-protocol.h>

namespace qs::dwl {

struct zdwl_ipc_manager_v2;
struct zdwl_ipc_output_v2;

extern const struct wl_interface ZDWL_IPC_MANAGER_V2_INTERFACE;
extern const struct wl_interface ZDWL_IPC_OUTPUT_V2_INTERFACE;

// zdwl_ipc_manager_v2 request opcodes
static constexpr uint32_t ZDWL_IPC_MANAGER_V2_RELEASE = 0;
static constexpr uint32_t ZDWL_IPC_MANAGER_V2_GET_OUTPUT = 1;

// zdwl_ipc_output_v2 request opcodes
static constexpr uint32_t ZDWL_IPC_OUTPUT_V2_RELEASE = 0;
static constexpr uint32_t ZDWL_IPC_OUTPUT_V2_SET_TAGS = 1;
static constexpr uint32_t ZDWL_IPC_OUTPUT_V2_SET_CLIENT_TAGS = 2;
static constexpr uint32_t ZDWL_IPC_OUTPUT_V2_SET_LAYOUT = 3;

// zdwl_ipc_output_v2 tag_state enum
static constexpr uint32_t ZDWL_IPC_OUTPUT_V2_TAG_STATE_NONE = 0;
static constexpr uint32_t ZDWL_IPC_OUTPUT_V2_TAG_STATE_ACTIVE = 1;
static constexpr uint32_t ZDWL_IPC_OUTPUT_V2_TAG_STATE_URGENT = 2;

// Listener structs (wayland-scanner mirror)

struct ZdwlIpcManagerV2Listener {
	void (*tags)(void* data, struct zdwl_ipc_manager_v2*, uint32_t amount);
	void (*layout)(void* data, struct zdwl_ipc_manager_v2*, const char* name);
};

struct ZdwlIpcOutputV2Listener {
	void (*toggleVisibility)(void* data, struct zdwl_ipc_output_v2*);
	void (*active)(void* data, struct zdwl_ipc_output_v2*, uint32_t active);
	void (*tag)(
	    void* data,
	    struct zdwl_ipc_output_v2*,
	    uint32_t tag,
	    uint32_t state,
	    uint32_t clients,
	    uint32_t focused
	);

	void (*layout)(void* data, struct zdwl_ipc_output_v2*, uint32_t layout);
	void (*title)(void* data, struct zdwl_ipc_output_v2*, const char* title);
	void (*appid)(void* data, struct zdwl_ipc_output_v2*, const char* appid);
	void (*layoutSymbol)(void* data, struct zdwl_ipc_output_v2*, const char* layout);
	void (*frame)(void* data, struct zdwl_ipc_output_v2*);
	void (*fullscreen)(void* data, struct zdwl_ipc_output_v2*, uint32_t isFullscreen);
	void (*floating)(void* data, struct zdwl_ipc_output_v2*, uint32_t isFloating);
	void (*x)(void* data, struct zdwl_ipc_output_v2*, int32_t x);
	void (*y)(void* data, struct zdwl_ipc_output_v2*, int32_t y);
	void (*width)(void* data, struct zdwl_ipc_output_v2*, int32_t width);
	void (*height)(void* data, struct zdwl_ipc_output_v2*, int32_t height);
	void (*lastLayer)(void* data, struct zdwl_ipc_output_v2*, const char* lastLayer);
	void (*kbLayout)(void* data, struct zdwl_ipc_output_v2*, const char* kbLayout);
	void (*keymode)(void* data, struct zdwl_ipc_output_v2*, const char* keymode);
	void (*scalefactor)(void* data, struct zdwl_ipc_output_v2*, uint32_t scalefactor);
};

inline int zdwlIpcManagerV2AddListener(
    struct zdwl_ipc_manager_v2* manager,
    struct ZdwlIpcManagerV2Listener* listener,
    void* data
) {
	return wl_proxy_add_listener(
	    reinterpret_cast<struct wl_proxy*>(manager),
	    reinterpret_cast<void (**)(void)>(listener),
	    data
	);
}

inline void zdwlIpcManagerV2Release(struct zdwl_ipc_manager_v2* manager) {
	wl_proxy_marshal_flags(
	    reinterpret_cast<struct wl_proxy*>(manager),
	    ZDWL_IPC_MANAGER_V2_RELEASE,
	    nullptr,
	    wl_proxy_get_version(reinterpret_cast<struct wl_proxy*>(manager)),
	    WL_MARSHAL_FLAG_DESTROY
	);
}

inline struct zdwl_ipc_output_v2*
zdwlIpcManagerV2GetOutput(struct zdwl_ipc_manager_v2* manager, struct wl_output* output) {
	return reinterpret_cast<struct zdwl_ipc_output_v2*>(wl_proxy_marshal_flags(
	    reinterpret_cast<struct wl_proxy*>(manager),
	    ZDWL_IPC_MANAGER_V2_GET_OUTPUT,
	    &ZDWL_IPC_OUTPUT_V2_INTERFACE,
	    wl_proxy_get_version(reinterpret_cast<struct wl_proxy*>(manager)),
	    0,
	    nullptr, // new_id placeholder
	    output
	));
}

inline int zdwlIpcOutputV2AddListener(
    struct zdwl_ipc_output_v2* output,
    struct ZdwlIpcOutputV2Listener* listener,
    void* data
) {
	return wl_proxy_add_listener(
	    reinterpret_cast<struct wl_proxy*>(output),
	    reinterpret_cast<void (**)(void)>(listener),
	    data
	);
}

inline void zdwlIpcOutputV2Release(struct zdwl_ipc_output_v2* output) {
	wl_proxy_marshal_flags(
	    reinterpret_cast<struct wl_proxy*>(output),
	    ZDWL_IPC_OUTPUT_V2_RELEASE,
	    nullptr,
	    wl_proxy_get_version(reinterpret_cast<struct wl_proxy*>(output)),
	    WL_MARSHAL_FLAG_DESTROY
	);
}

inline void
zdwlIpcOutputV2SetTags(struct zdwl_ipc_output_v2* output, uint32_t tagmask, uint32_t toggleTagset) {
	wl_proxy_marshal_flags(
	    reinterpret_cast<struct wl_proxy*>(output),
	    ZDWL_IPC_OUTPUT_V2_SET_TAGS,
	    nullptr,
	    wl_proxy_get_version(reinterpret_cast<struct wl_proxy*>(output)),
	    0,
	    tagmask,
	    toggleTagset
	);
}

inline void zdwlIpcOutputV2SetClientTags(
    struct zdwl_ipc_output_v2* output,
    uint32_t andTags,
    uint32_t xorTags
) {
	wl_proxy_marshal_flags(
	    reinterpret_cast<struct wl_proxy*>(output),
	    ZDWL_IPC_OUTPUT_V2_SET_CLIENT_TAGS,
	    nullptr,
	    wl_proxy_get_version(reinterpret_cast<struct wl_proxy*>(output)),
	    0,
	    andTags,
	    xorTags
	);
}

inline void zdwlIpcOutputV2SetLayout(struct zdwl_ipc_output_v2* output, uint32_t index) {
	wl_proxy_marshal_flags(
	    reinterpret_cast<struct wl_proxy*>(output),
	    ZDWL_IPC_OUTPUT_V2_SET_LAYOUT,
	    nullptr,
	    wl_proxy_get_version(reinterpret_cast<struct wl_proxy*>(output)),
	    0,
	    index
	);
}

} // namespace qs::dwl
