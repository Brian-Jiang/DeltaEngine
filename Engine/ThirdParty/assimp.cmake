
add_subdirectory(assimp)

set_target_properties(assimp PROPERTIES FOLDER ${third_party_folder})
set_target_properties(UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTIES FOLDER ${third_party_folder})