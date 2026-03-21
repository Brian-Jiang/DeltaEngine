message(STATUS "====== Delta Engine ====== Configuring imgui ======")

set(imgui_SOURCE_DIR_ ${CMAKE_CURRENT_SOURCE_DIR}/imgui)

file(GLOB imgui_sources CONFIGURE_DEPENDS
	"${imgui_SOURCE_DIR_}/imgui.cpp"
	"${imgui_SOURCE_DIR_}/imgui_demo.cpp"
	"${imgui_SOURCE_DIR_}/imgui_draw.cpp"
	"${imgui_SOURCE_DIR_}/imgui_tables.cpp"
	"${imgui_SOURCE_DIR_}/imgui_widgets.cpp"
)

file(GLOB imgui_impl CONFIGURE_DEPENDS
	"${imgui_SOURCE_DIR_}/backends/imgui_impl_dx12.cpp"
	"${imgui_SOURCE_DIR_}/backends/imgui_impl_sdl3.cpp"
)

add_library(imgui STATIC ${imgui_sources} ${imgui_impl})
target_include_directories(imgui SYSTEM PUBLIC
    $<BUILD_INTERFACE:${imgui_SOURCE_DIR_}>
    $<BUILD_INTERFACE:${imgui_SOURCE_DIR_}/backends>)
target_link_libraries(imgui PRIVATE SDL3::SDL3)

add_library(ThirdParty::imgui ALIAS imgui)

set_target_properties(imgui PROPERTIES FOLDER ${third_party_folder})