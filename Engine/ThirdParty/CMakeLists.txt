set(third_party_folder "ThirdParty")
set(utility_folder "Utility")
set(DELTA_THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

include(sdl3.cmake)
include(imgui.cmake)
include(imguizmo.cmake)
include(dx12agility.cmake)
include(dxheaders.cmake)
include(dxtk12.cmake)
include(dxtex.cmake)
include(assimp.cmake)
include(nlohmann_json.cmake)
include(spdlog.cmake)
include(asio.cmake)
include(googletest.cmake)
include(slang.cmake)
include(pix.cmake)

add_custom_target(CopyDxcBin ALL
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   "${CMAKE_CURRENT_SOURCE_DIR}/dxc/bin/x64"
                   "${CMAKE_BINARY_DIR}/bin"
                   COMMENT "Copying DXC binaries from ${CMAKE_CURRENT_SOURCE_DIR}/dxc/bin/x86 to ${CMAKE_BINARY_DIR}/bin"
)

set_target_properties(CopyDxcBin PROPERTIES FOLDER ${utility_folder})
