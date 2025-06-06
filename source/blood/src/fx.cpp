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

#include "build.h"
#include "common_game.h"

#include "actor.h"
#include "blood.h"
#include "callback.h"
#include "config.h"
#include "db.h"
#include "eventq.h"
#include "fx.h"
#include "gameutil.h"
#include "levels.h"
#include "seq.h"
#include "tile.h"
#include "trig.h"
#include "view.h"

CFX gFX;

struct FXDATA {
    CALLBACK_ID funcID;
    char detail;
    short seq;
    short flags;
    int gravity;
    int airdrag;
    int duration;
    short picnum;
    unsigned char xrepeat;
    unsigned char yrepeat;
    short cstat;
    signed char shade;
    char pal;
};

FXDATA gFXData[] = {
    { kCallbackNone, 0, 49, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 0, 50, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 0, 51, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 0, 52, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 0, 7, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 0, 44, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 0, 45, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 0, 46, 1, -128, 8192, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 2, 6, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 2, 42, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 2, 43, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 1, 48, 3, -256, 8192, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 1, 60, 3, -256, 8192, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackFXBloodBitsShort, 2, 0, 1, 46603, 2048, 480, 2154, 40, 40, 0, -12, 0 }, // blood chunk
    { kCallbackNone, 2, 0, 3, 46603, 5120, 480, 2269, 24, 24, 0, -128, 0 },
    { kCallbackNone, 2, 0, 3, 46603, 5120, 480, 1720, 24, 24, 0, -128, 0 },
    { kCallbackNone, 1, 0, 1, 58254, 3072, 480, 2280, 48, 48, 0, -128, 0 },
    { kCallbackNone, 1, 0, 1, 58254, 3072, 480, 3135, 48, 48, 0, -128, 0 },
    { kCallbackNone, 0, 0, 3, 58254, 1024, 480, 3261, 32, 32, 0, 0, 0 },
    { kCallbackNone, 1, 0, 3, 58254, 1024, 480, 3265, 32, 32, 0, 0, 0 },
    { kCallbackNone, 1, 0, 3, 58254, 1024, 480, 3269, 32, 32, 0, 0, 0 },
    { kCallbackNone, 1, 0, 3, 58254, 1024, 480, 3273, 32, 32, 0, 0, 0 },
    { kCallbackNone, 1, 0, 3, 58254, 1024, 480, 3277, 32, 32, 0, 0, 0 },
    { kCallbackNone, 2, 0, 1, -27962, 8192, 600, 1128, 16, 16, 514, -16, 0 }, // bubble 1
    { kCallbackNone, 2, 0, 1, -18641, 8192, 600, 1128, 12, 12, 514, -16, 0 }, // bubble 2
    { kCallbackNone, 2, 0, 1, -9320, 8192, 600, 1128, 8, 8, 514, -16, 0 }, // bubble 3
    { kCallbackNone, 2, 0, 1, -18641, 8192, 600, 1131, 32, 32, 514, -16, 0 },
    { kCallbackFXBloodBits, 2, 0, 3, 27962, 4096, 480, 733, 32, 32, 0, -16, 0 }, // blood spray
    { kCallbackNone, 1, 0, 3, 18641, 4096, 120, 2261, 12, 12, 0, -128, 0 },
    { kCallbackNone, 0, 47, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 1, 0, 3, 58254, 3328, 480, 2185, 48, 48, 0, 0, 0 },
    { kCallbackNone, 0, 0, 3, 58254, 1024, 480, 2620, 48, 48, 0, 0, 0 },
    { kCallbackNone, 1, 55, 1, -13981, 5120, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 1, 56, 1, -13981, 5120, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 1, 57, 1, 0, 2048, 0, 0, 0, 0, 0, 0, 0 }, // big wall blood splat
    { kCallbackNone, 1, 58, 1, 0, 2048, 0, 0, 0, 0, 0, 0, 0 }, // small wall blood splat
    { kCallbackNone, 2, 0, 0, 0, 0, 960, 956, 32, 32, 610, 0, 0 }, // floor blood splat
    { kCallbackFXBouncingSleeve, 2, 62, 0, 46603, 1024, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackFXBouncingSleeve, 2, 63, 0, 46603, 1024, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackFXBouncingSleeve, 2, 64, 0, 46603, 1024, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackFXBouncingSleeve, 2, 65, 0, 46603, 1024, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackFXBouncingSleeve, 2, 66, 0, 46603, 1024, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackFXBouncingSleeve, 2, 67, 0, 46603, 1024, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 1, 0, 3, 0, 0, 0, 838, 16, 16, 80, -8, 0 }, // wall bullet decal
    { kCallbackNone, 0, 0, 3, 34952, 8192, 0, 2078, 64, 64, 0, -8, 0 },
    { kCallbackNone, 0, 0, 3, 34952, 8192, 0, 1106, 64, 64, 0, -8, 0 },
    { kCallbackNone, 0, 0, 3, 58254, 3328, 480, 2406, 48, 48, 0, 0, 0 },
    { kCallbackNone, 1, 0, 3, 46603, 4096, 480, 3511, 64, 64, 0, -128, 0 },
    { kCallbackNone, 0, 8, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 2, 11, 3, -256, 8192, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 2, 11, 3, 0, 8192, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 1, 30, 3, 0, 0, 0, 0, 40, 40, 80, -8, 0 },
    { kCallbackFXPodBloodSplat, 2, 0, 3, 27962, 4096, 480, 4023, 32, 32, 0, -16, 0 },
    { kCallbackFXPodBloodSplat, 2, 0, 3, 27962, 4096, 480, 4028, 32, 32, 0, -16, 0 },
    { kCallbackNone, 2, 0, 0, 0, 0, 480, 926, 32, 32, 610, -12, 0 },
    { kCallbackNone, 1, 70, 1, -13981, 5120, 0, 0, 0, 0, 0, 0, 0 },
    { kCallbackNone, 2, 0, 0, 0, 0, 960, 956, 32, 32, 610, 0, 0 }, // marius, ceiling fx, ceiling blood splat
    { kCallbackNone, 1, 0, 3, 0, 0, 0, 838, 16, 16, 80, -8, 0 }, // marius, ceiling fx, ceiling bullet decal
    { kCallbackNone, 1, 0, 3, 0, 0, 0, 838, 16, 16, 80, -8, 0 }, // marius, floor fx, floor bullet decal
    { kCallbackNone, 2, 0, 0, 0, 0, 960, 0, 32, 32, 610, 0, 0 }, // marius, footprints
    { kCallbackNone, 2, 0, 0, 0, 0, 960, 956, 32, 32, 610, 0, 0 }, // marius, short floor blood splat
};

