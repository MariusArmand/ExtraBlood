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

struct HITINFO {
    short hitsect;
    short hitwall;
    short hitsprite;
    int hitx;
    int hity;
    int hitz;
};

extern POINT2D baseWall[kMaxWalls];
extern POINT3D baseSprite[kMaxSprites];
extern int baseFloor[kMaxSectors];
extern int baseCeil[kMaxSectors];
extern int velFloor[kMaxSectors];
extern int velCeil[kMaxSectors];
extern short gUpperLink[kMaxSectors];
extern short gLowerLink[kMaxSectors];
extern HITINFO gHitInfo;

enum {
    PARALLAXCLIP_CEILING = 1,
    PARALLAXCLIP_FLOOR = 2,
};


// by NoOne: functions to quickly check range of specific arrays
inline bool spriRangeIsFine(int nIndex) {
    return (nIndex >= 0 && nIndex < kMaxSprites);
}

inline bool sectRangeIsFine(int nIndex) {
    return (nIndex >= 0 && nIndex < kMaxSectors);
}

inline bool wallRangeIsFine(int nIndex) {
    return (nIndex >= 0 && nIndex < kMaxWalls);
}
///

#define DANGLE(a1, a2)  		(((a1 + kAng180 - a2) & kAngMask) - kAng180)

bool AreSectorsNeighbors(int sect1, int sect2);
bool FindSector(int nX, int nY, int nZ, int *nSector);
bool FindSector(int nX, int nY, int *nSector);
void CalcFrameRate(void);
bool CheckProximity(spritetype *pSprite, int nX, int nY, int nZ, int nSector, int nDist);
bool CheckProximityPoint(int nX1, int nY1, int nZ1, int nX2, int nY2, int nZ2, int nDist);
bool CheckProximityWall(int nWall, int x, int y, int nDist);
int GetWallAngle(int nWall);
void GetWallNormal(int nWall, int *pX, int *pY);
bool IntersectRay(int wx, int wy, int wdx, int wdy, int x1, int y1, int z1, int x2, int y2, int z2, int *ix, int *iy, int *iz);
int HitScan(spritetype *pSprite, int z, int dx, int dy, int dz, unsigned int nMask, int a8);
int VectorScan(spritetype *pSprite, int nOffset, int nZOffset, int dx, int dy, int dz, int nRange, int ac);
void GetZRange(spritetype *pSprite, int *ceilZ, int *ceilHit, int *floorZ, int *floorHit, int nDist, unsigned int nMask, unsigned int nClipParallax = 0);
void GetZRangeAtXYZ(int x, int y, int z, int nSector, int *ceilZ, int *ceilHit, int *floorZ, int *floorHit, int nDist, unsigned int nMask, unsigned int nClipParallax = 0);
int GetDistToLine(int x1, int y1, int x2, int y2, int x3, int y3);
unsigned int ClipMove(int *x, int *y, int *z, int *nSector, int xv, int yv, int wd, int cd, int fd, unsigned int nMask);
int GetClosestSectors(int nSector, int x, int y, int nDist, short *pSectors, char *pSectBit);
int GetClosestSpriteSectors(int nSector, int x, int y, int nDist, short *pSectors, char *pSectBit, short *pWalls = NULL, bool bAccurateCheck = false);
void GetSpriteExtents(spritetype* pSpr, int* x1, int* y1, int* x2, int* y2, int* zt = NULL, int* zb = NULL, char flags = 0x07);
// NoOne, extrablood
// ceiling fx
void GetSpriteExtents(spritetype* pSpr, int* x1, int* y1, int* x2, int* y2, int* x3, int* y3, int* x4, int* y4, char flags = 0x07);
bool SprInside(spritetype* pSpr, int nSect);
void OffsetPos(int oX, int oY, int oZ, int nAng, int* x, int* y, int* z);
void DoWallCorrection(int nWall, int* x, int* y, int step = 4);
char CanPutOnWall(spritetype* pSpr, int nWall, int wAng, int nMaxDang, int nTopOffset);
inline char CanPutOnWall(spritetype* pSpr, int nWall, int nMaxDang = kAng60, int nTopOffset = 0)
{
    return CanPutOnWall(pSpr, nWall, GetWallAngle(nWall), nMaxDang, nTopOffset);
}
// End NoOne, extrablood
bool IsFloorPanning(int nSector); // marius, floor fx