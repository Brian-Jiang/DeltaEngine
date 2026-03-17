message(STATUS "====== Delta Engine ====== Configuring DirectXTK12 ======")

set(BUILD_SHARED_LIBS ON)
set(BUILD_TESTING OFF)

add_subdirectory(DirectXTK12 ${CMAKE_BINARY_DIR}/bin/CMake/DirectXTK12)

set_target_properties(DirectXTK12 PROPERTIES FOLDER ${third_party_folder})

set(THIRD_PARTY_INCLUDES ${THIRD_PARTY_INCLUDES} "${CMAKE_CURRENT_SOURCE_DIR}/DirectXTK12/Inc" CACHE INTERNAL "Third party include directories")
