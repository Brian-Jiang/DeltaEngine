message(STATUS "====== Delta Engine ====== Configuring Google Test ======")

set(INSTALL_GTEST OFF)

add_subdirectory(googletest)

foreach(target gtest gtest_main gmock gmock_main)
    if(TARGET ${target})
        target_compile_options(${target} PRIVATE
            "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/WX->"
        )
    endif()
endforeach()

set_target_properties(gtest gtest_main gmock gmock_main PROPERTIES FOLDER ${third_party_folder})

add_library(ThirdParty::gtest ALIAS gtest)
add_library(ThirdParty::gtest_main ALIAS gtest_main)
add_library(ThirdParty::gmock ALIAS gmock)
add_library(ThirdParty::gmock_main ALIAS gmock_main)
