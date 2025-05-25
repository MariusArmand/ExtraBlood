# ExtraBlood
Fork of the Reverse-engineered port of the Build game Blood [[NBlood](https://github.com/NBlood/NBlood)] using EDuke32 engine technology and development principles.

### Distinctive features
#### Gore
- More and longer lasting gore
- Decals and blood fx on ceilings
- Decals on floors
- Blood fx on floors follow slope
- Squish gibs by running through them
- Leave bloody footsteps after running through gibs
#### Dual wielding
- Dual wield flarepistols, shotguns, tommyguns and teslacannons (toggled by configurable keyboard key)
- Cultists have an increased chance to drop their weapons to accommodate the dual wield mechanic (Green cultists drop flarepistols and flares)
- Akimbo power-up is replaced by Super Armor
#### Misc
- Configurable keyboard key to Quick Restart the current level

***

### To Do
- Ideas are welcome

***

### Installing
1. Extract ExtraBlood to a new directory
2. Copy the following files from Blood 1.21 to ExtraBlood folder:
   * BLOOD.INI
   * BLOOD.RFF
   * BLOOD000.DEM, ..., BLOOD003.DEM (optional)
   * CP01.MAP, ..., CP09.MAP (optional, Cryptic Passage)
   * CPART07.AR_ (optional, Cryptic Passage)
   * CPART15.AR_ (optional, Cryptic Passage)
   * CPBB01.MAP, ..., CPBB04.MAP (optional, Cryptic Passage)
   * CPSL.MAP (optional, Cryptic Passage)
   * CRYPTIC.INI (optional, Cryptic Passage)
   * CRYPTIC.SMK (optional, Cryptic Passage)
   * CRYPTIC.WAV (optional, Cryptic Passage)
   * GUI.RFF
   * SOUNDS.RFF
   * SURFACE.DAT
   * TILES000.ART, ..., TILES017.ART
   * VOXEL.DAT

3. Optionally, if you want to use CD audio tracks instead of MIDI, provide FLAC/OGG recordings in following format: bloodXX.flac/ogg, where XX is track number. Make sure to enable Redbook audio option in sound menu.
4. Optionally, if you want cutscenes and you have the original CD, copy the `movie` folder into ExtraBlood's folder (the folder itself too).
If you have the GOG version of the game, do the following:
   * make a copy of `game.ins` (or `game.inst`) named `game.cue`
   * mount the `.cue` as a virtual CD (for example with `WinCDEmu`)
   * copy the `movie` folder from the mounted CD into ExtraBlood's folder
5. Launch ExtraBlood (on Linux, to play Cryptic Passage, launch with the `-ini CRYPTIC.INI` parameter)

### Notes
ExtraBlood now uses extrablood_cvars.cfg instead of settings.cfg. Please rename your settings.cfg if you need to retain settings from a previous version.

## Building from source
See: https://wiki.eduke32.com/wiki/Main_Page

## Acknowledgments
  See AUTHORS.md

## License
ExtraBlood is licensed under the GNU General Public License version 2 (GPLv2), as inherited from NBlood.
