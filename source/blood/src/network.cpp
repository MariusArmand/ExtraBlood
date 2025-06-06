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
#include "mmulti.h"
#include "pragmas.h"
#ifndef NETCODE_DISABLE
#include "enet.h"
#endif
#include "compat.h"
#include "config.h"
#include "controls.h"
#include "globals.h"
#include "network.h"
#include "menu.h"
#include "player.h"
#include "seq.h"
#include "sound.h"
#include "view.h"

char packet[576];
bool gStartNewGame = 0;
PACKETMODE gPacketMode = PACKETMODE_1;
ClockTicks gNetFifoClock = 0;
int gNetFifoTail = 0;
int gNetFifoHead[8];
int gPredictTail = 0;
int gNetFifoMasterTail = 0;
GINPUT gFifoInput[256][8];
int myMinLag[8];
int otherMinLag = 0;
int myMaxLag = 0;
unsigned int gChecksum[4];
unsigned int gCheckFifo[256][8][4];
int gCheckHead[8];
int gSendCheckTail = 0;
int gCheckTail = 0;
int gInitialNetPlayers = 0;
int gBufferJitter = 1;
int gPlayerReady[8];
bool bNoResend = true;
bool gRobust = false;
bool bOutOfSync = false;
bool ready2send = false;

NETWORKMODE gNetMode = NETWORK_NONE;
char gNetAddress[32];
// PORT-TODO: Use different port?
int gNetPort = kNetDefaultPort;

const short kNetVersion = 0x214;

PKT_STARTGAME gPacketStartGame;

#ifndef NETCODE_DISABLE
ENetAddress gNetENetAddress;
ENetHost *gNetENetServer;
ENetHost *gNetENetClient;
ENetPeer *gNetENetPeer;
ENetPeer *gNetPlayerPeer[kMaxPlayers];
bool gNetENetInit = false;

#define kENetFifoSize 2048

//ENetPacket *gNetServerPacketFifo[kENetFifoSize];
//int gNetServerPacketHead, gNetServerPacketTail;
ENetPacket *gNetPacketFifo[kENetFifoSize];
int gNetPacketHead, gNetPacketTail;

enum BLOOD_ENET_CHANNEL {
    BLOOD_ENET_SERVICE = 0,
    BLOOD_ENET_GAME,
    BLOOD_ENET_CHANNEL_MAX
};

enum BLOOD_SERVICE {
    BLOOD_SERVICE_CONNECTINFO = 0,
    BLOOD_SERVICE_CONNECTID,
    BLOOD_SERVICE_SENDTOID,
};

struct PKT_CONNECTINFO {
    short numplayers;
    short connecthead;
    short connectpoint2[kMaxPlayers];
};

void netServerDisconnect(void)
{
    ENetEvent event;
    if (gNetMode != NETWORK_SERVER)
        return;
    for (int p = 0; p < kMaxPlayers; p++)
        if (gNetPlayerPeer[p])
        {
            bool bDisconnectStatus = false;
            enet_peer_disconnect_later(gNetPlayerPeer[p], 0);
            if (enet_host_service(gNetENetServer, &event, 3000) > 0)
            {
                switch (event.type)
                {
                case ENET_EVENT_TYPE_DISCONNECT:
                    bDisconnectStatus = true;
                    break;
                default:
                    break;
                }
            }
            if (!bDisconnectStatus)
                enet_peer_reset(gNetPlayerPeer[p]);
            gNetPlayerPeer[p] = NULL;
        }
}

void netClientDisconnect(void)
{
    ENetEvent event;
    if (gNetMode != NETWORK_CLIENT || gNetENetPeer == NULL)
        return;
    enet_peer_disconnect_later(gNetENetPeer, 0);
    bool bDisconnectStatus = false;
    if (enet_host_service(gNetENetClient, &event, 3000) > 0)
    {
        switch (event.type)
        {
        case ENET_EVENT_TYPE_DISCONNECT:
            bDisconnectStatus = true;
            break;
        default:
            break;
        }
    }
    if (!bDisconnectStatus)
        enet_peer_reset(gNetENetPeer);
    gNetENetPeer = NULL;
}
#endif

void netResetToSinglePlayer(void)
{
    myconnectindex = connecthead = 0;
    gInitialNetPlayers = gNetPlayers = numplayers = 1;
    connectpoint2[0] = -1;
    gGameOptions.nGameType = kGameTypeSinglePlayer;
    gNetMode = NETWORK_NONE;
    UpdateNetworkMenus();
    gGameMenuMgr.Deactivate();
}

void netSendPacket(int nDest, char *pBuffer, int nSize)
{
#ifndef NETCODE_DISABLE
    if (gNetMode == NETWORK_NONE)
        return;
    netUpdate();

    if (gNetMode == NETWORK_SERVER)
    {
        if (gNetPlayerPeer[nDest] != NULL)
        {
            ENetPacket *pNetPacket = enet_packet_create(NULL, nSize + 1, ENET_PACKET_FLAG_RELIABLE);
            char *pPBuffer = (char*)pNetPacket->data;
            PutPacketByte(pPBuffer, myconnectindex);
            PutPacketBuffer(pPBuffer, pBuffer, nSize);
            enet_peer_send(gNetPlayerPeer[nDest], BLOOD_ENET_GAME, pNetPacket);
            enet_host_service(gNetENetServer, NULL, 0);
        }
    }
    else
    {
        if (nDest == 0)
        {
            ENetPacket *pNetPacket = enet_packet_create(NULL, nSize + 1, ENET_PACKET_FLAG_RELIABLE);
            char *pPBuffer = (char*)pNetPacket->data;
            PutPacketByte(pPBuffer, myconnectindex);
            PutPacketBuffer(pPBuffer, pBuffer, nSize);
            enet_peer_send(gNetENetPeer, BLOOD_ENET_GAME, pNetPacket);
        }
        else
        {
            ENetPacket *pNetPacket = enet_packet_create(NULL, nSize + 3, ENET_PACKET_FLAG_RELIABLE);
            char *pPBuffer = (char*)pNetPacket->data;
            PutPacketByte(pPBuffer, BLOOD_SERVICE_SENDTOID);
            PutPacketByte(pPBuffer, nDest);
            PutPacketByte(pPBuffer, myconnectindex);
            PutPacketBuffer(pPBuffer, pBuffer, nSize);
            enet_peer_send(gNetENetPeer, BLOOD_ENET_SERVICE, pNetPacket);
        }
        enet_host_service(gNetENetClient, NULL, 0);
    }

    netUpdate();
#endif
}

