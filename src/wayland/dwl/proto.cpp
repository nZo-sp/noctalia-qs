#include "proto.hpp"
#include <array>

#include <wayland-client-protocol.h>
#include <wayland-util.h>

namespace qs::dwl {
namespace {

const std::array<struct wl_message, 6> ZDWL_IPC_OUTPUT_V2_REQUESTS = {{
    {.name = "release", .signature = "", .types = nullptr},
    {.name = "set_tags", .signature = "uu", .types = nullptr},
    {.name = "set_client_tags", .signature = "uu", .types = nullptr},
    {.name = "set_layout", .signature = "u", .types = nullptr},
    {.name = "quit", .signature = "2", .types = nullptr},
    {.name = "dispatch", .signature = "2sssss", .types = nullptr},
}};

const std::array<struct wl_message, 18> ZDWL_IPC_OUTPUT_V2_EVENTS = {{
    {.name = "toggle_visibility", .signature = "", .types = nullptr},
    {.name = "active", .signature = "u", .types = nullptr},
    {.name = "tag", .signature = "uuuu", .types = nullptr},
    {.name = "layout", .signature = "u", .types = nullptr},
    {.name = "title", .signature = "s", .types = nullptr},
    {.name = "appid", .signature = "s", .types = nullptr},
    {.name = "layout_symbol", .signature = "s", .types = nullptr},
    {.name = "frame", .signature = "", .types = nullptr},
    {.name = "fullscreen", .signature = "2u", .types = nullptr},
    {.name = "floating", .signature = "2u", .types = nullptr},
    {.name = "x", .signature = "2i", .types = nullptr},
    {.name = "y", .signature = "2i", .types = nullptr},
    {.name = "width", .signature = "2i", .types = nullptr},
    {.name = "height", .signature = "2i", .types = nullptr},
    {.name = "last_layer", .signature = "2s", .types = nullptr},
    {.name = "kb_layout", .signature = "2s", .types = nullptr},
    {.name = "keymode", .signature = "2s", .types = nullptr},
    {.name = "scalefactor", .signature = "2u", .types = nullptr},
}};

} // anonymous namespace

const struct wl_interface ZDWL_IPC_OUTPUT_V2_INTERFACE = {
    .name = "zdwl_ipc_output_v2",
    .version = 2,
    .method_count = 6,
    .methods = ZDWL_IPC_OUTPUT_V2_REQUESTS.data(),
    .event_count = 18,
    .events = ZDWL_IPC_OUTPUT_V2_EVENTS.data(),
};

namespace {

const std::array<const struct wl_interface*, 3> DWL_TYPES = {
    nullptr,
    &ZDWL_IPC_OUTPUT_V2_INTERFACE,
    &wl_output_interface,
};

const std::array<struct wl_message, 2> ZDWL_IPC_MANAGER_V2_REQUESTS = {{
    {.name = "release", .signature = "", .types = nullptr},
    {.name = "get_output",
     .signature = "no",
     .types = const_cast<const wl_interface**>(DWL_TYPES.data())},
}};

const std::array<struct wl_message, 2> ZDWL_IPC_MANAGER_V2_EVENTS = {{
    {.name = "tags", .signature = "u", .types = nullptr},
    {.name = "layout", .signature = "s", .types = nullptr},
}};

} // anonymous namespace

const struct wl_interface ZDWL_IPC_MANAGER_V2_INTERFACE = {
    .name = "zdwl_ipc_manager_v2",
    .version = 2,
    .method_count = 2,
    .methods = ZDWL_IPC_MANAGER_V2_REQUESTS.data(),
    .event_count = 2,
    .events = ZDWL_IPC_MANAGER_V2_EVENTS.data(),
};

} // namespace qs::dwl
