set(DSDL_SHARED OFF)
set(DSDL_STATIC ON)
set(DSDL_TEST_LIBRARY OFF)
set(DSDL_TESTS OFF)
set(DSDL_DISABLE_INSTALL ON)
set(DSDL_DISABLE_INSTALL_DOCS ON)
set(DSDL_INSTALL_TESTS ON)

add_subdirectory(sdl3)

set_target_properties(SDL3-static PROPERTIES FOLDER ${third_party_folder})