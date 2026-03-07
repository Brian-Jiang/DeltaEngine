message(STATUS "====== Delta Engine ====== Configuring nlohmann json ======")

set(nlohmann_SOURCE_DIR_ ${CMAKE_CURRENT_SOURCE_DIR}/nlohmann)

add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

set_target_properties(nlohmann_json PROPERTIES FOLDER ${third_party_folder})