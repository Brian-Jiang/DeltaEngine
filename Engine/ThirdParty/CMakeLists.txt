set(third_party_folder "ThirdParty")
set(utility_folder "Utility")
set(DELTA_THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

include(sdl3.cmake)
include(imgui.cmake)
# include(sail.cmake)
include(dxtk12.cmake)
include(assimp.cmake)

add_custom_target(CopyDxcBin ALL
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   "${CMAKE_CURRENT_SOURCE_DIR}/dxc/bin/x64"
                   "${CMAKE_BINARY_DIR}/bin"
                   COMMENT "Copying DXC binaries from ${CMAKE_CURRENT_SOURCE_DIR}/dxc/bin/x86 to ${CMAKE_BINARY_DIR}/bin"
)

set_target_properties(CopyDxcBin PROPERTIES FOLDER ${utility_folder})

# set_target_properties(glfw PROPERTIES FOLDER ${third_party_folder})
# set_target_properties(SDL3-static PROPERTIES FOLDER ${third_party_folder})