void CFX::fxKill(int nSprite)
{
    if (nSprite < 0 || nSprite >= kMaxSprites)
        return;
    evKill(nSprite, 3);
    if (sprite[nSprite].extra > 0)
        seqKill(3, sprite[nSprite].extra);
    DeleteSprite(nSprite);
}

void CFX::fxFree(int nSprite)
{
    if (nSprite < 0 || nSprite >= kMaxSprites)
        return;
    spritetype *pSprite = &sprite[nSprite];
    if (pSprite->extra > 0)
        seqKill(3, pSprite->extra);
    if (pSprite->statnum != kStatFree)
        actPostSprite(nSprite, kStatFree);
}

spritetype * CFX::fxSpawn(FX_ID nFx, int nSector, int x, int y, int z, unsigned int duration)
{
    if (nSector < 0 || nSector >= numsectors)
        return NULL;
    int nSector2 = nSector;
    if (!FindSector(x, y, z, &nSector2))
        return NULL;
    if (gbAdultContent && gGameOptions.nGameType == kGameTypeSinglePlayer)
    {
        switch (nFx)
        {
        case FX_0:
        case FX_1:
        case FX_2:
        case FX_3:
        case FX_13:
        case FX_34:
        case FX_35:
        case FX_36:
            return NULL;
        default:
            break;
        }
    }
    if (nFx < 0 || nFx >= kFXMax)
        return NULL;
    FXDATA *pFX = &gFXData[nFx];
    // marius
    if (!VanillaMode()) // extrablood code
    {
        // increase duration of fx
        switch (nFx)
        {
        case FX_0:
        case FX_1:
        case FX_2:
        case FX_3:
        case FX_13:
        case FX_34: // big wall blood splat
            if (!duration) // no override duration given, load from global fx data struct
                duration = pFX->duration;
            duration *= 20;
            pFX->airdrag = 0;
            break;
        case FX_35: // small wall blood splat
            if (!duration)
                duration = pFX->duration;
            duration *= 10;
            pFX->airdrag = 0;
            break;
        case FX_36: // floor blood splat
        case FX_57: // ceiling blood splat
        case FX_60: // footprint
            if (!duration)
                duration = pFX->duration;
            duration *= 200;
            break;   
        case FX_39: // bullet casing
            if (!duration)
                duration = pFX->duration;
            duration *= 2;
            break;
        case FX_40: // shell casing
            if (!duration)
                duration = pFX->duration;
            duration *= 5;
            break;
        default:
            break;
        }        
    }
    // end marius

    // marius
    if (VanillaMode()) // original code
    {
        if (gStatCount[kStatFX] == 512)
        {
            int nSprite = headspritestat[kStatFX];
            while ((sprite[nSprite].flags & 32) && nSprite != -1) // scan through sprites for free slot
                nSprite = nextspritestat[nSprite];
            if (nSprite == -1)
                return NULL;
            fxKill(nSprite);
        }
    }
    else // extrablood code
    {
        // allow more fx before starting to remove them
        if (gStatCount[kStatFX] >= 3072)
        {
            int nSprite = headspritestat[kStatFX];
            while ((sprite[nSprite].flags & 32) && nSprite != -1) // scan through sprites for free slot
                nSprite = nextspritestat[nSprite];
            if (nSprite == -1)
                return NULL;
            fxKill(nSprite);
        }
    }
    // end marius
    spritetype *pSprite = actSpawnSprite(nSector, x, y, z, 1, 0);
    pSprite->type = nFx;
    pSprite->picnum = pFX->picnum;
    pSprite->cstat |= pFX->cstat;
    pSprite->shade = pFX->shade;
    pSprite->pal = pFX->pal;
    qsprite_filler[pSprite->index] = pFX->detail;
    if (pFX->xrepeat > 0)
        pSprite->xrepeat = pFX->xrepeat;
    if (pFX->yrepeat > 0)
        pSprite->yrepeat = pFX->yrepeat;
    if ((pFX->flags & 1) && Chance(0x8000))
        pSprite->cstat |= 4;
    if ((pFX->flags & 2) && Chance(0x8000))
        pSprite->cstat |= 8;
    if (pFX->seq)
    {
        int nXSprite = dbInsertXSprite(pSprite->index);
        seqSpawn(pFX->seq, 3, nXSprite, -1);
    }
    if (duration == 0) // no override duration set, load from global fx data struct
        duration = pFX->duration;
    if (duration)
        evPost((int)pSprite->index, 3, duration+Random2(duration>>1), kCallbackRemove);
    return pSprite;
}