void netSendPacketAll(char *pBuffer, int nSize)
{
    for (int p = connecthead; p >= 0; p = connectpoint2[p])
        if (p != myconnectindex)
            netSendPacket(p, pBuffer, nSize);
}

void netResetState(void)
{
    gNetFifoClock = gFrameClock = totalclock = 0;
    gNetFifoMasterTail = 0;
    gPredictTail = 0;
    gNetFifoTail = 0;
    memset(gNetFifoHead, 0, sizeof(gNetFifoHead));
    memset(gCheckFifo, 0, sizeof(gCheckFifo));
    memset(myMinLag, 0, sizeof(myMinLag));
    otherMinLag = 0;
    myMaxLag = 0;
    memset(gCheckHead, 0, sizeof(gCheckHead));
    gSendCheckTail = 0;
    gCheckTail = 0;
    bOutOfSync = 0;
    gBufferJitter = 1;
}

void CalcGameChecksum(void)
{
    memset(gChecksum, 0, sizeof(gChecksum));
    gChecksum[0] = wrand();
    for (int p = connecthead; p >= 0; p = connectpoint2[p])
    {
        gChecksum[1] ^= gPlayer[p].CalcNonSpriteChecksum();

        int *pBuffer = (int*)gPlayer[p].pSprite;
        int sum = 0;
        int length = sizeof(spritetype)/4;
        while (length--)
        {
            sum += *pBuffer++;
        }
        gChecksum[2] ^= sum;

        gChecksum[3] ^= gPlayer[p].pXSprite->CalcChecksum();
    }
}

void netCheckSync(void)
{
    char buffer[80];
    if (gGameOptions.nGameType == kGameTypeSinglePlayer)
        return;
    if (numplayers == 1)
        return;
    if (bOutOfSync)
        return;
    while (1)
    {
        for (int p = connecthead; p >= 0; p = connectpoint2[p])
        {
            if (gCheckTail >= gCheckHead[p])
                return;
        }

        for (int p = connecthead; p >= 0; p = connectpoint2[p])
        {
            if (p != myconnectindex)
            {
                int status = memcmp(gCheckFifo[gCheckTail&255][p], gCheckFifo[gCheckTail&255][connecthead], 16);
                if (status)
                {
                    sprintf(buffer, "OUT OF SYNC (%d)", p);
                    char *pBuffer = buffer + strlen(buffer);
                    for (unsigned int i = 0; i < 4; i++)
                    {
                        if (gCheckFifo[gCheckTail&255][p][i] != gCheckFifo[gCheckTail&255][connecthead][i])
                            pBuffer += sprintf(pBuffer, " %d", i);
                    }
                    viewSetErrorMessage(buffer);
                    bOutOfSync = 1;
                }
            }
        }
        gCheckTail++;
    }
}

short netGetPacket(short *pSource, char *pMessage)
{
#ifndef NETCODE_DISABLE
    if (gNetMode == NETWORK_NONE)
        return 0;
    netUpdate();
    if (gNetPacketTail != gNetPacketHead)
    {
        ENetPacket *pEPacket = gNetPacketFifo[gNetPacketTail];
        gNetPacketTail = (gNetPacketTail+1)%kENetFifoSize;
        char *pPacket = (char*)pEPacket->data;
        *pSource = GetPacketByte(pPacket);
        int nSize = pEPacket->dataLength-1;
        memcpy(pMessage, pPacket, nSize);
        enet_packet_destroy(pEPacket);
        netUpdate();
        return nSize;
    }
    netUpdate();
#endif
    return 0;
}

