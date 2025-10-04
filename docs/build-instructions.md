# Build Instructions

## BP Mod

To build the `.pak` file for the BP mod, I just cook the assets from the editor and package them manually with [repak](https://github.com/trumank/repak). (Cooked assets go to `client/pseudoregalia/Saved/Cooked/Windows/pseudoregalia/Content/Mods/PseudoregaliaMultiplayerMod`.) It probably wouldn't be too hard to set up chunking just for the assets in the `Content/Mods/PseudoregaliaMultiplayerMod` folder, though.

## C++ Mod

### Prerequisites

1. Link your GitHub account to your Epic Games account. This is required in order to gain access to the Unreal Engine source code used in UE4SS.

1. Add an [SSH key](https://docs.github.com/en/authentication/connecting-to-github-with-ssh) to your GitHub account for authentication. This is required in order to clone said Unreal Engine source code.

1. Install [Visual Studio 2022](https://learn.microsoft.com/en-us/visualstudio/releases/2022/release-history). In order to build, you will need version 17.10 Build Tools, but you can also download the Community version to makes edits.

1. Install [CMake](https://cmake.org/download/).

1. Clone the repo recursively:

    ```
    > git clone https://github.com/highrow623/pseudoregalia-multiplayer.git --recursive
    ```

    If you already cloned the repo without the `--recursive` tag, you should be able to run this inside the repo:

    ```
    pseudoregalia-multiplayer> git submodule update --init --recursive
    ```

1. Open `client/RE-UE4SS/deps/third/CMakeLists.txt` and change the `GIT_TAG` for `ImGuiTextEdit` on line 29 from `master` to `v1.1.0`.

    *Note: An update was made to ImGuiColorTextEdit in v1.2.0 that makes it incompatible with the version of UE4SS that the mod uses. The dependency was never tagged in this file, so without this change CMake would always grab latest. We have to lock it to a working tag manually. This will make the submodule "dirty" but git won't let you commit it anyway.*

### Building the Project

1. Navigate inside the repo and build the project with CMake:

    ```
    pseudoregalia-multiplayer> cmake -S . -B Output
    ```

    The solution file will be built to `client/Output/client.sln`. You can open the solution in Visual Studio to make edits, but you will build with the build tools.

1. Launch Visual Studio Build Tools 17.10 from the Visual Studio Installer. This will open a new console.

1. In the build tools console, navigate to the PseudoregaliaMultiplayerMod project at `client/Output/PseudoregaliaMultiplayerMod`.

1. Build with MSBuild:

    ```
    PseudoregaliaMultiplayerMod> msbuild PseudoregaliaMultiplayerMod.sln -p:Configuration=Game__Shipping__Win64 -p:Platform=x64
    ```

    PseudoregaliaMultiplayerMod.dll will be written to `client/Output/PseudoregaliaMultiplayerMod/Game__Shipping__Win64`. Rename the file to `main.dll` and replace `pseudoregalia/Binaries/Win64/Mods/PseudoregaliaMultiplayerMod/dlls/main.dll` in your Pseudoregalia game to use/test it.

## Server

The server is written in Rust, so just building a Rust executable like normal is all you need:

```
server> cargo build --release
```

This will output the executable to `server/target/release`. To run the server with cargo, use

```
server> cargo run --release
```

If you need to provide an address override, you can pass arguments to the server like this:

```
server> cargo run --release -- 0.0.0.0:23432
```
