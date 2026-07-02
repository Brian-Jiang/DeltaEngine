set(third_party_folder "ThirdParty")
set(utility_folder "Utility")
set(DELTA_THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

include("${CMAKE_SOURCE_DIR}/Tools/CMake/SyncDirectory.cmake")

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

delta_add_sync_directory_target(CopyDxcBin
    "${CMAKE_CURRENT_SOURCE_DIR}/dxc/bin/x64"
    "${CMAKE_BINARY_DIR}/bin")
