set(TARGET_NAME DeltaEngine)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE HEADER_FILES "*.h")
file(GLOB_RECURSE SOURCE_FILES "*.cpp")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${HEADER_FILES} ${SOURCE_FILES})

add_library(${TARGET_NAME} ${HEADER_FILES} ${SOURCE_FILES})

set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD 20)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Engine")

target_compile_options(${TARGET_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->")
target_compile_options(${TARGET_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/WX->")

add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy
                   ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/dxc/bin/x86/dxcompiler.dll
                   $<TARGET_FILE_DIR:${TARGET_NAME}>)

add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy
                   ${CMAKE_SOURCE_DIR}/Engine/ThirdParty/dxc/bin/x86/dxil.dll
                   $<TARGET_FILE_DIR:${TARGET_NAME}>)


target_link_libraries(${TARGET_NAME} PRIVATE glew)
target_link_libraries(${TARGET_NAME} PRIVATE glfw)
target_link_libraries(${TARGET_NAME} PRIVATE SDL3::SDL3)
target_link_libraries(${TARGET_NAME} PRIVATE imgui)
# target_link_libraries(${TARGET_NAME} PUBLIC sail-c++)
# target_link_libraries(${TARGET_NAME} PUBLIC sail-codecs)
target_link_libraries(${TARGET_NAME} PUBLIC "dxguid.lib" "dxgi.lib" "d3dcompiler.lib")
target_link_libraries(${TARGET_NAME} PUBLIC "dxcompiler.dll")
target_link_libraries(${TARGET_NAME} PUBLIC DirectXTK12)
target_link_libraries(${TARGET_NAME} PUBLIC DirectXTex)

target_link_libraries(${TARGET_NAME} PUBLIC d3d12.lib)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Shaders")
# add_dependencies(${TARGET_NAME} shaders)
