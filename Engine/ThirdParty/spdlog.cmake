message(STATUS "====== Delta Engine ====== Configuring Spdlog ======")

set(SPDLOG_BUILD_SHARED ON)
set(SPDLOG_SYSTEM_INCLUDES ON)
set(SPDLOG_USE_STD_FORMAT ON)

add_subdirectory(spdlog)

set_target_properties(spdlog PROPERTIES FOLDER ${third_party_folder})

add_library(ThirdParty::spdlog ALIAS spdlog)
