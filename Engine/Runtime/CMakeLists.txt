set(TARGET_NAME DeltaEngine)

file(GLOB_RECURSE HEADER_FILES "*.h")
file(GLOB_RECURSE SOURCE_FILES "*.cpp")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${HEADER_FILES} ${SOURCE_FILES})

add_library(${TARGET_NAME} SHARED ${HEADER_FILES} ${SOURCE_FILES})

target_compile_definitions(${TARGET_NAME} PRIVATE DELTAENGINE_EXPORTS)

set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD 23)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Engine")

target_include_directories(${TARGET_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

add_dependencies(${TARGET_NAME} CopyDxcBin)

target_compile_options(${TARGET_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->")
target_compile_options(${TARGET_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/WX->")

target_link_libraries(${TARGET_NAME} PUBLIC SDL3::SDL3)
target_link_libraries(${TARGET_NAME} PUBLIC imgui)
target_link_libraries(${TARGET_NAME} PUBLIC assimp)
target_link_libraries(${TARGET_NAME} PUBLIC dxheaders)
target_link_libraries(${TARGET_NAME} PUBLIC DirectXTK12)
target_link_libraries(${TARGET_NAME} PUBLIC DirectXTex)

target_link_libraries(${TARGET_NAME} PUBLIC d3d12.lib)
target_link_libraries(${TARGET_NAME} PUBLIC dxguid.lib dxgi.lib d3dcompiler.lib)
target_link_libraries(${TARGET_NAME} PUBLIC dxcompiler.dll)

# add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Shaders")

# ── DeltaHeaderTool integration ──────────────────────────────

include("${CMAKE_SOURCE_DIR}/Tools/CMake/PythonSetup.cmake")

set(HEADER_TOOL   "${CMAKE_SOURCE_DIR}/Tools/DeltaHeaderTool/main.py")
set(INTERMEDIATE  "${CMAKE_SOURCE_DIR}/Intermediate/DeltaHeaderTool")
set(GENERATED_DIR "${INTERMEDIATE}/Generated")
set(MANIFEST      "${INTERMEDIATE}/generated_sources.cmake")

file(MAKE_DIRECTORY "${GENERATED_DIR}")

if(NOT EXISTS "${MANIFEST}")
    file(WRITE "${MANIFEST}" "set(DELTA_GENERATED_SOURCES \"\")\n")
endif()

execute_process(
    COMMAND "${DELTA_PYTHON}" "${HEADER_TOOL}"
            --input-dir   "Engine/Runtime"
            --output-dir  "${GENERATED_DIR}"
            --manifest    "${MANIFEST}"
            --engine-root "${CMAKE_SOURCE_DIR}"
            --include-dir "${CMAKE_SOURCE_DIR}/Engine"
            --include-dir "${CMAKE_CURRENT_SOURCE_DIR}"
            --include-dir "${CMAKE_SOURCE_DIR}/Engine/ThirdParty/DirectXTK12/Inc"
            --include-dir "${CMAKE_SOURCE_DIR}/Engine/ThirdParty/DirectX-Headers/include/directx"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    RESULT_VARIABLE HEADER_TOOL_RESULT
    ERROR_VARIABLE  HEADER_TOOL_ERROR
)
if(NOT HEADER_TOOL_RESULT EQUAL 0)
    message(FATAL_ERROR "DeltaHeaderTool failed:\n${HEADER_TOOL_ERROR}")
endif()

include("${MANIFEST}")

target_sources(${TARGET_NAME} PRIVATE ${DELTA_GENERATED_SOURCES})
target_include_directories(${TARGET_NAME} PRIVATE "${GENERATED_DIR}")

file(GLOB_RECURSE REFLECTED_HEADERS "${CMAKE_SOURCE_DIR}/Engine/Runtime/*.h")

add_custom_command(
    OUTPUT  "${MANIFEST}"
    COMMAND "${DELTA_PYTHON}" "${HEADER_TOOL}"
            --input-dir   "Engine/Runtime"
            --output-dir  "${GENERATED_DIR}"
            --manifest    "${MANIFEST}"
            --engine-root "${CMAKE_SOURCE_DIR}"
            --include-dir "${CMAKE_SOURCE_DIR}/Engine"
            --include-dir "${CMAKE_CURRENT_SOURCE_DIR}"
            --include-dir "${CMAKE_SOURCE_DIR}/Engine/ThirdParty/DirectXTK12/Inc"
            --include-dir "${CMAKE_SOURCE_DIR}/Engine/ThirdParty/DirectX-Headers/include/directx"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    DEPENDS ${REFLECTED_HEADERS}
    COMMENT "DeltaHeaderTool: generating reflection code..."
)

add_custom_target(DeltaHeaderTool
    COMMAND "${DELTA_PYTHON}" "${HEADER_TOOL}"
            --input-dir   "Engine/Runtime"
            --output-dir  "${GENERATED_DIR}"
            --manifest    "${MANIFEST}"
            --engine-root "${CMAKE_SOURCE_DIR}"
            --include-dir "${CMAKE_SOURCE_DIR}/Engine"
            --include-dir "${CMAKE_CURRENT_SOURCE_DIR}"
            --include-dir "${CMAKE_SOURCE_DIR}/Engine/ThirdParty/DirectXTK12/Inc"
            --include-dir "${CMAKE_SOURCE_DIR}/Engine/ThirdParty/DirectX-Headers/include/directx"
            --force
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

add_custom_target(DeltaHeaderTool_PreBuild DEPENDS "${MANIFEST}")
set_target_properties(DeltaHeaderTool_PreBuild PROPERTIES FOLDER "Utility")
add_dependencies(${TARGET_NAME} DeltaHeaderTool_PreBuild)
