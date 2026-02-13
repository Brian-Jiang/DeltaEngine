message(STATUS "====== Delta Engine ====== Configuring DirectXTex ======")

set(BUILD_SHARED_LIBS ON)
set(BUILD_TOOLS OFF)
set(BUILD_SAMPLE OFF)
set(BUILD_DX11 OFF)

add_subdirectory(DirectXTex ${CMAKE_BINARY_DIR}/bin/CMake/DirectXTex)

set_target_properties(DirectXTex PROPERTIES FOLDER ${third_party_folder})