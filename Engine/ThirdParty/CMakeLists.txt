set(third_party_folder "ThirdParty")
set(DELTA_THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

# include(glew.cmake)
add_subdirectory(glfw)
include(sdl3.cmake)
include(imgui.cmake)
# include(sail.cmake)
include(dxtk12.cmake)
include(assimp.cmake) 

set_target_properties(glfw PROPERTIES FOLDER ${third_party_folder})
# set_target_properties(SDL3-static PROPERTIES FOLDER ${third_party_folder})


