include_guard(GLOBAL)

include("${CMAKE_SOURCE_DIR}/Tools/CMake/PythonSetup.cmake")

set(DELTA_SYNC_DIRECTORY_SCRIPT "${CMAKE_SOURCE_DIR}/Tools/DeltaBuildTool/sync_directory.py")

function(delta_add_sync_directory_target target_name source_path dest_path)
    set(_stamp "${CMAKE_BINARY_DIR}/Intermediate/SyncStamps/${target_name}.stamp")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/Intermediate/SyncStamps")

    if(IS_DIRECTORY "${source_path}")
        file(GLOB_RECURSE _sync_sources CONFIGURE_DEPENDS
            LIST_DIRECTORIES false
            "${source_path}/*")
    else()
        set(_sync_sources "${source_path}")
    endif()

    add_custom_command(
        OUTPUT "${_stamp}"
        COMMAND ${DELTA_PYTHON} "${DELTA_SYNC_DIRECTORY_SCRIPT}"
            --source "${source_path}"
            --dest "${dest_path}"
            --stamp "${_stamp}"
        DEPENDS ${_sync_sources} "${DELTA_SYNC_DIRECTORY_SCRIPT}"
        COMMENT "Syncing third-party binaries: ${target_name}"
        VERBATIM)

    add_custom_target(${target_name} DEPENDS "${_stamp}")
    set_target_properties(${target_name} PROPERTIES FOLDER ${utility_folder})
endfunction()
