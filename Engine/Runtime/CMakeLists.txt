set(TARGET_NAME DeltaEngine)

file(GLOB_RECURSE HEADER_FILES "*.h")
file(GLOB_RECURSE SOURCE_FILES "*.cpp")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${HEADER_FILES} ${SOURCE_FILES})

add_library(${TARGET_NAME} ${HEADER_FILES} ${SOURCE_FILES})

set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD 20)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Engine")

target_compile_options(${TARGET_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->")
target_compile_options(${TARGET_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/WX->")

target_link_libraries(${TARGET_NAME} PRIVATE glew)
target_link_libraries(${TARGET_NAME} PRIVATE glfw)
target_link_libraries(${TARGET_NAME} PRIVATE SDL3::SDL3)
target_link_libraries(${TARGET_NAME} PRIVATE imgui)
# target_link_libraries(${TARGET_NAME} PUBLIC sail-c++)
# target_link_libraries(${TARGET_NAME} PUBLIC sail-codecs)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Shaders")
add_dependencies(${TARGET_NAME} shaders)
