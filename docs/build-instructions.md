# Build Instructions

## BP Mod

To build the `.pak` file for the BP mod, I just cook the assets from the editor and package them manually with [repak](https://github.com/trumank/repak). (Cooked assets go to `client/pseudoregalia/Saved/Cooked/Windows/pseudoregalia/Content/Mods/PseudoregaliaMultiplayerMod`.) It probably wouldn't be too hard to set up chunking just for the assets in the `Content/Mods/PseudoregaliaMultiplayerMod` folder, though.

## C++ Mod

### Prerequisites

1. Link your GitHub account to your Epic Games account. This is required in order to gain access to the Unreal Engine source code used in UE4SS.

1. Add an [SSH key](https://docs.github.com/en/authentication/connecting-to-github-with-ssh) to your GitHub account for authentication. This is required in order to clone said Unreal Engine source code.

1. Install [Visual Studio 2022](https://learn.microsoft.com/en-us/visualstudio/releases/2022/release-history). You'll need the 17.14 Community version and may also need the 17.14 Build Tools.

1. Install [CMake](https://cmake.org/download/).

1. Clone the repo recursively:

    ```cmd
    > git clone https://github.com/highrow623/pseudoregalia-multiplayer.git --recursive
    ```

    If you already cloned the repo without the `--recursive` tag, you should be able to run this inside the repo:

    ```cmd
    pseudoregalia-multiplayer> git submodule update --init --recursive
    ```

### Building the Project

1. Navigate to `client` and build the project with CMake:

    ```cmd
    client> cmake -S . -B Output
    ```

    The solution file will be built to `client/Output/client.sln`.
    
1. Open `client.sln` in Visual Studio.

1. Right click `PseudoregaliaMultiplayerMod` in the Solution Explorer and select Properties.

1. Click the Configuration Manager button and set "Active solution configuration" to `Game__Shipping__Win64`.

1. Click Close, then OK.

1. Right click `PseudoregaliaMultiplayerMod` in the Solution Explorer and select Build.

    PseudoregaliaMultiplayerMod.dll will be written to `client/Output/PseudoregaliaMultiplayerMod/Game__Shipping__Win64`. Rename the file to `main.dll` and replace `pseudoregalia/Binaries/Win64/ue4ss/Mods/PseudoregaliaMultiplayerMod/dlls/main.dll` in your Pseudoregalia game to use/test it.

## Server

The server is written in Rust, so just building a Rust executable like normal is all you need:

```cmd
server> cargo build --release
```

This will output the executable to `server/target/release`. To run the server with cargo, use

```cmd
server> cargo run --release
```

If you need to provide an address override, you can pass arguments to the server like this:

```cmd
server> cargo run --release -- 0.0.0.0:23432
```
