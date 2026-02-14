set(TARGET_NAME DeltaEngine)

file(GLOB_RECURSE HEADER_FILES "*.h")
file(GLOB_RECURSE SOURCE_FILES "*.cpp")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${HEADER_FILES} ${SOURCE_FILES})

add_library(${TARGET_NAME} ${HEADER_FILES} ${SOURCE_FILES})

set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD 23)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Engine")

target_include_directories(${TARGET_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

add_dependencies(${TARGET_NAME} CopyDxcBin)

target_compile_options(${TARGET_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->")
target_compile_options(${TARGET_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/WX->")

target_link_libraries(${TARGET_NAME} PUBLIC SDL3::SDL3)
target_link_libraries(${TARGET_NAME} PUBLIC imgui)
target_link_libraries(${TARGET_NAME} PUBLIC assimp)
target_link_libraries(${TARGET_NAME} PUBLIC dxheaders)
target_link_libraries(${TARGET_NAME} PUBLIC DirectXTK12)
target_link_libraries(${TARGET_NAME} PUBLIC DirectXTex)

target_link_libraries(${TARGET_NAME} PUBLIC d3d12.lib)
target_link_libraries(${TARGET_NAME} PUBLIC dxguid.lib dxgi.lib d3dcompiler.lib)
target_link_libraries(${TARGET_NAME} PUBLIC dxcompiler.dll)

# add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Shaders")
