set(DELTA_ENGINE_TEST_SOURCES
    Assets/DPrimaryAssetTests.cpp
    Assets/SceneAssetIntegrationTests.cpp
    Serialization/Core/UuidTests.cpp
    Serialization/Core/ScriptPointerTests.cpp
    Serialization/Core/BulkDataHandleTests.cpp
    Assets/Locator/NullAssetDatabaseTests.cpp
)

add_executable(DeltaEngineTests
    ${DELTA_ENGINE_TEST_SOURCES}
    $<TARGET_OBJECTS:DeltaTestEnvironmentSupport>
)
set_target_properties(DeltaEngineTests PROPERTIES
    CXX_STANDARD 23
    FOLDER "Tests/Engine"
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:DeltaEngineTests>"
)
target_include_directories(DeltaEngineTests PRIVATE
    "${CMAKE_SOURCE_DIR}/Engine"
    "${CMAKE_SOURCE_DIR}/Engine/Tests"
)
target_link_libraries(DeltaEngineTests PRIVATE DeltaEngine ThirdParty::gtest_main)
target_compile_options(DeltaEngineTests PRIVATE
    "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->"
    "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/WX->"
)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${DELTA_ENGINE_TEST_SOURCES})

gtest_discover_tests(DeltaEngineTests DISCOVERY_TIMEOUT 60)
