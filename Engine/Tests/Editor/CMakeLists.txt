set(DELTA_EDITOR_TEST_SOURCES
    Assets/Locator/EditorAssetDatabaseLocatorTests.cpp
    Assets/Lifecycle/EditorAssetDatabaseLifecycleTests.cpp
    State/EditorSelectionStateTests.cpp
    Serialization/RoundTrip/SingleAssetRoundTripTests.cpp
    Serialization/RoundTrip/TypedPrimaryAssetRoundTripTests.cpp
    Serialization/Compatibility/FallbackAndDefaultsTests.cpp
    Serialization/References/CrossAssetReferenceTests.cpp
    Serialization/BulkData/BulkDataRoundTripTests.cpp
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
target_link_libraries(DeltaEditorTests PRIVATE DeltaEditor ThirdParty::gtest_main)
target_compile_options(DeltaEditorTests PRIVATE
    "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->"
    "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/EHsc>"
    "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/WX->"
)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${DELTA_EDITOR_TEST_SOURCES})

gtest_discover_tests(DeltaEditorTests DISCOVERY_TIMEOUT 60)
