message(STATUS "====== Delta Engine ====== Configuring DirectX-Headers ======")

set(dxheaders_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/DirectX-Headers)

file(GLOB dxheaders_files CONFIGURE_DEPENDS
    "${dxheaders_SOURCE_DIR}/include/directx/d3dx12*.*"
)

add_library(dxheaders INTERFACE)
target_sources(dxheaders INTERFACE ${dxheaders_files})
target_include_directories(dxheaders SYSTEM INTERFACE 
    $<BUILD_INTERFACE:${dxheaders_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${dxheaders_SOURCE_DIR}/include/directx>
)

set_target_properties(dxheaders PROPERTIES FOLDER ${third_party_folder})

add_library(ThirdParty::dxheaders ALIAS dxheaders)
