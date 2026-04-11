set(TARGET_NAME DeltaEditor)

include("${CMAKE_SOURCE_DIR}/Engine/CMake/DeltaThirdPartyMSVC.cmake")

file(GLOB_RECURSE HEADER_FILES "*.h")
file(GLOB_RECURSE SOURCE_FILES "*.cpp")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${HEADER_FILES} ${SOURCE_FILES})

add_library(${TARGET_NAME} SHARED ${HEADER_FILES} ${SOURCE_FILES})

set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD 23 OUTPUT_NAME "DeltaEditor")
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Engine")

target_include_directories(${TARGET_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(${TARGET_NAME} PRIVATE DELTAEDITOR_EXPORTS)

target_compile_options(${TARGET_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/WX->")

delta_apply_third_party_msvc_external_includes(${TARGET_NAME})

target_link_libraries(${TARGET_NAME} DeltaEngine ThirdParty::imguizmo)

# Copy Editor fonts to output dir so ApplyDeltaTheme can load them
set(EDITOR_FONTS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/Assets/Fonts")
if(EXISTS "${EDITOR_FONTS_SRC}")
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${EDITOR_FONTS_SRC}"
            "$<TARGET_FILE_DIR:${TARGET_NAME}>/Editor/Assets/Fonts"
        COMMENT "Copying Editor fonts to output directory"
    )
endif()
