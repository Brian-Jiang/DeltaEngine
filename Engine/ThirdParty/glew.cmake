set(BUILD_UTILS OFF)
set(GLEW_REGAL OFF)
set(GLEW_OSMESA OFF)
set(BUILD_FRAMEWORK OFF)

add_subdirectory(glew/build/cmake)

set_target_properties(glew PROPERTIES FOLDER ${third_party_folder})
set_target_properties(glew_s PROPERTIES FOLDER ${third_party_folder})