set(third_party_folder "ThirdParty")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(glew/build/cmake)
add_subdirectory(glfw)
add_subdirectory(sdl3)
add_subdirectory(sail)

include(imgui.cmake)

set_target_properties(glew PROPERTIES FOLDER ${third_party_folder})
set_target_properties(glfw PROPERTIES FOLDER ${third_party_folder})
# set_target_properties(SDL3-static PROPERTIES FOLDER ${third_party_folder})
set_target_properties(imgui PROPERTIES FOLDER ${third_party_folder})
set_target_properties(sail PROPERTIES FOLDER ${third_party_folder})
