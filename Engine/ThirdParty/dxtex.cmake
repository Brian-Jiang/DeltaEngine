set(BUILD_SAMPLE OFF)
set(BUILD_TOOLS OFF)
set(BUILD_XBOX_EXTS_SCARLETT OFF)
set(BUILD_XBOX_EXTS_XBOXONE OFF)
set(BUILD_DX11 OFF)

add_subdirectory(DirectXTex)

set_target_properties(DirectXTex PROPERTIES FOLDER ${third_party_folder})