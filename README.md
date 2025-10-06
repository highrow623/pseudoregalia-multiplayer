# Pseudoregalia Multiplayer

This is a mod for [Pseudoregalia](https://store.steampowered.com/app/2365810/Pseudoregalia/) that enables online multiplayer play. Clients connect to a central server and state is shared between the clients.

Currently, all the mod does is sync player transforms (location/rotation), so you can see people moving around in real time. Other players show up as ghosts, and players can configure what color their ghost is. Animations are NOT synced. Zone syncing works based on the name of the zone, so it already works with modded maps. More features coming soon..?

The mod uses UE4SS and was built to be compatible with [`pseudoregalia-archipelago`](https://github.com/qwint/pseudoregalia-archipelago). This mod is independent though; you don't need to use `pseudoregalia-archipelago` to use this mod. Note that this mod may not be compatible with other UE4SS mods because of the version of UE4SS it uses. It should work fine with all mods that don't use UE4SS, notably Pseudo Menu Mod/custom maps and Attire UI Overhaul Mod/custom outfits.

Check out [installing the mod](./installing-the-mod.md) or [running the server](./running-the-server.md) to get started.