void CFX::fxProcess(void)
{
    for (int nSprite = headspritestat[kStatFX]; nSprite >= 0; nSprite = nextspritestat[nSprite])
    {
        spritetype *pSprite = &sprite[nSprite];
        if (pSprite->statnum == kStatFree) // skip free'd fx sprite
            continue;
        viewBackupSpriteLoc(nSprite, pSprite);
        short nSector = pSprite->sectnum;
        dassert(nSector >= 0 && nSector < kMaxSectors);
        dassert(pSprite->type < kFXMax);
        FXDATA *pFXData = &gFXData[pSprite->type];
        // marius
        // embed/tweak NotBlood underwater gravity behaviour
        vec3_t oldPos = pSprite->xyz;
        int nGravity = pFXData->gravity;
        int nAirDrag = pFXData->airdrag;
        if (!VanillaMode()) // extrablood code
        {
            nAirDrag >>= 1; // make blood drag 
            if (IsUnderwaterSector(pSprite->sectnum) && pSprite->type == FX_27)
            {
                nGravity = 1250; // make blood lighter
            }
        }
        // end marius
        actAirDrag(pSprite, pFXData->airdrag);
        if (xvel[nSprite])
            pSprite->x += xvel[nSprite]>>12;
        if (yvel[nSprite])
            pSprite->y += yvel[nSprite]>>12;
        // marius
        // wall blood splats shouldn't sink away
        if (VanillaMode()) // original code
        {
            if (zvel[nSprite])
                pSprite->z += zvel[nSprite]>>8;
        }
        else // extrablood code
        {
            if (pSprite->type != FX_34 && pSprite->type != FX_35 && zvel[nSprite])
                pSprite->z += zvel[nSprite]>>8;
        }
        // embed NotBlood underwater gravity behavior
		const bool bCasingType = (pSprite->type >= FX_37) && (pSprite->type <= FX_42);
		if (!VanillaMode() && bCasingType && IsUnderwaterSector(pSprite->sectnum)) // lower gravity by 75% underwater (only for bullet casings)
			nGravity >>= 2;
        // embed NotBlood casing bounce back behavior
		if (!VanillaMode() && bCasingType) // check if new xy position is within a wall
		{
			if (!cansee(oldPos.x, oldPos.y, oldPos.z, pSprite->sectnum, pSprite->x, pSprite->y, oldPos.z, pSprite->sectnum)) // if new position has clipped into wall, invert velocity and continue
			{
				xvel[nSprite] = -((xvel[nSprite]>>1)+(xvel[nSprite]>>2)); // lower velocity by 75% and invert
				yvel[nSprite] = -((yvel[nSprite]>>1)+(yvel[nSprite]>>2));
				pSprite->x = oldPos.x + (xvel[nSprite]>>12);
				pSprite->y = oldPos.y + (yvel[nSprite]>>12);
			}
		}
        // embed NotBlood gib drag against wall behavior
        else if (!VanillaMode() && (pSprite->type == FX_27 || pSprite->type == FX_13)) // check if new xy position is within a wall
        {
            if ((xvel[nSprite] || yvel[nSprite]) && !cansee(oldPos.x, oldPos.y, oldPos.z, pSprite->sectnum, pSprite->x, pSprite->y, oldPos.z, pSprite->sectnum)) // if new position has clipped into wall, freeze xy position
            {
                pSprite->x = oldPos.x;
                pSprite->y = oldPos.y;
            }
        }
        // end marius	
        // Weird...
        if (xvel[nSprite] || (yvel[nSprite] && pSprite->z >= sector[pSprite->sectnum].floorz))
        {
            updatesector(pSprite->x, pSprite->y, &nSector);
            if (nSector == -1)
            {
                fxFree(nSprite);
                continue;
            }
            if (getflorzofslope(pSprite->sectnum, pSprite->x, pSprite->y) <= pSprite->z)
            {
                if (pFXData->funcID < 0 || pFXData->funcID >= kCallbackMax)
                {
                    fxFree(nSprite);
                    continue;
                }
                dassert(gCallback[pFXData->funcID] != NULL);
                gCallback[pFXData->funcID](nSprite);
                continue;
            }
            if (nSector != pSprite->sectnum)
            {
                dassert(nSector >= 0 && nSector < kMaxSectors);
                ChangeSpriteSect(nSprite, nSector);
            }
        }
        if (xvel[nSprite] || yvel[nSprite] || zvel[nSprite])
        {
            int32_t floorZ, ceilZ;
            getzsofslope(nSector, pSprite->x, pSprite->y, &ceilZ, &floorZ);
            if (ceilZ > pSprite->z && !(sector[nSector].ceilingstat&1))
            {
                // marius
                // ceiling fx
                if (!VanillaMode()) // extrablood code
                {
                    // spawn a blood splat on the ceiling where a bloodspurt hits the ceiling
                    if (pFXData->funcID == kCallbackFXBloodBits)
                    {
                        fxSpawnCeiling(FX_57, nSector, pSprite->x, pSprite->y, ceilZ, Random2(512));
                    }
                }
                //end marius
                fxFree(nSprite);
                continue;
            }
            if (floorZ < pSprite->z)
            {
                if (pFXData->funcID < 0 || pFXData->funcID >= kCallbackMax)
                {
                    fxFree(nSprite);
                    continue;
                }
                dassert(gCallback[pFXData->funcID] != NULL);
                gCallback[pFXData->funcID](nSprite);
                continue;
            }
        }
        // marius
        if (VanillaMode()) // original code
        {
            zvel[nSprite] += pFXData->gravity;
        }
        else // extrablood code
        {
            // use tweaked gravity
            zvel[nSprite] += nGravity;
        
            // reset z after Z-Motion Sprite sector move
            int32_t floorZ, ceilZ;
            getzsofslope(nSector, pSprite->x, pSprite->y, &ceilZ, &floorZ);
            switch (pSprite->type)
            {
            case FX_59: // floor bullet decal
                pSprite->z = floorZ - 3;
                break;
            case FX_36: // floor blood splat
            case FX_61: // short floor blood splat
                pSprite->z = floorZ - 6;
                break;
            case FX_60: // footprint
                pSprite->z = floorZ - 9;
                break;
            case FX_58: // ceiling bullet decal
                pSprite->z = ceilZ + 3;
                break;
            case FX_57: // ceiling blood splat
                pSprite->z = ceilZ + 6;
                break;
            }

            // kill floor/ceiling fx if it is no longer in the original sector (e.g. due to a slide marked sector such as the grave in e1m1) 
            if ((pSprite->type >= FX_57 && pSprite->type <= FX_61) && !SprInside(pSprite, nSector)) 
            {
                gFX.fxKill(pSprite->index); 
            }
        }
        // end marius
    }
}

