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
target_link_libraries(DeltaTestEnvironmentSupport PRIVATE DeltaEngine gtest)
target_compile_options(DeltaTestEnvironmentSupport PRIVATE
    "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->"
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
target_link_libraries(DeltaEditorSerializationTestSupport PRIVATE DeltaEditor gtest nlohmann_json)
target_compile_options(DeltaEditorSerializationTestSupport PRIVATE
    "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->"
)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    SerializationTestSupport.cpp
    SerializationTestSupport.h
)
