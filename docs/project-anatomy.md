# Project Anatomy

There are three main components to this project:

1. `client/pseudoregalia`: the Unreal project for the BP mod

1. `client/PseudoregaliaMultiplayerMod`: the C++ project for the UE4SS mod

1. `server`: the server, written in Rust

The BP mod contains blueprints for the ghost and manager actors. The manager collects player data each frame and handles updating the ghosts. The BP mod is loaded with UE4SS' BPModLoaderMod.

The UE4SS mod handles communication with both the BP mod and the server. It keeps track of the state of the ghosts and communicates that state to the manager by hooking into one of its functions.

The server contains the state for all connected players. It receives updates from clients containing their current state and updates clients with the state of other connected players. See the [application protocol](./application-protocol.md) for more information.