// marius
// ceiling fx
void fxSpawnCeiling(FX_ID nFx, int nSector, int x, int y, int z, int angle)
{
    spritetype *pFX = gFX.fxSpawn(nFx, nSector, x, y, z);
    if (pFX)
    {
        pFX->ang = angle;

        // set initial alignment before spriteSetSlope
        pFX->cstat |= CSTAT_SPRITE_ALIGNMENT_SLOPE | CSTAT_SPRITE_YFLIP | 0x4000; // 0x4000 is ceiling move mask
        pFX->cstat &= ~CSTAT_SPRITE_ONE_SIDED; // disable one sided flag to ensure we can flip around and randomize look

        // apply ceiling slope                    
        if (sector[nSector].ceilingstat&2) // if it's a sloped ceiling
        { 
            // set ang based on slope direction
            int32_t slopeAngle;
            if (sector[nSector].ceilingheinum >= 0) // positive slope
            { 
                slopeAngle = (GetWallAngle(sector[nSector].wallptr) + kAng180 + kAng90) & kAngMask;
            } 
            else // negative slope
            {
                slopeAngle = (GetWallAngle(sector[nSector].wallptr) - kAng180 + kAng90) & kAngMask; 
            }
            pFX->ang = slopeAngle;

            spriteSetSlope(pFX->index, sector[nSector].ceilingheinum);
        }

        // chance to flip to randomize look
        if (Chance(0X8000))
        {
            pFX->cstat |= CSTAT_SPRITE_XFLIP;
        }
        if (Chance(0X8000))
        {
            pFX->cstat |= CSTAT_SPRITE_YFLIP;
        }

        // offset z based on fx type
        switch (nFx)
        {
        case FX_57: // ceiling blood splat
            pFX->z = z + 6;
            break;
        default:
            pFX->z = z + 3;
            break;            
        }

        // delete fx when it is:
        // - edging out of the sector
        // - in a lower ror sector
        if (!SprInside(pFX, nSector) || gLowerLink[nSector] > -1) 
        {
            gFX.fxKill(pFX->index); 
        }
        else
        {
            if (nFx == FX_57 && Chance(0xBFFF)) // 75% chance
            {
                // spawn blood drips on their own invisible dummy ceiling bloodsplat
                // this ensures kCallbackRemove won't remove the original ceiling bloodsplat
                spritetype *pDFX = gFX.fxSpawn(nFx, nSector, pFX->x, pFX->y, pFX->z);
                if (pDFX)
                {
                    pDFX->ang = pFX->ang;
                    pDFX->cstat = pFX->cstat | CSTAT_SPRITE_INVISIBLE;
                    int delay = Chance(0x8000) ? 85 : 150; // 50% chance for 85, 50% chance for 150
                    evPost(pDFX->index, 3, delay, kCallbackFXBloodSpurt);
                    if (delay == 85)
                    {
                        // when spawned with least delay, wait longer to stop dripping
                        evPost(pDFX->index, 3, delay + 10, kCallbackRemove);
                    }
                    else
                    {
                        // when spawned with more delay, wait less long to stop dripping
                        evPost(pDFX->index, 3, delay + 2, kCallbackRemove);
                    }
                }
            }
        }
    }
}

