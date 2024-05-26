set(BUILD_XAUDIO_WIN10 OFF)

add_subdirectory(DirectXTK12)

set_target_properties(DirectXTK12 PROPERTIES FOLDER ${third_party_folder})


set(BUILD_SAMPLE OFF)
set(BUILD_TOOLS OFF)
set(BUILD_XBOX_EXTS_SCARLETT OFF)
set(BUILD_XBOX_EXTS_XBOXONE OFF)
set(BUILD_DX11 OFF)

add_subdirectory(DirectXTex)

set_target_properties(DirectXTex PROPERTIES FOLDER ${third_party_folder})


install(DIRECTORY ${third_party_folder}/dxc/bin DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/bin)
# add_custom_command(TARGET YourTargetName POST_BUILD
#                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
#                    ${third_party_folder}/dxc/bin/dxcompiler.dll
#                    $<TARGET_FILE_DIR:YourTargetName>)
