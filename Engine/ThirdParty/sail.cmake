set(SAIL_BUILD_APPS OFF)
set(SAIL_BUILD_EXAMPLES OFF)
set(SAIL_COMBINE_CODECS ON)

set(BUILD_SHARED_LIBS ON)

add_subdirectory(sail)

set(BUILD_SHARED_LIBS OFF)

set_target_properties(sail PROPERTIES FOLDER ${third_party_folder})