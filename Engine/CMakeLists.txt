if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options("/MP")
    set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT DeltaEditor)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(ThirdParty)
# include_directories(ThirdParty/sdl3/include)
# include_directories(ThirdParty/dxc/inc)
# include_directories(ThirdParty/DirectXTK12/Inc)
# include_directories(ThirdParty/DirectXTK12/Src)
# include_directories(ThirdParty/DirectXShaderCompiler/include)
# include_directories(ThirdParty/assimp/include)
# include_directories(ThirdParty/DirectXTex/Inc)

add_subdirectory(Runtime)
add_subdirectory(Editor)