// floor fx
void fxSpawnFloor(FX_ID nFx, int nSector, int x, int y, int z, int angle)
{
    spritetype *pFX = gFX.fxSpawn(nFx, nSector, x, y, z);
    if (pFX)
    {
        pFX->ang = angle;

        // set initial alignment before spriteSetSlope
        pFX->cstat |= CSTAT_SPRITE_ALIGNMENT_SLOPE | CSTAT_SPRITE_YFLIP | 0x2000; // 0x2000 is floor move mask
        pFX->cstat &= ~CSTAT_SPRITE_ONE_SIDED; // disable one sided flag to ensure we can flip around and randomize look

        // apply floor slope                    
        if (sector[nSector].floorstat&2) // if it's a sloped floor
        { 
            // set ang based on slope direction
            int32_t slopeAngle;
            if (sector[nSector].floorheinum >= 0) // positive slope
            { 
                slopeAngle = (GetWallAngle(sector[nSector].wallptr) + kAng180 + kAng90) & kAngMask;
            } 
            else // negative slope
            {
                slopeAngle = (GetWallAngle(sector[nSector].wallptr) - kAng180 + kAng90) & kAngMask; 
            }
            pFX->ang = slopeAngle;

            spriteSetSlope(pFX->index, sector[nSector].floorheinum);
        }

        // chance to flip to randomize look
        if (Chance(0X8000))
        {
            pFX->cstat |= CSTAT_SPRITE_YFLIP;
        }
        if (Chance(0X8000))
        {
            pFX->cstat |= CSTAT_SPRITE_XFLIP;
        }

        // offset z based on fx type
        switch (nFx)
        {
        case FX_36: // floor blood splat
        case FX_61: // short floor blood splat
            pFX->z = z - 6;
            break;                
        case FX_60: // footprint
            pFX->z = z - 9;
            break;
        default:
            pFX->z = z - 3;
            break;
        }

        // delete fx when it is:
        // - edging out of the sector
        // - is in an upper ror sector
        // - in a sector where floor is panning
        if (!SprInside(pFX, nSector) || gUpperLink[nSector] > -1 || IsFloorPanning(nSector))
        {
            gFX.fxKill(pFX->index);
        }
    }
}

