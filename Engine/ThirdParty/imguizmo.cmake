message(STATUS "====== Delta Engine ====== Configuring ImGuizmo ======")

set(IMGUIZMO_BUILD_EXAMPLE OFF)

add_subdirectory(ImGuizmo)

target_link_libraries(imguizmo PUBLIC imgui)
set_target_properties(imguizmo PROPERTIES FOLDER ${third_party_folder})

add_library(ThirdParty::imguizmo ALIAS imguizmo)
