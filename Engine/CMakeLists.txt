if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options("/MP")
    set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT DeltaEngineEditor)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(Runtime)
add_subdirectory(Editor)