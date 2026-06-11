set(DELTA_TEST_INCLUDE_DIRS
    "${CMAKE_SOURCE_DIR}/Engine"
    "${CMAKE_SOURCE_DIR}/Engine/Tests"
)

add_library(DeltaTestEnvironmentSupport OBJECT
    TestEnvironment.cpp
)
set_target_properties(DeltaTestEnvironmentSupport PROPERTIES
    CXX_STANDARD 23
    FOLDER "Tests/Shared"
)
target_include_directories(DeltaTestEnvironmentSupport PRIVATE ${DELTA_TEST_INCLUDE_DIRS})
target_link_libraries(DeltaTestEnvironmentSupport PRIVATE DeltaEngine ThirdParty::gtest)
target_compile_options(DeltaTestEnvironmentSupport PRIVATE
    "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->"
    "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/WX->"
)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    TestEnvironment.cpp
)

add_library(DeltaEditorSerializationTestSupport OBJECT
    SerializationTestSupport.cpp
    SerializationTestSupport.h
)
set_target_properties(DeltaEditorSerializationTestSupport PROPERTIES
    CXX_STANDARD 23
    FOLDER "Tests/Shared"
)
target_include_directories(DeltaEditorSerializationTestSupport PRIVATE ${DELTA_TEST_INCLUDE_DIRS})
target_link_libraries(DeltaEditorSerializationTestSupport PRIVATE DeltaEditor ThirdParty::gtest ThirdParty::nlohmann_json)
target_compile_options(DeltaEditorSerializationTestSupport PRIVATE
    "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->"
    "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/WX->"
)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    SerializationTestSupport.cpp
    SerializationTestSupport.h
)

add_library(DeltaGpuTestSupport OBJECT
    GpuD3D12Validation.cpp
    GpuD3D12Validation.h
    GpuGraphicsFixture.cpp
    GpuGraphicsFixture.h
)
set_target_properties(DeltaGpuTestSupport PROPERTIES
    CXX_STANDARD 23
    FOLDER "Tests/Shared"
)
target_include_directories(DeltaGpuTestSupport PRIVATE ${DELTA_TEST_INCLUDE_DIRS})
target_link_libraries(DeltaGpuTestSupport PRIVATE DeltaEngine ThirdParty::gtest)
target_compile_options(DeltaGpuTestSupport PRIVATE
    "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->"
    "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/WX->"
)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    GpuD3D12Validation.cpp
    GpuD3D12Validation.h
    GpuGraphicsFixture.cpp
    GpuGraphicsFixture.h
)

add_library(DeltaGpuAssetTestSupport OBJECT
    GpuFramebufferReadback.cpp
    GpuFramebufferReadback.h
    GpuSceneBuilder.cpp
    GpuSceneBuilder.h
    GpuTestAssetFixture.cpp
    GpuTestAssetFixture.h
)
set_target_properties(DeltaGpuAssetTestSupport PROPERTIES
    CXX_STANDARD 23
    FOLDER "Tests/Shared"
)
target_include_directories(DeltaGpuAssetTestSupport PRIVATE ${DELTA_TEST_INCLUDE_DIRS})
target_link_libraries(DeltaGpuAssetTestSupport PRIVATE DeltaEditor DeltaEngine ThirdParty::gtest)
target_compile_options(DeltaGpuAssetTestSupport PRIVATE
    "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->"
    "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/WX->"
)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    GpuFramebufferReadback.cpp
    GpuFramebufferReadback.h
    GpuSceneBuilder.cpp
    GpuSceneBuilder.h
    GpuTestAssetFixture.cpp
    GpuTestAssetFixture.h
)
