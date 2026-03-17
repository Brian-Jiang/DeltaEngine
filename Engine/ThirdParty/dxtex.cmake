message(STATUS "====== Delta Engine ====== Configuring DirectXTex ======")

set(BUILD_SHARED_LIBS ON)
set(BUILD_TOOLS OFF)
set(BUILD_SAMPLE OFF)
set(BUILD_DX11 OFF)
set(BUILD_TESTING OFF)

add_subdirectory(DirectXTex ${CMAKE_BINARY_DIR}/bin/CMake/DirectXTex)

set_target_properties(DirectXTex PROPERTIES FOLDER ${third_party_folder})

set(THIRD_PARTY_INCLUDES ${THIRD_PARTY_INCLUDES} "${CMAKE_CURRENT_SOURCE_DIR}/DirectXTex/DirectXTex" CACHE INTERNAL "Third party include directories")
