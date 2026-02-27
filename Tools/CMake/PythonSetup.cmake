set(TOOLS_DIR "${CMAKE_SOURCE_DIR}/Tools")
set(DELTA_PYTHON "${TOOLS_DIR}/Python/python.exe")

if(NOT EXISTS "${DELTA_PYTHON}")
    message(FATAL_ERROR
        "Bundled Python not found at ${DELTA_PYTHON}\n"
        "Run Tools/Scripts/setup_tools.py with your system Python first.")
endif()