void netGetPackets(void)
{
    short nPlayer;
    short nSize;
    char buffer[128];
    while ((nSize = netGetPacket(&nPlayer, packet)) > 0)
    {
        char *pPacket = packet;
        switch (GetPacketByte(pPacket))
        {
        case 0: // From master
            for (int p = connecthead; p >= 0; p = connectpoint2[p])
            {
                if (p != myconnectindex)
                {
                    GINPUT *pInput = &gFifoInput[gNetFifoHead[p]&255][p];
                    memset(pInput, 0, sizeof(GINPUT));
                    pInput->syncFlags.byte = GetPacketByte(pPacket);
                    pInput->forward = GetPacketWord(pPacket);
                    pInput->q16turn = GetPacketDWord(pPacket);
                    pInput->strafe = GetPacketWord(pPacket);
                    if (pInput->syncFlags.buttonChange)
                        pInput->buttonFlags.byte = GetPacketByte(pPacket);
                    if (pInput->syncFlags.keyChange)
                        pInput->keyFlags.word = GetPacketWord(pPacket);
                    if (pInput->syncFlags.useChange)
                        pInput->useFlags.byte = GetPacketByte(pPacket);
                    if (pInput->syncFlags.weaponChange)
                        pInput->newWeapon = GetPacketByte(pPacket);
                    if (pInput->syncFlags.mlookChange)
                        pInput->q16mlook = GetPacketDWord(pPacket);
                    gNetFifoHead[p]++;
                }
                else
                {
                    SYNCFLAGS syncFlags;
                    syncFlags.byte = GetPacketByte(pPacket);
                    pPacket += 2+4+2;
                    if (syncFlags.buttonChange)
                        pPacket++;
                    if (syncFlags.keyChange)
                        pPacket+=2;
                    if (syncFlags.useChange)
                        pPacket++;
                    if (syncFlags.weaponChange)
                        pPacket++;
                    if (syncFlags.mlookChange)
                        pPacket+=4;
                }
            }
            if (((gNetFifoHead[connecthead]-1)&15)==0)
            {
                for (int p = connectpoint2[connecthead]; p >= 0; p = connectpoint2[p])
                {
                    int nLag = (signed char)GetPacketByte(pPacket);
                    if (p == myconnectindex)
                        otherMinLag = nLag;
                }
            }
            while (pPacket < packet+nSize)
            {
                int checkSum[4];
                GetPacketBuffer(pPacket, checkSum, sizeof(checkSum));
                for (int p = connecthead; p >= 0; p = connectpoint2[p])
                {
                    if (p != myconnectindex)
                    {
                        memcpy(gCheckFifo[gCheckHead[p]&255][p], checkSum, sizeof(checkSum));
                        gCheckHead[p]++;
                    }
                }
            }
            break;
        case 1: // From slave
        {
            GINPUT *pInput = &gFifoInput[gNetFifoHead[nPlayer]&255][nPlayer];
            memset(pInput, 0, sizeof(GINPUT));
            pInput->syncFlags.byte = GetPacketByte(pPacket);
            pInput->forward = GetPacketWord(pPacket);
            pInput->q16turn = GetPacketDWord(pPacket);
            pInput->strafe = GetPacketWord(pPacket);
            if (pInput->syncFlags.buttonChange)
                pInput->buttonFlags.byte = GetPacketByte(pPacket);
            if (pInput->syncFlags.keyChange)
                pInput->keyFlags.word = GetPacketWord(pPacket);
            if (pInput->syncFlags.useChange)
                pInput->useFlags.byte = GetPacketByte(pPacket);
            if (pInput->syncFlags.weaponChange)
                pInput->newWeapon = GetPacketByte(pPacket);
            if (pInput->syncFlags.mlookChange)
                pInput->q16mlook = GetPacketDWord(pPacket);
            gNetFifoHead[nPlayer]++;
            while (pPacket < packet+nSize)
            {
                int checkSum[4];
                GetPacketBuffer(pPacket, checkSum, sizeof(checkSum));
                memcpy(gCheckFifo[gCheckHead[nPlayer]&255][nPlayer], checkSum, sizeof(checkSum));
                gCheckHead[nPlayer]++;
            }
            break;
        }
        case 2:
        {
            if (nPlayer == connecthead && (gNetFifoHead[nPlayer]&15) == 0)
            {
                for (int p = connectpoint2[connecthead]; p >= 0; p = connectpoint2[p])
                {
                    int nLag = (signed char)GetPacketByte(pPacket);
                    if (p == myconnectindex)
                        otherMinLag = nLag;
                }
            }
            GINPUT *pInput = &gFifoInput[gNetFifoHead[nPlayer]&255][nPlayer];
            memset(pInput, 0, sizeof(GINPUT));
            pInput->syncFlags.byte = GetPacketByte(pPacket);
            pInput->forward = GetPacketWord(pPacket);
            pInput->q16turn = GetPacketDWord(pPacket);
            pInput->strafe = GetPacketWord(pPacket);
            if (pInput->syncFlags.buttonChange)
                pInput->buttonFlags.byte = GetPacketByte(pPacket);
            if (pInput->syncFlags.keyChange)
                pInput->keyFlags.word = GetPacketWord(pPacket);
            if (pInput->syncFlags.useChange)
                pInput->useFlags.byte = GetPacketByte(pPacket);
            if (pInput->syncFlags.weaponChange)
                pInput->newWeapon = GetPacketByte(pPacket);
            if (pInput->syncFlags.mlookChange)
                pInput->q16mlook = GetPacketDWord(pPacket);
            gNetFifoHead[nPlayer]++;
            while (pPacket < packet+nSize)
            {
                int checkSum[4];
                GetPacketBuffer(pPacket, checkSum, sizeof(checkSum));
                memcpy(gCheckFifo[gCheckHead[nPlayer]&255][nPlayer], checkSum, sizeof(checkSum));
                gCheckHead[nPlayer]++;
            }
            break;
        }
        case 3:
            pPacket += 4;
            if (pPacket[0] != '/' || (pPacket[0] == '/' && (!pPacket[1] || (pPacket[1] >= '1' && pPacket[1] <= '8' && pPacket[1] - '1' == myconnectindex))))
            {
                sprintf(buffer, VanillaMode() ? "%s : %s" : "%s: %s", gProfile[nPlayer].name, pPacket);
                viewSetMessage(buffer, VanillaMode() ? 0 : 10); // 10: dark blue
                sndStartSample("DMRADIO", 128, -1);
            }
            break;
        case 4:
            sndStartSample(4400+GetPacketByte(pPacket), 128, 1, 0);
            break;
        case 7:
            nPlayer = GetPacketDWord(pPacket);
            dassert(nPlayer != myconnectindex);
            netWaitForEveryone(0);
            netPlayerQuit(nPlayer);
            netWaitForEveryone(0);
            break;
        case 249:
            nPlayer = GetPacketDWord(pPacket);
            dassert(nPlayer != myconnectindex);
            netPlayerQuit(nPlayer);
            netWaitForEveryone(0);
            break;
        case 250:
            gPlayerReady[nPlayer]++;
            break;
        case 251:
            memcpy(&gProfile[nPlayer], pPacket, sizeof(PROFILE));
            break;
        case 252:
            pPacket += 4;
            memcpy(&gPacketStartGame, pPacket, sizeof(PKT_STARTGAME));
            if (gPacketStartGame.version != kNetVersion)
                ThrowError("\nThese versions of Blood cannot play together.\n");
            gStartNewGame = 1;
            break;
        case 255:
            keystatus[1] = 1;
            break;
        }
    }
}

