message(STATUS "====== Delta Engine ====== Configuring pix (WinPixEventRuntime) ======")

if(NOT MSVC)
    return()
endif()

set(_pix_root "${CMAKE_CURRENT_SOURCE_DIR}/pix")
set(_pix_dll_src "${_pix_root}/bin/x64/WinPixEventRuntime.dll")

delta_add_sync_directory_target(CopyPixBin
    "${_pix_dll_src}"
    "${CMAKE_BINARY_DIR}/bin/WinPixEventRuntime.dll")

add_library(pix INTERFACE)
target_include_directories(pix SYSTEM INTERFACE
    $<BUILD_INTERFACE:${_pix_root}/Include/WinPixEventRuntime>)
target_link_libraries(pix INTERFACE
    "${_pix_root}/bin/x64/WinPixEventRuntime.lib")
set_target_properties(pix PROPERTIES FOLDER ${third_party_folder})
add_library(ThirdParty::pix ALIAS pix)
