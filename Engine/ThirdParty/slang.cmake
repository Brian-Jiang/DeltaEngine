message(STATUS "====== Delta Engine ====== Configuring slang ======")

set(_slang_root "${CMAKE_CURRENT_SOURCE_DIR}/slang")

delta_add_sync_directory_target(CopySlangBin
    "${_slang_root}/bin"
    "${CMAKE_BINARY_DIR}/bin")

add_library(slang INTERFACE)
target_include_directories(slang SYSTEM INTERFACE
    $<BUILD_INTERFACE:${_slang_root}/include>)
target_link_libraries(slang INTERFACE
    "${_slang_root}/lib/slang.lib")
set_target_properties(slang PROPERTIES FOLDER ${third_party_folder})
add_library(ThirdParty::slang ALIAS slang)
