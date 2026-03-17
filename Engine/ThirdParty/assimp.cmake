message(STATUS "====== Delta Engine ====== Configuring Assimp ======")

SET(BUILD_SHARED_LIBS ON)
SET(ASSIMP_BUILD_TESTS OFF)
SET(ASSIMP_INSTALL OFF)
SET(ASSIMP_INSTALL_PDB OFF)
SET(ASSIMP_BUILD_ASSIMP_VIEW OFF)
# SET(ASSIMP_DOUBLE_PRECISION ON)
# SET(ASSIMP_BUILD_USD_IMPORTER ON)

add_subdirectory(assimp)

set_target_properties(assimp PROPERTIES FOLDER ${third_party_folder})

set(THIRD_PARTY_INCLUDES ${THIRD_PARTY_INCLUDES} "${CMAKE_CURRENT_SOURCE_DIR}/assimp/include" CACHE INTERNAL "Third party include directories")