void netBroadcastPlayerLogoff(int nPlayer)
{
    if (numplayers < 2)
        return;
    netWaitForEveryone(0);
    netPlayerQuit(nPlayer);
    if (nPlayer != myconnectindex)
        netWaitForEveryone(0);
}

void netBroadcastMyLogoff(bool bRestart)
{
    if (numplayers < 2)
        return;
    char *pPacket = packet;
    PutPacketByte(pPacket, 7);
    PutPacketDWord(pPacket, myconnectindex);
    netSendPacketAll(packet, pPacket - packet);
    netWaitForEveryone(0);
    ready2send = 0;
    gQuitGame = true;
    if (bRestart)
        gRestartGame = true;
    netDeinitialize();
    netResetToSinglePlayer();
}

void netBroadcastPlayerInfo(int nPlayer)
{
    PROFILE *pProfile = &gProfile[nPlayer];
    Bstrncpyz(pProfile->name, szPlayerName, sizeof(szPlayerName));
    pProfile->nAutoAim = gAutoAim;
    pProfile->nWeaponSwitch = gWeaponSwitch;
    if (numplayers < 2)
        return;
    pProfile->skill = gSkill;
    char *pPacket = packet;
    PutPacketByte(pPacket, 251);
    PutPacketBuffer(pPacket, pProfile, sizeof(PROFILE));
    netSendPacketAll(packet, pPacket-packet);
}

void netBroadcastNewGame(void)
{
    if (numplayers < 2)
        return;
    gPacketStartGame.version = kNetVersion;
    char *pPacket = packet;
    PutPacketByte(pPacket, 252);
    PutPacketDWord(pPacket, myconnectindex);
    PutPacketBuffer(pPacket, &gPacketStartGame, sizeof(PKT_STARTGAME));
    netSendPacketAll(packet, pPacket-packet);
}

void netBroadcastTaunt(int nPlayer, int nTaunt)
{
    UNREFERENCED_PARAMETER(nPlayer);
    if (numplayers > 1)
    {
        char *pPacket = packet;
        PutPacketByte(pPacket, 4);
        PutPacketByte(pPacket, nTaunt);
        netSendPacketAll(packet, pPacket-packet);
    }
    sndStartSample(4400+nTaunt, 128, 1, 0);
}

void netBroadcastMessage(int nPlayer, const char *pzMessage)
{
    if (numplayers > 1)
    {
        int nSize = strlen(pzMessage);
        char *pPacket = packet;
        PutPacketByte(pPacket, 3);
        PutPacketDWord(pPacket, nPlayer);
        PutPacketBuffer(pPacket, pzMessage, nSize+1);
        netSendPacketAll(packet, pPacket-packet);
    }
}

void netWaitForEveryone(char a1)
{
    if (numplayers < 2)
        return;
    char *pPacket = packet;
    PutPacketByte(pPacket, 250);
    netSendPacketAll(packet, pPacket-packet);
    gPlayerReady[myconnectindex]++;
    int p;
    do
    {
        if (keystatus[sc_Escape] && a1)
            exit(0);
        gameHandleEvents();
        faketimerhandler();
        for (p = connecthead; p >= 0; p = connectpoint2[p])
            if (gPlayerReady[p] < gPlayerReady[myconnectindex])
                break;
        if (gRestartGame || gNetPlayers <= 1)
            break;
    } while (p >= 0);
}

void netBroadcastFrag(const char *pzString)
{
    if (numplayers < 2)
        return;
    if (pzString)
    {
        int nLength = strlen(pzString);
        if (nLength > 0)
        {
            char *pPacket = packet;
            PutPacketByte(pPacket, 5);
            PutPacketBuffer(pPacket, pzString, nLength+1);
            netSendPacketAll(packet, pPacket-packet);
        }
    }
}

void netSendEmptyPackets(void)
{
    ClockTicks nClock = totalclock;
    char *pPacket = packet;
    PutPacketByte(pPacket, 254);
    for (int i = 0; i < 8; i++)
    {
        if (nClock <= totalclock)
        {
            nClock = totalclock+kTicsPerFrame;
            netSendPacketAll(packet, pPacket-packet);
        }
    }
}

void netMasterUpdate(void)
{
    if (myconnectindex != connecthead)
        return;
    char v4 = 0;
    do
    {
        for (int p = connecthead; p >= 0; p = connectpoint2[p])
            if (gNetFifoMasterTail >= gNetFifoHead[p])
            {
                if (v4)
                    return;
                char *pPacket = packet;
                PutPacketByte(pPacket, 254);
                for (; p >= 0; p = connectpoint2[p])
                    netSendPacket(p, packet, pPacket-packet);
                return;
            }
        v4 = 1;
        char *pPacket = packet;
        PutPacketByte(pPacket, 0);
        for (int p = connecthead; p >= 0; p = connectpoint2[p])
        {
            GINPUT *pInput = &gFifoInput[gNetFifoMasterTail&255][p];
            if (pInput->buttonFlags.byte)
                pInput->syncFlags.buttonChange = 1;
            if (pInput->keyFlags.word)
                pInput->syncFlags.keyChange = 1;
            if (pInput->useFlags.byte)
                pInput->syncFlags.useChange = 1;
            if (pInput->newWeapon)
                pInput->syncFlags.weaponChange = 1;
            if (pInput->q16mlook)
                pInput->syncFlags.mlookChange = 1;
            PutPacketByte(pPacket, pInput->syncFlags.byte);
            PutPacketWord(pPacket, pInput->forward);
            PutPacketDWord(pPacket, pInput->q16turn);
            PutPacketWord(pPacket, pInput->strafe);
            if (pInput->syncFlags.buttonChange)
                PutPacketByte(pPacket, pInput->buttonFlags.byte);
            if (pInput->syncFlags.keyChange)
                PutPacketWord(pPacket, pInput->keyFlags.word);
            if (pInput->syncFlags.useChange)
                PutPacketByte(pPacket, pInput->useFlags.byte);
            if (pInput->syncFlags.weaponChange)
                PutPacketByte(pPacket, pInput->newWeapon);
            if (pInput->syncFlags.mlookChange)
                PutPacketDWord(pPacket, pInput->q16mlook);
        }
        if ((gNetFifoMasterTail&15) == 0)
        {
            for (int p = connectpoint2[connecthead]; p >= 0; p = connectpoint2[p])
                PutPacketByte(pPacket, ClipRange(myMinLag[p], -128, 127));
            for (int p = connecthead; p >= 0; p = connectpoint2[p])
                myMinLag[p] = 0x7fffffff;
        }
        while (gSendCheckTail != gCheckHead[myconnectindex])
        {
            PutPacketBuffer(pPacket, gCheckFifo[gSendCheckTail&255][myconnectindex], 16);
            gSendCheckTail++;
        }
        for (int p = connectpoint2[connecthead]; p >= 0; p = connectpoint2[p])
            netSendPacket(p, packet, pPacket-packet);
        gNetFifoMasterTail++;
    } while (1);
}

