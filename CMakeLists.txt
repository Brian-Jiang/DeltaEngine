# CMakeList.txt : CMake project for DeltaEngine, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("DeltaEngine")

find_package(fmt CONFIG REQUIRED)

set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}")

add_subdirectory(ThirdParty/glfw-3.4)
add_subdirectory(ThirdParty/SDL-prerelease-3.1.1)
#add_subdirectory(ThirdParty/imgui)

# Add source to this project's executable.
add_executable (DeltaEngine "DeltaEngine.cpp" "DeltaEngine.h")

#add_library(sdl3 )
include(ThirdParty/imgui.cmake)

target_link_libraries(DeltaEngine PRIVATE glfw)
target_link_libraries(DeltaEngine PRIVATE SDL3::SDL3)
target_link_libraries(DeltaEngine PRIVATE imgui)
target_link_libraries(DeltaEngine PRIVATE fmt::fmt)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET DeltaEngine PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
