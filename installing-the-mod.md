# Installing the Mod

`pseudoregalia-multiplayer` is built to work with an old version of UE4SS, so your installation steps will be different depending on if you already have UE4SS installed. It was built to work specifically with the version that `pseudoregalia-archipelago` uses so that it'd be compatible with that mod. You don't need that mod installed to use this one.

Installation steps:

* If you have `pseudoregalia-archipelago` installed, just follow the steps in the [installing into `pseudoregalia-archipelago`](#installing-into-pseudoregalia-archipelago) section.
* If you have UE4SS installed but not `pseudoregalia-archipelago`, start with the [cleaning up UE4SS](#cleaning-up-ue4ss) section, then read the [installing fresh](#installing-fresh) section.
* If you don't have UE4SS installed, just follow the steps in the [installing fresh](#installing-fresh) section.

After the mod is installed, check out the [configuring the client](#configuring-the-client) section to set the server information. The mod will automatically connect to the server when loading into a save file.

If you want to uninstall, check out the [uninstalling the mod](#uninstalling-the-mod) section.

## Installing Into `pseudoregalia-archipelago`

If you have `pseudoregalia-archipelago` installed, then you already have the version of UE4SS that this mod is compatible with.

1. Download `pseudoregalia-multiplayer.zip` from the [Releases](https://github.com/highrow623/pseudoregalia-multiplayer/releases) page. This just contains the mod files for the mod.

1. Extract the zip directly into the `Pseudoregalia` folder. If you did it correctly, you should see a file in this location: `Pseudoregalia/pseudoregalia/Content/Paks/LogicMods/PseudoregaliaMultiplayerMod.pak`.

1. Open `Pseudoregalia/pseudoregalia/Binaries/Win64/Mods/mods.txt` and add a new line with this text:

   ```
   PseudoregaliaMultiplayerMod : 1
   ```

## Cleaning Up UE4SS

`pseudoregalia-multiplayer` was built for a specific, older version of UE4SS, so it might not be compatible with the version you have installed. The safest way to make sure the mod will work is to create a new folder and install the mod fresh.

1. Go to Steam, right click the Pseudoregalia game, and navigate to Manage > Browse local files. This will put you *into* the `Pseudoregalia` folder.

1. Navigate up one level, then make a copy of the `Pseudoregalia` folder. This will be your original install that still has all your current mods, so it can be helpful to name it something to reflect that.

1. Uninstall Pseudoregalia on Steam. This will clear all the vanilla files out of the `Pseudoregalia` folder, leaving only the modded files.

1. Delete the `Pseudoregalia` folder.

1. Reinstall Pseudoregalia. The `Pseudoregalia` folder now only contains a completely fresh, vanilla version of the game.

## Installing Fresh

1. [Optional] It's recommended that you copy your `Pseudoregalia` folder before you install. This is because UE4SS comes with a lot of files, and if you don't want it anymore, cleaning it up manually can be a hassle. You can choose to install the mod into either folder.

    1. Go to Steam, right click the Pseudoregalia game, and navigate to Manage > Browse local files. This will put you *into* the `Pseudoregalia` folder.

    1. Navigate up one level, then make a copy of the `Pseudoregalia` folder.

1. Download `pseudoregalia-multiplayer-ue4ss.zip` from the [Releases](https://github.com/highrow623/pseudoregalia-multiplayer/releases) page. This contains the mod files for the mod as well as a compatible version of UE4SS bundled together.

1. Extract the zip directly into the `Pseudoregalia` folder. If you did it correctly, you should see a file in this location: `Pseudoregalia/pseudoregalia/Content/Paks/LogicMods/PseudoregaliaMultiplayerMod.pak`.

## Configuring the Client

Releases include a template settings file at `Pseudoregalia/pseudoregalia/Binaries/Win64/Mods/PseudoregaliaMultiplayerMod/settings.tmpl.toml`. Rename it to `settings.toml` for the settings to actually be used. *Note: the settings file is in a template in releases so that when you update the mod, it won't overwrite your current settings. You can check the template file after installing a new release to see if there is anything new.*

The settings file has these options:

| Option | Type | Description |
| --- | --- | --- |
| `address` | string | The address of the server to connect to. Can be an IP address or a domain. |
| `port` | string | The port number the server is running on. |

The settings file is only read when you start Pseudoregalia. If you need to change them, you'll need to quit out and restart the game for the new settings to take effect.

## Uninstalling the Mod

If you want to turn off the mod without removing UE4SS, do the following:

1. Remove `Pseudoregalia/pseudoregalia/Content/Paks/LogicMods/PseudoregaliaMultiplayerMod.pak`. You may want to copy it to another location in case you decide to reinstall later.

1. Open `Pseudoregalia/pseudoregalia/Binaries/Win64/Mods/mods.txt` and change the number after `PseudoregaliaMultiplayerMod` from `1` to `0`.

To uninstall the mod and UE4SS, see the [cleaning up UE4SS](#cleaning-up-ue4ss) section. If you copied the `Pseudoregalia` folder specifically for this mod, you can just delete the folder.
