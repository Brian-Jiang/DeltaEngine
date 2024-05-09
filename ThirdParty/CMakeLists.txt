set(third_party_folder "ThirdParty")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(glew/build/cmake)
add_subdirectory(glfw)
add_subdirectory(sdl3)

include(imgui.cmake)
set_target_properties(imgui PROPERTIES FOLDER ${third_party_folder}/imgui)
