//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------
#pragma once
#include "build.h"
#include "common_game.h"

enum FX_ID {
    FX_NONE = -1,
    FX_0 = 0,
    FX_1,
    FX_2,
    FX_3,
    FX_4,
    FX_5,
    FX_6,
    FX_7,
    FX_8,
    FX_9,
    FX_10,
    FX_11,
    FX_12,
    FX_13,
    FX_14,
    FX_15,
    FX_16,
    FX_17,
    FX_18,
    FX_19,
    FX_20,
    FX_21,
    FX_22,
    FX_23,
    FX_24,
    FX_25,
    FX_26,
    FX_27,
    FX_28,
    FX_29,
    FX_30,
    FX_31,
    FX_32,
    FX_33,
    FX_34,
    FX_35,
    FX_36,
    FX_37,
    FX_38,
    FX_39,
    FX_40,
    FX_41,
    FX_42,
    FX_43,
    FX_44,
    FX_45,
    FX_46,
    FX_47,
    FX_48,
    FX_49,
    FX_50,
    FX_51,
    FX_52,
    FX_53,
    FX_54,
    FX_55,
    FX_56,
    // marius
    FX_57, 
    FX_58,
    FX_59,
    FX_60,
    FX_61,
    // end marius
    kFXMax
};

// marius
// footprints
enum
{
    kFootprintBlood = 1902,
    kFootprintWater = 1903,
    kFootprintDirt = 1904,
    kFootprintAcid = 1906,
};
// end marius

class CFX {
public:
    void fxKill(int);
    void fxFree(int);
    spritetype * fxSpawn(FX_ID, int, int, int, int, unsigned int duration = 0);
    void fxProcess(void);
};

void fxSpawnCeiling(FX_ID nFx, int nSector, int x, int y, int z, int angle = 0); // marius, ceiling fx
void fxSpawnFloor(FX_ID nFx, int nSector, int x, int y, int z, int angle = 0); // marius, floor fx
int fxSpawnFootprint(FX_ID nFx, int nFootprintPicnum, int nSector, int x, int y, int z, int angle = 0); // marius, footprints
void fxSpawnBlood(spritetype *pSprite, int a2);
void fxSpawnPodBlood(spritetype *pSprite, int a2);
void fxSpawnEjectingBrass(spritetype *pSprite, int z, int a3, int a4);
void fxSpawnEjectingShell(spritetype *pSprite, int z, int a3, int a4);

void fxPrecache(void);

extern CFX gFX;
