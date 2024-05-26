set(third_party_folder "ThirdParty")

include(glew.cmake)
add_subdirectory(glfw)
include(sdl3.cmake)
include(imgui.cmake)
# include(sail.cmake)
include(dxtk12.cmake)

set_target_properties(glfw PROPERTIES FOLDER ${third_party_folder})
# set_target_properties(SDL3-static PROPERTIES FOLDER ${third_party_folder})


