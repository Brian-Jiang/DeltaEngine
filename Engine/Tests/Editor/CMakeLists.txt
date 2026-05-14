set(DELTA_EDITOR_TEST_SOURCES
    EditorCommandTests_SetProperty.cpp
    EditorCommandTests_SetAssetDynamicMeta.cpp
    EditorCommandTests_SceneStructure.cpp
    EditorCommandTests_Scene_SaveLoad.cpp
    EditorCommandTests_CommandQueue.cpp
    EditorCommandTests_McpPath.cpp
    EditorCommandTests_UndoStack.cpp
    Commands/EditorCommandBatchTests.cpp
    Commands/EditorCommandManagerTests.cpp
    Commands/EditorCommandPropertyValueIOTests.cpp
    Commands/EditorCommandRegistryTests.cpp
    Commands/EditorCommandRenameAssetTests.cpp
    Panels/EditorChromePanelTests.cpp
    Style/EditorThemeLayoutTests.cpp
    UI/EditorThemeColorTests.cpp
    UIComponents/WidgetEditEventTests.cpp
    UIComponents/EditorInlineRenameTests.cpp
    UIComponents/ClassPickerPopupTests.cpp
    UIComponents/ContextMenuPopupTests.cpp
    UIComponents/PropertyWidgetsTests.cpp
    UIComponents/HorizontalToggleGroupTests.cpp
    UIComponents/ObjectPtrFieldTests.cpp
    EditorWindows/EditorAssetBrowserPathTests.cpp
    EditorWindows/EditorDetailsPropertyFormattingTests.cpp
    EditorWindows/EditorOutlinerFilteringTests.cpp
    EditorWindows/EditorViewportCameraPersistenceTests.cpp
    EditorWindows/EditorWindowConceptTests.cpp
    EditorWindows/EditorWindowIdTests.cpp
    EditorWindows/ViewportPresetTests.cpp
    Assets/Locator/EditorAssetDatabaseLocatorTests.cpp
    Assets/Lifecycle/EditorAssetDatabaseLifecycleTests.cpp
    Assets/Lifecycle/AssetMetaCacheTests.cpp
    Assets/Importer/AssetImporterTests.cpp
    Core/EditorCoreTests.cpp
    EditorViewportCameraTests.cpp
    State/EditorSelectionStateTests.cpp
    Serialization/RoundTrip/SingleAssetRoundTripTests.cpp
    Serialization/RoundTrip/TypedPrimaryAssetRoundTripTests.cpp
    Serialization/RoundTrip/DynamicMetaRoundTripTests.cpp
    Serialization/RoundTrip/StaticMetaRoundTripTests.cpp
    Serialization/RoundTrip/StaticMetaRoundTripTests_Texture.cpp
    Serialization/Compatibility/FallbackAndDefaultsTests.cpp
    Serialization/References/CrossAssetReferenceTests.cpp
    Serialization/BulkData/BulkDataRoundTripTests.cpp
    Serialization/Snapshot/PartialSnapshotTests.cpp
    Serialization/Snapshot/ObjectSnapshotHardeningTests.cpp
    Mcp/McpSceneSystemTests.cpp
    Mcp/McpAssetsSystemTests.cpp
    Mcp/McpAssetsMetaTests.cpp
    Mcp/McpSelectionSystemTests.cpp
    Mcp/McpUndoSystemTests.cpp
    Mcp/McpViewportSystemTests.cpp
    Mcp/McpCommonSystemTests.cpp
    Mcp/McpReflectionSystemTests.cpp
    Mcp/McpProjectSystemTests.cpp
    Mcp/McpMetaSystemTests.cpp
    Mcp/McpLogSystemTests.cpp
    Mcp/McpQueryRouterTests.cpp
    Mcp/McpRegistryDispatchTests.cpp
    Mcp/McpQueryRouterTests.cpp
    Mcp/McpRegistryDispatchTests.cpp
)

add_executable(DeltaEditorTests
    ${DELTA_EDITOR_TEST_SOURCES}
    $<TARGET_OBJECTS:DeltaTestEnvironmentSupport>
    $<TARGET_OBJECTS:DeltaEditorSerializationTestSupport>
)
set_target_properties(DeltaEditorTests PROPERTIES
    CXX_STANDARD 23
    FOLDER "Tests/Editor"
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:DeltaEditorTests>"
)
target_include_directories(DeltaEditorTests PRIVATE
    "${CMAKE_SOURCE_DIR}/Engine"
    "${CMAKE_SOURCE_DIR}/Engine/Tests"
)
target_link_libraries(DeltaEditorTests PRIVATE DeltaEditor DeltaEngine ThirdParty::gtest_main)
target_compile_options(DeltaEditorTests PRIVATE
    "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->"
    "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/EHsc>"
    "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/WX->"
)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${DELTA_EDITOR_TEST_SOURCES})

gtest_discover_tests(DeltaEditorTests DISCOVERY_TIMEOUT 60)