// footprints
int fxSpawnFootprint(FX_ID nFx, int nFootprintPicnum, int nSector, int x, int y, int z, int angle)
{
    int nXSector = sector[nSector].extra;
    int nSurf = surfType[sector[nSector].floorpicnum];

    if (!IsUnderwaterSector(nSector) && // if not underwater
        sector[nSector].floorheinum == 0 && // floor is not sloped
        (nXSector == -1 || xsector[nXSector].Depth == 0) && // sector has no depth
        nSurf != kSurfFlesh && nSurf != kSurfWater && nSurf != kSurfGoo && nSurf != kSurfLava) // floor surface is appropriate
    { 
        spritetype *pFX = gFX.fxSpawn(nFx, nSector, x, y, z - 3);
        if (pFX)
        {
            pFX->picnum = nFootprintPicnum;
            pFX->ang = angle;

            // set alignment
            pFX->cstat |= CSTAT_SPRITE_ALIGNMENT_FLOOR | 0x2000; // 0x2000 is floor move mask

            // delete fx when it is:
            // - edging out of the sector
            // - is in an upper ror sector
            // - in a sector where floor is panning
            if (!SprInside(pFX, nSector) || gUpperLink[nSector] > -1 || IsFloorPanning(nSector))
            {
                gFX.fxKill(pFX->index);
                return -1;
            }
            else
            {
                return pFX->index;
            }
        }
        else
        {
            return -1;
        }
    }
    else
    {
        return -1;
    }
}
// end marius

void fxSpawnBlood(spritetype *pSprite, int a2)
{
    UNREFERENCED_PARAMETER(a2);
    if (pSprite->sectnum < 0 || pSprite->sectnum >= numsectors)
        return;
    int nSector = pSprite->sectnum;
    if (!FindSector(pSprite->x, pSprite->y, pSprite->z, &nSector))
        return;
    if (gbAdultContent && gGameOptions.nGameType == kGameTypeSinglePlayer)
        return;
    spritetype *pBlood = gFX.fxSpawn(FX_27, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z);
    if (pBlood)
    {
        pBlood->ang = 1024;
        // marius
        if (VanillaMode()) // original code
        {
            xvel[pBlood->index] = Random2(0x6aaaa);
            yvel[pBlood->index] = Random2(0x6aaaa);
            zvel[pBlood->index] = -((int)Random(0x10aaaa))-100;
        }
        else // extrablood code
        {
            if (IsUnderwaterSector(pSprite->sectnum))
            {
                // Make bloodspray more dense underwater
                // Scale velocities (bigger nr is bigger spread)                
                int nScaleFactor = 80; 
                xvel[pBlood->index] = mulscale8(Random2(0x6aaaa), nScaleFactor);
                yvel[pBlood->index] = mulscale8(Random2(0x6aaaa), nScaleFactor);
                zvel[pBlood->index] = mulscale8(-((int)Random(0x10aaaa)) - 100, nScaleFactor);
            }
            else
            {
                xvel[pBlood->index] = Random2(0x6aaaa);
                yvel[pBlood->index] = Random2(0x6aaaa);
                zvel[pBlood->index] = -((int)Random(0x10aaaa))-100;
            }
        }
        // end marius
        evPost(pBlood->index, 3, 8, kCallbackFXBloodSpurt);
    }
}

