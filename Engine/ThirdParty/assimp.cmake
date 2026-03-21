message(STATUS "====== Delta Engine ====== Configuring Assimp ======")

SET(BUILD_SHARED_LIBS ON)
SET(ASSIMP_BUILD_TESTS OFF)
SET(ASSIMP_INSTALL OFF)
SET(ASSIMP_INSTALL_PDB OFF)
SET(ASSIMP_BUILD_ASSIMP_VIEW OFF)
# SET(ASSIMP_DOUBLE_PRECISION ON)
# SET(ASSIMP_BUILD_USD_IMPORTER ON)
set(ASSIMP_WARNINGS_AS_ERRORS OFF)

add_subdirectory(assimp)

set_target_properties(assimp PROPERTIES FOLDER ${third_party_folder})

add_library(ThirdParty::assimp ALIAS assimp)
