# Build Instructions

## BP Mod

To build the `.pak` file for the BP mod, I just cook the assets from the editor and package them manually with [repak](https://github.com/trumank/repak). (Cooked assets go to `client/pseudoregalia/Saved/Cooked/Windows/pseudoregalia/Content/Mods/PseudoregaliaMultiplayerMod`.) It probably wouldn't be too hard to set up chunking just for the assets in the `Content/Mods/PseudoregaliaMultiplayerMod` folder, though.

## C++ Mod

TODO

```
msbuild PseudoregaliaMultiplayerMod.sln -p:Configuration=Game__Shipping__Win64 -p:Platform=x64
```

## Server

The server is written in Rust, so just building a Rust executable like normal is all you need:

```
cargo build --release
```

This will output the executable to `server/target/release`. To run the server with cargo, use

```
cargo run --release
```

If you need to provide an address override, you can pass arguments to the server like this:

```
cargo run --release -- 0.0.0.0:23432
```
