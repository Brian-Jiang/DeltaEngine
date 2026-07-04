message(STATUS "====== Delta Engine ====== Configuring dx12 agility ======")

delta_add_sync_directory_target(CopyDx12AgilityBin
    "${CMAKE_CURRENT_SOURCE_DIR}/dx12agility/bin/x64"
    "${CMAKE_BINARY_DIR}/bin/D3D12")

set(_dx12_agility_root "${CMAKE_CURRENT_SOURCE_DIR}/dx12agility")
add_library(dx12agility INTERFACE)
target_include_directories(dx12agility SYSTEM INTERFACE
    $<BUILD_INTERFACE:${_dx12_agility_root}/include>
    $<BUILD_INTERFACE:${_dx12_agility_root}/include/d3dx12>
)
set_target_properties(dx12agility PROPERTIES FOLDER ${third_party_folder})
add_library(ThirdParty::dx12agility ALIAS dx12agility)
