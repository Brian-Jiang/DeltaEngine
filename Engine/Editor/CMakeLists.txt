set(TARGET_NAME DeltaEditor)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(${TARGET_NAME} "Main.cpp" "EditorMain.cpp" "EditorRenderManager.cpp")

set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD 23 OUTPUT_NAME "DeltaEditor")
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Engine")

target_compile_options(${TARGET_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/WX->")

target_link_libraries(${TARGET_NAME} DeltaEngine)
