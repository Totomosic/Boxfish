![Build-Windows](https://github.com/Totomosic/Boxfish/workflows/Build-Windows/badge.svg)
![Build-Linux](https://github.com/Totomosic/Boxfish/workflows/Build-Linux/badge.svg)

# Boxfish
C++ UCI Chess engine inspired by [Stockfish](https://stockfishchess.org/).
Example of the engine running [here](https://totomosic.github.io). (built to WebAssembly with Emscripten)

## Features
- Bitboards and magic bitboard move generation
- UCI protocol
- Search:
  - Transposition table with Zobrist hashing
  - PVS search
  - Aspiration windows and iterative deepening
  - Eval pruning
  - Razoring
  - Adaptive null move pruning
  - Futility pruning
  - Internal iterative deepening
  - Singular extension
  - Late move reduction
  - Quiescence search with delta pruning
- Evaluation:
  - Material
  - Piece squares
  - Blocked pieces
  - Passed pawns
  - Weak pawns
  - King safety
  - Space
  - Knights, Bishops, Rooks, Queens
- All perft tests passed
- Pondering
- SEE move ordering

## Installing:
1. Download or clone this repository.
2. If on windows run the `Scripts/Win-GenProjects.bat` script to generate the Visual Studio 2019 project and solution files.

## Building on Windows:
1. Run `Scripts/Win-GenProjects.bat` and build the solution using Visual Studio 2019.
2. Build outputs are located in the `bin` directory.

## Building on Linux:
1. Run `Scripts/Linux-GenProjects.sh` to generate the Makefiles.
2. Run `make -j<number_of_cores> Boxfish-Cli` to build Boxfish.
3. Build outputs are located in the `bin` directory.
