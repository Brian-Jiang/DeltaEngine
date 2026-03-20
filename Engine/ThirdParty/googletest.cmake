message(STATUS "====== Delta Engine ====== Configuring Google Test ======")

set(INSTALL_GTEST OFF)

add_subdirectory(googletest)

set_target_properties(gtest gtest_main gmock gmock_main PROPERTIES FOLDER ${third_party_folder})

set(THIRD_PARTY_INCLUDES ${THIRD_PARTY_INCLUDES} "${CMAKE_CURRENT_SOURCE_DIR}/googletest/googletest/include" CACHE INTERNAL "Third party include directories")
set(THIRD_PARTY_INCLUDES ${THIRD_PARTY_INCLUDES} "${CMAKE_CURRENT_SOURCE_DIR}/googletest/googlemock/include" CACHE INTERNAL "Third party include directories")