void netGetInput(void)
{
    if (numplayers > 1)
        netGetPackets();
    for (int p = connecthead; p >= 0; p = connectpoint2[p])
        if (gNetFifoHead[myconnectindex]-200 > gNetFifoHead[p])
            return;
    GINPUT &input = gFifoInput[gNetFifoHead[myconnectindex]&255][myconnectindex];
    input = gNetInput;
    gNetFifoHead[myconnectindex]++;
    if (gGameOptions.nGameType == kGameTypeSinglePlayer || numplayers == 1)
    {
        for (int p = connecthead; p >= 0; p = connectpoint2[p])
        {
            if (p != myconnectindex)
            {
                GINPUT *pInput1 = &gFifoInput[(gNetFifoHead[p]-1)&255][p];
                GINPUT *pInput2 = &gFifoInput[gNetFifoHead[p]&255][p];
                memcpy(pInput2, pInput1, sizeof(GINPUT));
                gNetFifoHead[p]++;
            }
        }
        return;
    }
    for (int p = connecthead; p >= 0; p = connectpoint2[p])
    {
        if (p != myconnectindex)
        {
            int nLag = gNetFifoHead[myconnectindex]-1-gNetFifoHead[p];
            myMinLag[p] = ClipHigh(nLag, myMinLag[p]);
            myMaxLag = ClipLow(nLag, myMaxLag);
        }
    }
    if (((gNetFifoHead[myconnectindex]-1)&15) == 0)
    {
        int t = myMaxLag-gBufferJitter;
        myMaxLag = 0;
        if (t > 0)
            gBufferJitter += (3+t)>>2;
        else if (t < 0)
            gBufferJitter -= (1-t)>>2;
    }
    if (gPacketMode == PACKETMODE_2)
    {
        char *pPacket = packet;
        PutPacketByte(pPacket, 2);
        if (((gNetFifoHead[myconnectindex]-1)&15) == 0)
        {
            if (myconnectindex == connecthead)
            {
                for (int p = connectpoint2[connecthead]; p >= 0; p = connectpoint2[p])
                    PutPacketByte(pPacket, ClipRange(myMinLag[p], -128, 127));
            }
            else
            {
                int t = myMinLag[connecthead]-otherMinLag;
                if (klabs(t) > 2)
                {
                    if (klabs(t) > 8)
                    {
                        if (t < 0)
                            t++;
                        t >>= 1;
                    }
                    else
                        t = ksgn(t);
                    totalclock -= t<<2;
                    otherMinLag += t;
                    myMinLag[connecthead] -= t;
                }
            }
            for (int p = connecthead; p >= 0; p = connectpoint2[p])
                myMinLag[p] = 0x7fffffff;
        }
        if (input.buttonFlags.byte)
            input.syncFlags.buttonChange = 1;
        if (input.keyFlags.word)
            input.syncFlags.keyChange = 1;
        if (input.useFlags.byte)
            input.syncFlags.useChange = 1;
        if (input.newWeapon)
            input.syncFlags.weaponChange = 1;
        if (input.q16mlook)
            input.syncFlags.mlookChange = 1;
        PutPacketByte(pPacket, input.syncFlags.byte);
        PutPacketWord(pPacket, input.forward);
        PutPacketDWord(pPacket, input.q16turn);
        PutPacketWord(pPacket, input.strafe);
        if (input.syncFlags.buttonChange)
            PutPacketByte(pPacket, input.buttonFlags.byte);
        if (input.syncFlags.keyChange)
            PutPacketWord(pPacket, input.keyFlags.word);
        if (input.syncFlags.useChange)
            PutPacketByte(pPacket, input.useFlags.byte);
        if (input.syncFlags.weaponChange)
            PutPacketByte(pPacket, input.newWeapon);
        if (input.syncFlags.mlookChange)
            PutPacketDWord(pPacket, input.q16mlook);
        while (gSendCheckTail != gCheckHead[myconnectindex])
        {
            unsigned int *checkSum = gCheckFifo[gSendCheckTail&255][myconnectindex];
            PutPacketBuffer(pPacket, checkSum, 16);
            gSendCheckTail++;
        }
        netSendPacketAll(packet, pPacket-packet);
        return;
    }
    if (myconnectindex != connecthead)
    {
        char *pPacket = packet;
        PutPacketByte(pPacket, 1);
        if (input.buttonFlags.byte)
            input.syncFlags.buttonChange = 1;
        if (input.keyFlags.word)
            input.syncFlags.keyChange = 1;
        if (input.useFlags.byte)
            input.syncFlags.useChange = 1;
        if (input.newWeapon)
            input.syncFlags.weaponChange = 1;
        if (input.q16mlook)
            input.syncFlags.mlookChange = 1;
        PutPacketByte(pPacket, input.syncFlags.byte);
        PutPacketWord(pPacket, input.forward);
        PutPacketDWord(pPacket, input.q16turn);
        PutPacketWord(pPacket, input.strafe);
        if (input.syncFlags.buttonChange)
            PutPacketByte(pPacket, input.buttonFlags.byte);
        if (input.syncFlags.keyChange)
            PutPacketWord(pPacket, input.keyFlags.word);
        if (input.syncFlags.useChange)
            PutPacketByte(pPacket, input.useFlags.byte);
        if (input.syncFlags.weaponChange)
            PutPacketByte(pPacket, input.newWeapon);
        if (input.syncFlags.mlookChange)
            PutPacketDWord(pPacket, input.q16mlook);
        if (((gNetFifoHead[myconnectindex]-1)&15) == 0)
        {
            int t = myMinLag[connecthead]-otherMinLag;
            if (klabs(t) > 2)
            {
                if (klabs(t) > 8)
                {
                    if (t < 0)
                        t++;
                    t >>= 1;
                }
                else
                    t = ksgn(t);
                totalclock -= t<<2;
                otherMinLag += t;
                myMinLag[connecthead] -= t;
            }
            for (int p = connecthead; p >= 0; p = connectpoint2[p])
                myMinLag[p] = 0x7fffffff;
        }
        while (gSendCheckTail != gCheckHead[myconnectindex])
        {
            PutPacketBuffer(pPacket, gCheckFifo[gSendCheckTail&255][myconnectindex], 16);
            gSendCheckTail++;
        }
        netSendPacket(connecthead, packet, pPacket-packet);
        return;
    }
    netMasterUpdate();
}

