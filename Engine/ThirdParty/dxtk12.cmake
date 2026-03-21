message(STATUS "====== Delta Engine ====== Configuring DirectXTK12 ======")

set(BUILD_SHARED_LIBS ON)
set(BUILD_TESTING OFF)

add_subdirectory(DirectXTK12 ${CMAKE_BINARY_DIR}/bin/CMake/DirectXTK12)

set_target_properties(DirectXTK12 PROPERTIES FOLDER ${third_party_folder})

add_library(ThirdParty::DirectXTK12 ALIAS DirectXTK12)
