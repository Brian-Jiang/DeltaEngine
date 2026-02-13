set(BUILD_SHARED_LIBS ON)

add_subdirectory(DirectXTK12 ${CMAKE_BINARY_DIR}/bin/CMake/DirectXTK12)

set_target_properties(DirectXTK12 PROPERTIES FOLDER ${third_party_folder})
