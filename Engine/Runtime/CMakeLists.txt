set(TARGET_NAME DeltaEngine)

include("${CMAKE_SOURCE_DIR}/Engine/CMake/DeltaThirdPartyMSVC.cmake")

file(GLOB_RECURSE HEADER_FILES "*.h")
file(GLOB_RECURSE SOURCE_FILES "*.cpp")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${HEADER_FILES} ${SOURCE_FILES})

add_library(${TARGET_NAME} SHARED ${HEADER_FILES} ${SOURCE_FILES})

target_compile_definitions(${TARGET_NAME} PRIVATE DELTAENGINE_EXPORTS)

set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD 23)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Engine")

target_include_directories(${TARGET_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(${TARGET_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/Engine")

add_dependencies(${TARGET_NAME} CopyDx12AgilityBin)
add_dependencies(${TARGET_NAME} CopyDxcBin)
add_dependencies(${TARGET_NAME} CopySlangBin)

target_compile_options(${TARGET_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->")
target_compile_options(${TARGET_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/WX->")

delta_apply_third_party_msvc_external_includes(${TARGET_NAME})

target_link_libraries(${TARGET_NAME} PUBLIC ThirdParty::SDL3)
target_link_libraries(${TARGET_NAME} PUBLIC ThirdParty::imgui)
target_link_libraries(${TARGET_NAME} PUBLIC ThirdParty::assimp)
target_link_libraries(${TARGET_NAME} PUBLIC ThirdParty::dx12agility)
target_link_libraries(${TARGET_NAME} PUBLIC ThirdParty::dxheaders)
target_link_libraries(${TARGET_NAME} PUBLIC ThirdParty::DirectXTK12)
target_link_libraries(${TARGET_NAME} PUBLIC ThirdParty::DirectXTex)
target_link_libraries(${TARGET_NAME} PUBLIC ThirdParty::nlohmann_json)
target_link_libraries(${TARGET_NAME} PUBLIC ThirdParty::spdlog)
target_link_libraries(${TARGET_NAME} PUBLIC ThirdParty::asio)
target_link_libraries(${TARGET_NAME} PUBLIC ThirdParty::slang)

target_link_libraries(${TARGET_NAME} PUBLIC d3d12.lib)
target_link_libraries(${TARGET_NAME} PUBLIC dxguid.lib dxgi.lib d3dcompiler.lib)
target_link_libraries(${TARGET_NAME} PUBLIC dxcompiler.dll)

# ── DeltaHeaderTool integration ──────────────────────────────

include("${CMAKE_SOURCE_DIR}/Tools/CMake/PythonSetup.cmake")

set(HEADER_TOOL   "${CMAKE_SOURCE_DIR}/Tools/DeltaHeaderTool/main.py")
set(INTERMEDIATE  "${CMAKE_SOURCE_DIR}/Intermediate/DeltaHeaderTool")
set(GENERATED_DIR "${INTERMEDIATE}/Generated")
set(MANIFEST      "${INTERMEDIATE}/generated_sources.cmake")

set(HEADER_TOOL_ARGS
    "${DELTA_PYTHON}" "${HEADER_TOOL}"
    --input-dir   "${CMAKE_SOURCE_DIR}/Engine/Runtime"
    --output-dir  "${GENERATED_DIR}"
    --manifest    "${MANIFEST}"
    --engine-root "${CMAKE_SOURCE_DIR}"
    --include-dir "${CMAKE_SOURCE_DIR}/Engine"
    --include-dir "${CMAKE_CURRENT_SOURCE_DIR}"
    --include-dir "${CMAKE_SOURCE_DIR}/Engine/ThirdParty/DirectXTK12/Inc"
    --include-dir "${CMAKE_SOURCE_DIR}/Engine/ThirdParty/DirectX-Headers/include/directx"
)

file(MAKE_DIRECTORY "${GENERATED_DIR}")

# ── Configure time: consume the manifest from the last build (or create it).
#    If the manifest does not exist (first configure or clean Intermediate),
#    run the header tool once so the first build has the full source list and
#    links successfully. Otherwise CMake would only reconfigure at the *next*
#    build (CONFIGURE_DEPENDS), so the first build would link with a stale list.
if(NOT EXISTS "${MANIFEST}")
    message(STATUS "DeltaHeaderTool: manifest missing, running once to generate sources...")
    execute_process(
        COMMAND ${HEADER_TOOL_ARGS}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE _header_tool_result
    )
    if(NOT _header_tool_result EQUAL 0)
        message(FATAL_ERROR "DeltaHeaderTool failed (exit ${_header_tool_result}). Fix errors and reconfigure.")
    endif()
endif()
if(NOT EXISTS "${MANIFEST}")
    file(WRITE "${MANIFEST}" "set(DELTA_GENERATED_SOURCES \"\")\n")
endif()

# If the manifest changes (new .cpp files added/removed by the tool) CMake
# will automatically reconfigure before the next compile, picking up the
# new source list.
set_property(
    DIRECTORY "${CMAKE_SOURCE_DIR}"
    APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${MANIFEST}"
)

include("${MANIFEST}")

target_sources(${TARGET_NAME} PRIVATE ${DELTA_GENERATED_SOURCES})
target_include_directories(${TARGET_NAME} PUBLIC "${GENERATED_DIR}")

# ── Build time: run the header tool before DeltaEngine (and thus DeltaEditor)
#    every time they are built. PRE_BUILD does not run when the target is
#    up-to-date (e.g. building only DeltaEditor), so we use a dependency
#    on a custom target with no outputs so the tool always runs when building
#    this target or any dependent (e.g. DeltaEditor).
add_custom_target(DeltaHeaderToolRun
    COMMAND ${HEADER_TOOL_ARGS}
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "DeltaHeaderTool: regenerating reflection code..."
)
add_dependencies(${TARGET_NAME} DeltaHeaderToolRun)
set_target_properties(DeltaHeaderToolRun PROPERTIES FOLDER "Utility")

# ── Manual force-regeneration target (runnable from VS: build this target) ─
add_custom_target(DeltaHeaderTool
    COMMAND ${HEADER_TOOL_ARGS} --force
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "DeltaHeaderTool: force regenerating all reflection code..."
    SOURCES
        "${CMAKE_SOURCE_DIR}/Tools/DeltaHeaderTool/main.py"
        "${CMAKE_SOURCE_DIR}/Tools/DeltaHeaderTool/parser.py"
        "${CMAKE_SOURCE_DIR}/Tools/DeltaHeaderTool/generator.py"
        "${CMAKE_SOURCE_DIR}/Tools/DeltaHeaderTool/type_resolver.py"
        "${CMAKE_SOURCE_DIR}/Tools/DeltaHeaderTool/templates.py"
)
set_target_properties(DeltaHeaderTool PROPERTIES FOLDER "Utility")