void fxSpawnPodBlood(spritetype *pSprite, int a2)
{
    UNREFERENCED_PARAMETER(a2);
    if (pSprite->sectnum < 0 || pSprite->sectnum >= numsectors)
        return;
    int nSector = pSprite->sectnum;
    if (!FindSector(pSprite->x, pSprite->y, pSprite->z, &nSector))
        return;
    if (gbAdultContent && gGameOptions.nGameType == kGameTypeSinglePlayer)
        return;
    spritetype *pSpawn;
    if (pSprite->type == kDudePodGreen)
        pSpawn = gFX.fxSpawn(FX_53, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z);
    else
        pSpawn = gFX.fxSpawn(FX_54, pSprite->sectnum, pSprite->x, pSprite->y, pSprite->z);
    if (pSpawn)
    {
        pSpawn->ang = 1024;
        xvel[pSpawn->index] = Random2(0x6aaaa);
        yvel[pSpawn->index] = Random2(0x6aaaa);
        zvel[pSpawn->index] = -((int)Random(0x10aaaa))-100;
        evPost(pSpawn->index, 3, 8, kCallbackFXPodBloodSpray);
    }
}

void fxSpawnEjectingBrass(spritetype *pSprite, int z, int a3, int a4)
{
    int x = pSprite->x+mulscale28(pSprite->clipdist-4, Cos(pSprite->ang));
    int y = pSprite->y+mulscale28(pSprite->clipdist-4, Sin(pSprite->ang));
    x += mulscale30(a3, Cos(pSprite->ang+512));
    y += mulscale30(a3, Sin(pSprite->ang+512));
    spritetype *pBrass = gFX.fxSpawn((FX_ID)(FX_37+Random(3)), pSprite->sectnum, x, y, z);
    if (pBrass)
    {
        if (!VanillaMode())
            pBrass->ang = Random(2047);
        int nDist = (a4<<18)/120+Random2(((a4/4)<<18)/120);
        int nAngle = pSprite->ang+Random2(56)+512;
        xvel[pBrass->index] = mulscale30(nDist, Cos(nAngle));
        yvel[pBrass->index] = mulscale30(nDist, Sin(nAngle));
        zvel[pBrass->index] = zvel[pSprite->index]-(0x20000+(Random2(40)<<18)/120);
    }
}

void fxSpawnEjectingShell(spritetype *pSprite, int z, int a3, int a4)
{
    int x = pSprite->x+mulscale28(pSprite->clipdist-4, Cos(pSprite->ang));
    int y = pSprite->y+mulscale28(pSprite->clipdist-4, Sin(pSprite->ang));
    x += mulscale30(a3, Cos(pSprite->ang+512));
    y += mulscale30(a3, Sin(pSprite->ang+512));
    spritetype *pShell = gFX.fxSpawn((FX_ID)(FX_40+Random(3)), pSprite->sectnum, x, y, z);
    if (pShell)
    {
        if (!VanillaMode())
            pShell->ang = Random(2047);
        int nDist = (a4<<18)/120+Random2(((a4/4)<<18)/120);
        int nAngle = pSprite->ang+Random2(56)+512;
        xvel[pShell->index] = mulscale30(nDist, Cos(nAngle));
        yvel[pShell->index] = mulscale30(nDist, Sin(nAngle));
        zvel[pShell->index] = zvel[pSprite->index]-(0x20000+(Random2(20)<<18)/120);
    }
}

void fxPrecache(void)
{
    for (int i = 0; i < kFXMax; i++)
    {
        tilePrecacheTile(gFXData[i].picnum, 0);
        if (gFXData[i].seq)
            seqPrecacheId(gFXData[i].seq);
    }
}
