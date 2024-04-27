set(third_party_folder "ThirdParty")

add_subdirectory(glfw-3.4)
add_subdirectory(SDL-prerelease-3.1.1)

include(imgui.cmake)
set_target_properties(imgui PROPERTIES FOLDER ${third_party_folder}/imgui)
