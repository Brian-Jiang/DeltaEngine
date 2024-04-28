set(third_party_folder "ThirdParty")

add_subdirectory(glfw)
add_subdirectory(sdl3)

include(imgui.cmake)
set_target_properties(imgui PROPERTIES FOLDER ${third_party_folder}/imgui)
