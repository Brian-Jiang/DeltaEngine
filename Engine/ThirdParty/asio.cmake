message(STATUS "====== Delta Engine ====== Configuring asio ======")

add_library(asio INTERFACE)
target_include_directories(asio SYSTEM INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/asio/include>)
target_compile_definitions(asio INTERFACE ASIO_STANDALONE _WIN32_WINNT=0x0A00)

add_library(ThirdParty::asio ALIAS asio)

set_target_properties(asio PROPERTIES FOLDER ${third_party_folder})
