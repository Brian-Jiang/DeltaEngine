set(TARGET_NAME DeltaEngineEditor)

add_executable (${TARGET_NAME} "Main.cpp")

set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD 20 OUTPUT_NAME "DeltaEngineEditor")
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Engine")

target_compile_options(${TARGET_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/WX->")

target_link_libraries(${TARGET_NAME} DeltaEngine)

#include_directories(${TARGET_NAME} PUBLIC Engine)
