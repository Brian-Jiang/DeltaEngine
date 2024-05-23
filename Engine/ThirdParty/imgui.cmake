set(imgui_SOURCE_DIR_ ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
# message(STATUS "imgui_SOURCE_DIR_: ${imgui_SOURCE_DIR_}")

file(GLOB imgui_sources CONFIGURE_DEPENDS  "${imgui_SOURCE_DIR_}/*.cpp")
file(GLOB imgui_impl CONFIGURE_DEPENDS  
"${imgui_SOURCE_DIR_}/backends/imgui_impl_glfw.cpp" 
"${imgui_SOURCE_DIR_}/backends/imgui_impl_glfw.h"
#"${imgui_SOURCE_DIR_}/backends/imgui_impl_vulkan.cpp" 
#"${imgui_SOURCE_DIR_}/backends/imgui_impl_vulkan.h"
"${imgui_SOURCE_DIR_}/backends/imgui_impl_dx12.cpp"
"${imgui_SOURCE_DIR_}/backends/imgui_impl_dx12.h"
"${imgui_SOURCE_DIR_}/backends/imgui_impl_sdl3.cpp"
"${imgui_SOURCE_DIR_}/backends/imgui_impl_sdl3.h"
)

add_library(imgui STATIC ${imgui_sources} ${imgui_impl})
target_include_directories(imgui PUBLIC $<BUILD_INTERFACE:${imgui_SOURCE_DIR_}>)
# target_include_directories(imgui PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/SDL-prerelease-3.1.1>)
#target_include_directories(imgui PUBLIC $<BUILD_INTERFACE:${vulkan_include}>)
target_link_libraries(imgui PUBLIC glfw SDL3::SDL3)

set_target_properties(imgui PROPERTIES FOLDER ${third_party_folder})