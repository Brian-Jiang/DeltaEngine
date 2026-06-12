set(DELTA_ENGINE_TEST_SOURCES
    Assets/DPrimaryAssetTests.cpp
    Assets/DPrimaryAssetGCRootingTests.cpp
    Assets/AssetDatabaseLocatorTests.cpp
    Assets/PA_CommonAssetsTests.cpp
    Assets/PA_DSceneTests.cpp
    Assets/SceneAssetIntegrationTests.cpp
    Graphics/CameraPostProcessStackTests.cpp
    Graphics/CameraRenderProxyTests.cpp
    Graphics/DefaultTexturesHardeningTests.cpp
    Graphics/DXGraphicsContextHardeningTests.cpp
    Graphics/GraphicsStructureTests.cpp
    Graphics/IBLBakerHardeningTests.cpp
    Graphics/LightComponentHardeningTests.cpp
    Graphics/LightProxyHardeningTests.cpp
    Graphics/LightProxyShadowTests.cpp
    Graphics/MeshRendererTests.cpp
    Graphics/MeshRenderProxyTests.cpp
    Graphics/RenderResourceReleaseQueueTests.cpp
    Graphics/MeshShadowSubmissionTests.cpp
    Graphics/PostProcessTests_DataModel.cpp
    Graphics/PostProcessStackHardeningTests.cpp
    Graphics/PostProcessRenderGraphCompileTests.cpp
    Graphics/RenderCoreTests.cpp
    Graphics/FullFrameRenderGraphCompileTests.cpp
    Graphics/Gpu/GpuDeviceSmokeTests.cpp
    Graphics/Gpu/GpuDefaultTexturesTests.cpp
    Graphics/Gpu/GpuIBLBakerTests.cpp
    Graphics/Gpu/GpuRenderManagerInitTests.cpp
    Graphics/Gpu/GpuShadowInfrastructureTests.cpp
    Graphics/Gpu/GpuShaderCompileTests.cpp
    Graphics/Gpu/GpuDefaultSceneTests.cpp
    Graphics/Gpu/GpuFrameSmokeTests.cpp
    Graphics/Gpu/GpuLifecycleTests.cpp
    Graphics/Gpu/GpuMeshAndShadowTests.cpp
    Graphics/Gpu/GpuPostProcessTests.cpp
    Graphics/RenderGraphCompileTests.cpp
    Graphics/RenderGraphTests.cpp
    Graphics/SceneRenderGraphCompileTests.cpp
    Graphics/ShadowMapAllocatorTests.cpp
    Graphics/ShadowRenderGraphCompileTests.cpp
    Graphics/DirectionalShadowMatrixTests.cpp
    Graphics/SkyboxRenderProxyTests.cpp
    Graphics/TransientTexturePoolTests.cpp
    Reflection/ReflectionRegistryTests.cpp
    Reflection/ReflectionRegistryHardeningTests.cpp
    Reflection/ReflectionStructClassTests.cpp
    Reflection/ReflectionPropertyTests.cpp
    Reflection/ReflectionReferenceTraversalTests.cpp
    Serialization/Core/UuidTests.cpp
    Serialization/Core/ScriptPointerTests.cpp
    Serialization/Core/BulkDataHandleTests.cpp
    Serialization/Core/TBulkDataTests.cpp
    Serialization/Core/JsonAssetArchiveTests.cpp
    Serialization/Core/JsonAssetArchiveHardeningTests.cpp
    Assets/Locator/NullAssetDatabaseTests.cpp
    Core/CoreObjectLifecycleTests.cpp
    Core/GCObjectRegistryTests.cpp
    Core/GCMarkTests.cpp
    Core/GCSweepTests.cpp
    Core/GCRenderSweepTests.cpp
    Core/SceneComponentTests.cpp
    Core/CoreAssetDataTests.cpp
    Core/CoreUtilityTests.cpp
    Core/EngineMainHardeningTests.cpp
    IO/IOManagerTests.cpp
    Logging/LogCategoryTests.cpp
    Logging/LoggingManagerTests.cpp
    Utils/StringUtilsTests.cpp
)

add_executable(DeltaEngineTests
    ${DELTA_ENGINE_TEST_SOURCES}
    $<TARGET_OBJECTS:DeltaTestEnvironmentSupport>
    $<TARGET_OBJECTS:DeltaGpuTestSupport>
    $<TARGET_OBJECTS:DeltaGpuAssetTestSupport>
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
target_link_libraries(DeltaEngineTests PRIVATE DeltaEditor DeltaEngine ThirdParty::gtest_main)
target_compile_options(DeltaEngineTests PRIVATE
    "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->"
    "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/WX->"
)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${DELTA_ENGINE_TEST_SOURCES})

gtest_discover_tests(DeltaEngineTests DISCOVERY_TIMEOUT 60)
