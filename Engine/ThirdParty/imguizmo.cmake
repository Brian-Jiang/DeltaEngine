message(STATUS "====== Delta Engine ====== Configuring ImGuizmo ======")

set(imguizmo_SOURCE_DIR_ ${CMAKE_CURRENT_SOURCE_DIR}/ImGuizmo)

file(GLOB imguizmo_sources CONFIGURE_DEPENDS
    "${imguizmo_SOURCE_DIR_}/ImGuizmo.cpp"
    "${imguizmo_SOURCE_DIR_}/ImSequencer.cpp"
    "${imguizmo_SOURCE_DIR_}/ImCurveEdit.cpp"
    "${imguizmo_SOURCE_DIR_}/ImGradient.cpp"
    "${imguizmo_SOURCE_DIR_}/GraphEditor.cpp"
)

add_library(imguizmo STATIC ${imguizmo_sources})
target_include_directories(imguizmo SYSTEM PUBLIC
    $<BUILD_INTERFACE:${imguizmo_SOURCE_DIR_}>)
target_link_libraries(imguizmo PUBLIC ThirdParty::imgui)

add_library(ThirdParty::imguizmo ALIAS imguizmo)

set_target_properties(imguizmo PROPERTIES FOLDER ${third_party_folder})
