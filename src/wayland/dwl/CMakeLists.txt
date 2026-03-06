qt_add_library(quickshell-dwl STATIC
	output.cpp
	tag.cpp
	manager.cpp
	proto.cpp
	qml.cpp
)

qt_add_qml_module(quickshell-dwl
	URI Quickshell.DWL
	VERSION 0.1
)

install_qml_module(quickshell-dwl)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets WaylandClient)

target_link_libraries(quickshell-dwl PRIVATE
	Qt6::Core
	Qt6::Gui
	Qt6::GuiPrivate
	Qt6::Widgets
	Qt6::WaylandClient
	Qt6::WaylandClientPrivate
	Wayland::Client
)

target_link_libraries(quickshell PRIVATE
	quickshell-dwl
)
