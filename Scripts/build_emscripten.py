import os
import sys

WEB_DIRECTORY = "../Web"

CLI_DIRECTORY = "../Boxfish-Cli/src"
SOURCE_DIRECTORY = "../Boxfish-Lib/src"
SOURCE_FILES = [
    "Attacks.cpp",
    "Bitboard.cpp",
    "Book.cpp",
    "Boxfish.cpp",
    "Evaluation.cpp",
    "Format.cpp",
    "Logging.cpp",
    "MoveGenerator.cpp",
    "MoveSelector.cpp",
    "Position.cpp",
    "PositionUtils.cpp",
    "Random.cpp",
    "Rays.cpp",
    "Search.cpp",
    "TranspositionTable.cpp",
    "ZobristHash.cpp",
    "Emscripten.cpp",
]

CLI_FILES = []

EXPORTED_FUNCTIONS = [
    "Uci"
]

if __name__ == "__main__":
    os.makedirs(WEB_DIRECTORY, exist_ok=True)
    current_directory = os.getcwd()

    os.chdir(WEB_DIRECTORY)

    command_line = "em++ --bind -o Boxfish.js -std=c++17 -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1 -s WASM=1 -O3 -DEMSCRIPTEN -DBOX_DIST -DBOX_PLATFORM_WINDOWS -s EXPORTED_RUNTIME_METHODS=[\"ccall\",\"cwrap\"]"# -s EXPORTED_FUNCTIONS=[".format(SOURCE_DIRECTORY)
    #for fn in EXPORTED_FUNCTIONS:
    #    command_line += "\"{}\",".format(fn)
    #command_line = command_line[:len(command_line) - 1]
    #command_line += "]"

    for source_file in SOURCE_FILES:
        command_line += " {}".format(os.path.join(SOURCE_DIRECTORY, source_file))
    for source_file in CLI_FILES:
        command_line += " {}".format(os.path.join(CLI_DIRECTORY, source_file))

    os.system(command_line)

    os.chdir(current_directory)
