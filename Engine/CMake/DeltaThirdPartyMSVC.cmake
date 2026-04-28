function(delta_apply_third_party_msvc_external_includes target)
    if(NOT MSVC)
        return()
    endif()
    set(_tp "${CMAKE_SOURCE_DIR}/Engine/ThirdParty")
    set(_sdl3_bin "${CMAKE_BINARY_DIR}/Engine/ThirdParty/SDL3")
    set(_roots
        "${_tp}/SDL3/include"
        "${_sdl3_bin}/include-revision"
        "${_tp}/assimp/include"
        "${_tp}/DirectXTK12/Inc"
        "${_tp}/DirectXTex/DirectXTex"
        "${_tp}/googletest/googletest/include"
        "${_tp}/googletest/googlemock/include"
        "${_tp}/pix/Include/WinPixEventRuntime"
    )
    foreach(_root IN LISTS _roots)
        target_compile_options(${target} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:SHELL:/external:I \"${_root}\">")
    endforeach()
endfunction()