void netInitialize(bool bConsole)
{
    netDeinitialize();
    memset(gPlayerReady, 0, sizeof(gPlayerReady));
    netResetState();
#ifndef NETCODE_DISABLE
    char buffer[128];
    gNetENetServer = gNetENetClient = NULL;
    //gNetServerPacketHead = gNetServerPacketTail = 0;
    gNetPacketHead = gNetPacketTail = 0;
    if (gNetMode == NETWORK_NONE)
    {
        netResetToSinglePlayer();
        return;
    }
    if (enet_initialize() != 0)
    {
        LOG_F(ERROR, "An error occurred while initializing ENet.");
        netResetToSinglePlayer();
        return;
    }
    if (gNetMode == NETWORK_SERVER)
    {
        memset(gNetPlayerPeer, 0, sizeof(gNetPlayerPeer));
        ENetEvent event;
        gNetENetAddress.host = ENET_HOST_ANY;
        gNetENetAddress.port = gNetPort;
        gNetENetServer = enet_host_create(&gNetENetAddress, 8, BLOOD_ENET_CHANNEL_MAX, 0, 0);
        if (!gNetENetServer)
        {
            LOG_F(ERROR, "An error occurred while trying to create an ENet server host.");
            netDeinitialize();
            netResetToSinglePlayer();
            return;
        }

        numplayers = 1;
        // Wait for clients
        if (!bConsole)
        {
            char buffer[128];
            sprintf(buffer, "Waiting for players (%i\\%i)", numplayers, gNetPlayers);
            viewLoadingScreen(gMenuPicnum, "Network Game", NULL, buffer);
            videoNextPage();
        }
        while (numplayers < gNetPlayers)
        {
            handleevents();
            if (quitevent)
            {
                netServerDisconnect();
                QuitGame();
            }
            if (!bConsole && KB_KeyPressed(sc_Escape))
            {
                netServerDisconnect();
                netDeinitialize();
                netResetToSinglePlayer();
                return;
            }
            enet_host_service(gNetENetServer, NULL, 0);
            if (enet_host_check_events(gNetENetServer, &event) > 0)
            {
                switch (event.type)
                {
                case ENET_EVENT_TYPE_CONNECT:
                {
                    char ipaddr[32];

                    enet_address_get_host_ip(&event.peer->address, ipaddr, sizeof(ipaddr));
                    LOG_F(INFO, "Client connected: %s:%u", ipaddr, event.peer->address.port);
                    numplayers++;
                    for (int i = 1; i < kMaxPlayers; i++)
                    {
                        if (gNetPlayerPeer[i] == NULL)
                        {
                            gNetPlayerPeer[i] = event.peer;
                            break;
                        }
                    }
                    if (!bConsole)
                    {
                        char buffer[128];
                        sprintf(buffer, "Waiting for players (%i\\%i)", numplayers, gNetPlayers);
                        viewLoadingScreen(gMenuPicnum, "Network Game", NULL, buffer);
                        videoNextPage();
                    }
                    break;
                }
                case ENET_EVENT_TYPE_DISCONNECT:
                {
                    char ipaddr[32];

                    enet_address_get_host_ip(&event.peer->address, ipaddr, sizeof(ipaddr));
                    LOG_F(INFO, "Client disconnected: %s:%u", ipaddr, event.peer->address.port);
                    numplayers--;
                    for (int i = 1; i < kMaxPlayers; i++)
                    {
                        if (gNetPlayerPeer[i] == event.peer)
                        {
                            gNetPlayerPeer[i] = NULL;
                            for (int j = kMaxPlayers-1; j > i; j--)
                            {
                                if (gNetPlayerPeer[j])
                                {
                                    gNetPlayerPeer[i] = gNetPlayerPeer[j];
                                    gNetPlayerPeer[j] = NULL;
                                    break;
                                }
                            }
                        }
                    }
                    if (!bConsole)
                    {
                        char buffer[128];
                        sprintf(buffer, "Waiting for players (%i\\%i)", numplayers, gNetPlayers);
                        viewLoadingScreen(gMenuPicnum, "Network Game", NULL, buffer);
                        videoNextPage();
                    }
                    break;
                }
                default:
                    break;
                }
            }
            enet_host_service(gNetENetServer, NULL, 0);
        }
        LOG_F(INFO, "All clients connected");

        dassert(numplayers >= 1);

        gInitialNetPlayers = gNetPlayers = numplayers;
        connecthead = 0;
        for (int i = 0; i < numplayers-1; i++)
            connectpoint2[i] = i+1;
        connectpoint2[numplayers-1] = -1;

        enet_host_service(gNetENetServer, NULL, 0);

        // Send connect info
        char *pPacket = packet;
        PutPacketByte(pPacket, BLOOD_SERVICE_CONNECTINFO);
        PKT_CONNECTINFO connectinfo;
        connectinfo.numplayers = numplayers;
        connectinfo.connecthead = connecthead;
        for (int i = 0; i < numplayers; i++)
            connectinfo.connectpoint2[i] = connectpoint2[i];
        PutPacketBuffer(pPacket, &connectinfo, sizeof(connectinfo));
        ENetPacket *pEPacket = enet_packet_create(packet, pPacket-packet, ENET_PACKET_FLAG_RELIABLE);
        enet_host_broadcast(gNetENetServer, BLOOD_ENET_SERVICE, pEPacket);
        //enet_packet_destroy(pEPacket);
        
        enet_host_service(gNetENetServer, NULL, 0);

        // Send id
        myconnectindex = 0;
        for (int i = 1; i < numplayers; i++)
        {
            if (!gNetPlayerPeer[i])
                continue;
            char *pPacket = packet;
            PutPacketByte(pPacket, BLOOD_SERVICE_CONNECTID);
            PutPacketByte(pPacket, i);
            ENetPacket *pEPacket = enet_packet_create(packet, pPacket-packet, ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(gNetPlayerPeer[i], BLOOD_ENET_SERVICE, pEPacket);

            enet_host_service(gNetENetServer, NULL, 0);
        }
        
        enet_host_service(gNetENetServer, NULL, 0);
    }
    else if (gNetMode == NETWORK_CLIENT)
    {
        ENetEvent event;
        sprintf(buffer, "Connecting to %s:%u", gNetAddress, gNetPort);
        LOG_F(INFO, "%s", buffer);
        if (!bConsole)
        {
            viewLoadingScreen(gMenuPicnum, "Network Game", NULL, buffer);
            videoNextPage();
        }
        gNetENetClient = enet_host_create(NULL, 1, BLOOD_ENET_CHANNEL_MAX, 0, 0);
        enet_address_set_host(&gNetENetAddress, gNetAddress);
        gNetENetAddress.port = gNetPort;
        gNetENetPeer = enet_host_connect(gNetENetClient, &gNetENetAddress, BLOOD_ENET_CHANNEL_MAX, 0);
        if (!gNetENetPeer)
        {
            LOG_F(ERROR, "No available peers for initiating an ENet connection.");
            netDeinitialize();
            netResetToSinglePlayer();
            return;
        }
        if (enet_host_service(gNetENetClient, &event, 5000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT)
            LOG_F(INFO, "Connected to %s:%i", gNetAddress, gNetPort);
        else
        {
            LOG_F(ERROR, "Could not connect to %s:%i", gNetAddress, gNetPort);
            netDeinitialize();
            netResetToSinglePlayer();
            return;
        }
        bool bWaitServer = true;
        if (!bConsole)
        {
            viewLoadingScreen(gMenuPicnum, "Network Game", NULL, "Waiting for server response");
            videoNextPage();
        }
        while (bWaitServer)
        {
            handleevents();
            if (quitevent)
            {
                netClientDisconnect();
                QuitGame();
            }
            if (!bConsole && KB_KeyPressed(sc_Escape))
            {
                netClientDisconnect();
                netDeinitialize();
                netResetToSinglePlayer();
                return;
            }
            enet_host_service(gNetENetClient, NULL, 0);
            if (enet_host_check_events(gNetENetClient, &event) > 0)
            {
                switch (event.type)
                {
                case ENET_EVENT_TYPE_DISCONNECT:
                    LOG_F(ERROR, "Lost connection to server");
                    netDeinitialize();
                    netResetToSinglePlayer();
                    return;
                case ENET_EVENT_TYPE_RECEIVE:
                    //LOG_F(INFO, "NETEVENT");
                    if (event.channelID == BLOOD_ENET_SERVICE)
                    {
                        char *pPacket = (char*)event.packet->data;
                        switch (GetPacketByte(pPacket))
                        {
                        case BLOOD_SERVICE_CONNECTINFO:
                        {
                            PKT_CONNECTINFO *connectinfo = (PKT_CONNECTINFO*)pPacket;
                            gInitialNetPlayers = gNetPlayers = numplayers = connectinfo->numplayers;
                            connecthead = connectinfo->connecthead;
                            for (int i = 0; i < numplayers; i++)
                                connectpoint2[i] = connectinfo->connectpoint2[i];
                            //LOG_F(INFO, "BLOOD_SERVICE_CONNECTINFO");
                            break;
                        }
                        case BLOOD_SERVICE_CONNECTID:
                            dassert(numplayers > 1);
                            myconnectindex = GetPacketByte(pPacket);
                            bWaitServer = false;
                            //LOG_F(INFO, "BLOOD_SERVICE_CONNECTID");
                            break;
                        }
                    }
                    enet_packet_destroy(event.packet);
                    break;
                default:
                    break;
                }
            }
        }
        enet_host_service(gNetENetClient, NULL, 0);
        LOG_F(INFO, "Successfully connected to server");
    }
    gNetENetInit = true;
    gGameOptions.nGameType = kGameTypeBloodBath;
#else
    netResetToSinglePlayer();
#endif
}

void netDeinitialize(void)
{
#ifndef NETCODE_DISABLE
    gNetENetInit = false;
    if (gNetMode != NETWORK_NONE)
    {
        if (gNetENetServer)
        {
            netServerDisconnect();
            enet_host_destroy(gNetENetServer);
        }
        else if (gNetENetClient)
        {
            netClientDisconnect();
            enet_host_destroy(gNetENetClient);
        }
        enet_deinitialize();
    }
    gNetENetServer = gNetENetClient = NULL;
#endif
}

#ifndef NETCODE_DISABLE
void netPostEPacket(ENetPacket *pEPacket)
{
    //static int number;
    //LOG_F(INFO, "netPostEPacket %i", number++);
    gNetPacketFifo[gNetPacketHead] = pEPacket;
    gNetPacketHead = (gNetPacketHead+1)%kENetFifoSize;
}
#endif

void netUpdate(void)
{
#ifndef NETCODE_DISABLE
    ENetEvent event;
    if (gNetMode == NETWORK_NONE)
        return;

    if (gNetENetServer)
    {
        enet_host_service(gNetENetServer, NULL, 0);
        while (enet_host_check_events(gNetENetServer, &event) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_DISCONNECT:
            {
                int nPlayer;
                for (nPlayer = connectpoint2[connecthead]; nPlayer >= 0; nPlayer = connectpoint2[nPlayer])
                    if (gNetPlayerPeer[nPlayer] == event.peer)
                        break;

                for (int p = 0; p < kMaxPlayers; p++)
                    if (gNetPlayerPeer[p] == event.peer)
                        gNetPlayerPeer[p] = NULL;
                if (nPlayer >= 0)
                {
                    // TODO: Game most likely will go out of sync here...
                    char *pPacket = packet;
                    PutPacketByte(pPacket, 249);
                    PutPacketDWord(pPacket, nPlayer);
                    netSendPacketAll(packet, pPacket - packet);
                    netPlayerQuit(nPlayer);
                    netWaitForEveryone(0);
                }
                if (gNetPlayers <= 1)
                {
                    netDeinitialize();
                    netResetToSinglePlayer();
                    return;
                }
                break;
            }
            case ENET_EVENT_TYPE_RECEIVE:
                switch (event.channelID)
                {
                case BLOOD_ENET_SERVICE:
                {
                    char *pPacket = (char*)event.packet->data;
                    if (GetPacketByte(pPacket) != BLOOD_SERVICE_SENDTOID)
                        ThrowError("Packet error\n");
                    int nDest = GetPacketByte(pPacket);
                    int nSource = GetPacketByte(pPacket);
                    int nSize = event.packet->dataLength-3;
                    if (gNetPlayerPeer[nDest] != NULL)
                    {
                        ENetPacket *pNetPacket = enet_packet_create(NULL, nSize + 1, ENET_PACKET_FLAG_RELIABLE);
                        char *pPBuffer = (char*)pNetPacket->data;
                        PutPacketByte(pPBuffer, nSource);
                        PutPacketBuffer(pPBuffer, pPacket, nSize);
                        enet_peer_send(gNetPlayerPeer[nDest], BLOOD_ENET_GAME, pNetPacket);
                        enet_host_service(gNetENetServer, NULL, 0);
                    }
                    enet_packet_destroy(event.packet);
                    break;
                }
                case BLOOD_ENET_GAME:
                    netPostEPacket(event.packet);
                    break;
                }
            default:
                break;
            }
            enet_host_service(gNetENetServer, NULL, 0);
        }
        enet_host_service(gNetENetServer, NULL, 0);
    }
    else if (gNetENetClient)
    {
        enet_host_service(gNetENetClient, NULL, 0);
        while (enet_host_check_events(gNetENetClient, &event) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_DISCONNECT:
                LOG_F(ERROR, "Lost connection to server");
                netDeinitialize();
                netResetToSinglePlayer();
                gQuitGame = gRestartGame = true;
                return;
            case ENET_EVENT_TYPE_RECEIVE:
                switch (event.channelID)
                {
                case BLOOD_ENET_SERVICE:
                {
                    ThrowError("Packet error\n");
                    break;
                }
                case BLOOD_ENET_GAME:
                    netPostEPacket(event.packet);
                    break;
                }
            default:
                break;
            }
            enet_host_service(gNetENetClient, NULL, 0);
        }
        enet_host_service(gNetENetClient, NULL, 0);
    }
#endif
}

