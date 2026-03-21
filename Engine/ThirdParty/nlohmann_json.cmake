message(STATUS "====== Delta Engine ====== Configuring nlohmann json ======")

add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json SYSTEM INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

add_library(ThirdParty::nlohmann_json ALIAS nlohmann_json)

set_target_properties(nlohmann_json PROPERTIES FOLDER ${third_party_folder})
