message(STATUS "====== Delta Engine ====== Configuring SDL3 ======")

set(BUILD_SHARED_LIBS ON)
set(SDL_SHARED ON)
set(SDL_STATIC OFF)
set(SDL_TEST_LIBRARY OFF)
set(SDL_TESTS OFF)
set(SDL_DISABLE_INSTALL ON)
set(SDL_DISABLE_INSTALL_DOCS ON)
set(SDL_INSTALL_TESTS OFF)

add_subdirectory(SDL3 EXCLUDE_FROM_ALL)

set_target_properties(SDL3-shared PROPERTIES FOLDER ${third_party_folder})

set(THIRD_PARTY_INCLUDES ${THIRD_PARTY_INCLUDES} "${CMAKE_CURRENT_SOURCE_DIR}/SDL3/include" CACHE INTERNAL "Third party include directories")