void faketimerhandler(void)
{
#ifndef NETCODE_DISABLE
    if (gNetMode != NETWORK_NONE && gNetENetInit)
        netUpdate();
#endif
    //if (gNetMode != NETWORK_NONE && gNetENetInit)
    //    enet_host_service(gNetMode == NETWORK_SERVER ? gNetENetServer : gNetENetClient, NULL, 0);
}

void netPlayerQuit(int nPlayer)
{
    char buffer[128];
    sprintf(buffer, "%s left the game with %d frags.", gProfile[nPlayer].name, gPlayer[nPlayer].fragCount);
    viewSetMessage(buffer);
    if (gGameStarted)
    {
        seqKill(3, gPlayer[nPlayer].pSprite->extra);
        actPostSprite(gPlayer[nPlayer].nSprite, kStatFree);
        if (nPlayer == gViewIndex)
            gViewIndex = myconnectindex;
        gView = &gPlayer[gViewIndex];
    }
    if (nPlayer == connecthead)
    {
        connecthead = connectpoint2[connecthead];
        //if (gPacketMode == PACKETMODE_1)
        {
            //byte_27B2CC = 1;
            gQuitGame = true;
            gRestartGame = true;
            gNetPlayers = 1;
            gQuitRequest = gNetMode == NETWORK_SERVER ? gQuitRequest : 2;
        }
    }
    else
    {
        for (int p = connecthead; p >= 0; p = connectpoint2[p])
        {
            if (connectpoint2[p] == nPlayer)
                connectpoint2[p] = connectpoint2[nPlayer];
        }
#ifndef NETCODE_DISABLE
        gNetPlayerPeer[nPlayer] = NULL;
#endif
    }
    gNetPlayers--;
    numplayers = ClipLow(numplayers-1, 1);
    if (gNetPlayers <= 1)
    {
        netDeinitialize();
        netResetToSinglePlayer();
    }
}
