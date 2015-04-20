/*****************************************************************************
*
*  PROJECT:     Multi Theft Auto v1.0
*  LICENSE:     See LICENSE in the top level directory
*  FILE:        mods/deathmatch/ClientCommands.cpp
*  PURPOSE:     Client commands handler class
*  DEVELOPERS:  Christian Myhre Lundheim <>
*               Cecill Etheredge <ijsf@gmx.net>
*               Jax <>
*
*  Multi Theft Auto is available from http://www.multitheftauto.com/
*
*****************************************************************************/

#include "StdInc.h"

using std::list;
using std::vector;

#ifdef MTA_DEBUG
#include <Tlhelp32.h>
#include <Psapi.h>
#include <shlwapi.h>
#include <Utils.h>
#endif

// Hide the "conversion from 'unsigned long' to 'DWORD*' of greater size" warning
#pragma warning(disable:4312)

extern CClientGame* g_pClientGame;

bool COMMAND_Executed ( const char* szCommand, const char* szArguments, bool bHandleRemotely, bool bHandled, bool bIsScriptedBind )
{
    // Has the core already handled this command?
    if ( !bHandled )
    {
        //char szBuffer [256];
        CLuaArguments Arguments;

        const char* szCommandBufferPointer = szCommand;

        if ( !bHandleRemotely )
        {
            // Is the command "say" and the arguments start with '/' ? (command comes from the chatbox)
            if ( stricmp ( szCommand, "chatboxsay" ) == 0 )
            {
                /* This code seems redundant, the chatbox now properly simulates commands.
                // His line starts with '/'?
                if ( *szArguments == '/' )
                {
                    // Copy the characters after the slash to the 0 terminator to a seperate buffer
                    strncpy ( szBuffer, &szArguments [ 1 ], 256 );
                    szBuffer [ 255 ] = 0;

                    // Split it into command and arguments
                    char* szNewCommand = strtok ( szBuffer, " " );
                    char* szNewArguments = strtok ( NULL, "\0" );
                    if ( szNewCommand )
                    {
                        // Execute it as another command
                        if ( szNewArguments )
                        {
                            g_pCore->GetCommands ()->Execute ( szNewCommand, szNewArguments );
                        }
                        else
                        {
                            g_pCore->GetCommands ()->Execute ( szNewCommand, "" );
                        }

                        return true;
                    }
                }
                */
                szCommandBufferPointer = "say";
            }
        }

        // Toss them together so we can send it to the server
        SString strClumpedCommand;
        if ( szArguments && szArguments [ 0 ] )
            strClumpedCommand.Format ( "%s %s", szCommandBufferPointer, szArguments );
        else
            strClumpedCommand = szCommandBufferPointer;

        // Convert to Unicode, and clamp it to a maximum command length
        std::wstring strClumpedCommandUTF = MbUTF8ToUTF16(strClumpedCommand.c_str());
        strClumpedCommandUTF = strClumpedCommandUTF.substr(0,MAX_COMMAND_LENGTH);
        strClumpedCommand = UTF16ToMbUTF8(strClumpedCommandUTF);

        g_pClientGame->GetRegisteredCommands ()->ProcessCommand ( szCommandBufferPointer, szArguments );

        // Call the onClientConsole event
        Arguments.PushString ( strClumpedCommand );

        // Call the event on the local player's onClientConsole first
        if ( g_pClientGame->GetLocalPlayer () )
            g_pClientGame->GetLocalPlayer ()->CallEvent ( "onClientConsole", Arguments, true );

        // Write the chatlength and the content
        NetBitStreamInterface* pBitStream = g_pNet->AllocateNetBitStream ();
        if ( !pBitStream ) 
            return false;

        // Write it to the bitstream
        pBitStream->Write ( strClumpedCommand.c_str(), static_cast < int > ( strlen ( strClumpedCommand.c_str() ) ) );

        // Send the packet to the server and free it
        g_pNet->SendPacket ( PACKET_ID_COMMAND, pBitStream, PACKET_PRIORITY_HIGH, PACKET_RELIABILITY_RELIABLE_ORDERED, PACKET_ORDERING_CHAT );
        g_pNet->DeallocateNetBitStream ( pBitStream );

        return true;
    }
    else
    {
        // Call our comand-handlers for core-executed commands too
        g_pClientGame->GetRegisteredCommands ()->ProcessCommand ( szCommand, szArguments );
    }
    return false;
}


void COMMAND_ChatBox ( const char* szInCmdLine )
{
    if ( !(szInCmdLine && szInCmdLine[0]) )
        return;

    COPY_CSTR_TO_TEMP_BUFFER( szCmdLine, szInCmdLine, 256 );

    // Split it up into command and rgb
    char* szCommand = strtok ( szCmdLine, " " );
    char* szRed = strtok ( NULL, " " );
    char* szGreen = strtok ( NULL, " " );
    char* szBlue = strtok ( NULL, " " );
    unsigned char ucRed = 0, ucGreen = 0, ucBlue = 0;

    if ( !szCommand )
        return;

    if ( szRed && szGreen && szBlue )
    {     
        ucRed = static_cast < unsigned char > ( atoi ( szRed ) );
        ucGreen = static_cast < unsigned char > ( atoi ( szGreen ) );
        ucBlue = static_cast < unsigned char > ( atoi ( szBlue ) );
    }

    // Open the chatbox input with command and color
    g_pCore->EnableChatInput ( szCommand, COLOR_ARGB ( 255, ucRed, ucGreen, ucBlue ) );
}


void COMMAND_Help ( const char *szCmdLine )
{
    // This isnt used
}


void COMMAND_Disconnect ( const char *szCmdLine )
{
    if ( g_pClientGame->IsWaitingForLocalConnect() )
        g_pCore->RemoveMessageBox ();

    AddReportLog( 7110, "Game - COMMAND_Disconnect" );
    g_pCore->GetModManager ()->RequestUnload ();
}

void COMMAND_ShowNametags ( const char* szCmdLine )
{
    int iCmd = ( szCmdLine && szCmdLine [ 0 ] ) ? atoi ( szCmdLine ) : -1;
    bool bShow = ( iCmd == 1 ) ? true : ( iCmd == 0 ) ? false : !g_pClientGame->GetNametags ()->IsVisible ();
    g_pClientGame->GetNametags ()->SetVisible ( bShow );
}

void COMMAND_ShowChat ( const char* szCmdLine )
{
    int iCmd = ( szCmdLine && szCmdLine [ 0 ] ) ? atoi ( szCmdLine ) : -1;
    bool bShow = ( iCmd == 1 ) ? true : ( iCmd == 0 ) ? false : !g_pCore->IsChatVisible ();
    g_pCore->SetChatVisible ( bShow );
}

void COMMAND_ShowNetstat ( const char* szCmdLine )
{
    int iCmd = ( szCmdLine && szCmdLine [ 0 ] ) ? atoi ( szCmdLine ) : -1;
    g_pClientGame->ShowNetstat ( iCmd );
}

void COMMAND_Eaeg ( const char* szCmdLine )
{
    g_pClientGame->ShowEaeg ( true );
}

void COMMAND_EnterPassenger ( const char* szCmdLine )
{
    // HACK: we don't want them to enter a vehicle if they're in cursor mode
    if ( g_pClientGame && !g_pClientGame->AreCursorEventsEnabled () )
    {
        // Disable passenger entry while reloading so it doesen't abort the animation
        CClientPlayer* pPlayer = g_pClientGame->GetPlayerManager ()->GetLocalPlayer ();
        if ( pPlayer )
        {
            CWeapon* pWeapon = pPlayer->GetWeapon ();
            if ( pWeapon )
            {
                if ( pWeapon->GetState() != WEAPONSTATE_RELOADING )
                {
                    g_pClientGame->ProcessVehicleInOutKey ( true );
                }
            }
        }
    }
}


void COMMAND_RadioNext ( const char* szCmdLine )
{
    if ( g_pClientGame )
    {
        CClientPlayer* pPlayer = g_pClientGame->GetPlayerManager ()->GetLocalPlayer ();
        if ( pPlayer )
        {
            pPlayer->NextRadioChannel ();
        }
    }
}


void COMMAND_RadioPrevious ( const char* szCmdLine )
{
    if ( g_pClientGame )
    {
        CClientPlayer* pPlayer = g_pClientGame->GetPlayerManager ()->GetLocalPlayer ();
        if ( pPlayer )
        {
            pPlayer->PreviousRadioChannel ();
        }
    }
}

/*
void COMMAND_Screenshot ( const char* szCmdLine )
{
    g_pClientGame->GetScreenshot ()->SetScreenshotKey ( true );
}
*/

void COMMAND_RadarMap ( const char* szCmdLine )
{
    int iCmd = ( szCmdLine && szCmdLine [ 0 ] ) ? atoi ( szCmdLine ) : -1;
    bool bShow = ( iCmd == 1 ) ? true : ( iCmd == 0 ) ? false : !g_pClientGame->GetRadarMap ()->GetRadarEnabled ();
    g_pClientGame->GetRadarMap ()->SetRadarEnabled ( bShow );
}

void COMMAND_RadarZoomIn ( const char* szCmdLine )
{
    CRadarMap* pRadarMap = g_pClientGame->GetRadarMap ();

    if ( pRadarMap->IsRadarShowing () )
    {
        pRadarMap->ZoomIn ();
    }
}

void COMMAND_RadarZoomOut ( const char* szCmdLine )
{
    CRadarMap* pRadarMap = g_pClientGame->GetRadarMap ();

    if ( pRadarMap->IsRadarShowing () )
    {
        pRadarMap->ZoomOut ();
    }
}

void COMMAND_RadarMoveNorth ( const char* szCmdLine )
{
    CRadarMap* pRadarMap = g_pClientGame->GetRadarMap ();

    if ( pRadarMap->IsRadarShowing () )
    {
        // Toggle on/off
        if ( pRadarMap->IsMovingNorth () )
            pRadarMap->SetMovingNorth ( false );
        else if ( pRadarMap->IsMovingSouth () )
            pRadarMap->SetMovingSouth ( false );
        else
        {
            pRadarMap->SetMovingNorth ( true );
            pRadarMap->SetMovingSouth ( false );
            pRadarMap->SetMovingEast ( false );
            pRadarMap->SetMovingWest ( false );
        }
    }
}

void COMMAND_RadarMoveSouth ( const char* szCmdLine )
{
    CRadarMap* pRadarMap = g_pClientGame->GetRadarMap ();

    if ( pRadarMap->IsRadarShowing () )
    {
        // Toggle on/off
        if ( pRadarMap->IsMovingSouth () )
            pRadarMap->SetMovingSouth ( false );
        else if ( pRadarMap->IsMovingNorth () )
            pRadarMap->SetMovingNorth ( false );
        else
        {
            pRadarMap->SetMovingNorth ( false );
            pRadarMap->SetMovingSouth ( true );
            pRadarMap->SetMovingEast ( false );
            pRadarMap->SetMovingWest ( false );
        }
    }
}

void COMMAND_RadarMoveEast ( const char* szCmdLine )
{
    CRadarMap* pRadarMap = g_pClientGame->GetRadarMap ();

    if ( pRadarMap->IsRadarShowing () )
    {
        // Toggle on/off
        if ( pRadarMap->IsMovingEast () )
            pRadarMap->SetMovingEast ( false );
        else if ( pRadarMap->IsMovingWest () )
            pRadarMap->SetMovingWest ( false );
        else
        {
            pRadarMap->SetMovingNorth ( false );
            pRadarMap->SetMovingSouth ( false );
            pRadarMap->SetMovingEast ( true );
            pRadarMap->SetMovingWest ( false );
        }
    }
}

void COMMAND_RadarMoveWest ( const char* szCmdLine )
{
    CRadarMap* pRadarMap = g_pClientGame->GetRadarMap ();

    if ( pRadarMap->IsRadarShowing () )
    {
        // Toggle on/off
        if ( pRadarMap->IsMovingWest () )
            pRadarMap->SetMovingWest ( false );
        else if ( pRadarMap->IsMovingEast () )
            pRadarMap->SetMovingEast ( false );
        else
        {
            pRadarMap->SetMovingNorth ( false );
            pRadarMap->SetMovingSouth ( false );
            pRadarMap->SetMovingEast ( false );
            pRadarMap->SetMovingWest ( true );
        }
    }
}

void COMMAND_RadarAttach ( const char* szCmdLine )
{
    CRadarMap* pRadarMap = g_pClientGame->GetRadarMap ();

    if ( pRadarMap->IsRadarShowing () )
    {
        pRadarMap->SetAttachedToLocalPlayer ( !g_pClientGame->GetRadarMap ()->IsAttachedToLocalPlayer () );
    }
}

void COMMAND_RadarOpacityDown ( const char* szCmdLine )
{
    CRadarMap* pRadarMap = g_pClientGame->GetRadarMap ();
    if ( pRadarMap->IsRadarShowing () )
    {
        int iAlpha;
        g_pCore->GetCVars ()->Get ( "mapalpha", iAlpha );
        iAlpha = Max ( 0, iAlpha - 20 );
        g_pCore->GetCVars ()->Set ( "mapalpha", iAlpha );
    }
}

void COMMAND_RadarOpacityUp ( const char* szCmdLine )
{
    CRadarMap* pRadarMap = g_pClientGame->GetRadarMap ();
    if ( pRadarMap->IsRadarShowing () )
    {
        int iAlpha;
        g_pCore->GetCVars ()->Get ( "mapalpha", iAlpha );
        iAlpha = Min ( 255, iAlpha + 20 );
        g_pCore->GetCVars ()->Set ( "mapalpha", iAlpha );
    }
}

void COMMAND_RadarHelp ( const char* szCmdLine )
{
    g_pClientGame->GetRadarMap ()->ToggleHelpText ();
}

void COMMAND_MessageTarget ( const char* szCmdLine )
{
    if ( !(szCmdLine && szCmdLine[0]) )
        return;

    CClientPlayer* pTarget = g_pClientGame->GetTargetedPlayer ();
    if ( pTarget )
    {
        const char * szNick = pTarget->GetNick ();
        if ( !szNick )
            return;
        SString strParameters ( "%s %s", pTarget->GetNick (), szCmdLine );
        g_pCore->GetCommands ()->Execute ( "msg", strParameters );
    }
    else
        g_pCore->ChatEchoColor ( "Error: no player target found", 255, 168, 0 );
}

void COMMAND_VehicleNextWeapon ( const char* szCmdLine )
{
    g_pClientGame->ChangeVehicleWeapon ( true );
}

void COMMAND_VehiclePreviousWeapon ( const char* szCmdLine )
{
    g_pClientGame->ChangeVehicleWeapon ( false );
}

void COMMAND_TextScale ( const char* szCmdLine )
{
    if ( !(szCmdLine && szCmdLine [ 0 ]) )
        return;

    float fScale = ( float ) atof ( szCmdLine );
    if ( fScale >= 0.8f && fScale <= 3.0f )
    {
        CClientTextDisplay::SetGlobalScale ( fScale );
        g_pCore->GetCVars ()->Set ( "text_scale", fScale );
    }
}


void DumpPlayer ( CClientPlayer* pPlayer, FILE* pFile )
{
    unsigned int uiIndex = 0;

    // Player
    fprintf ( pFile, "%s\n", "*** START OF PLAYER ***" );

    // Write the data
    fprintf ( pFile, "Nick: %s\n", pPlayer->GetNick () );
    
    CVector vecTemp;
    pPlayer->GetPosition ( vecTemp );
    fprintf ( pFile, "Position: %f %f %f\n", vecTemp.fX, vecTemp.fY, vecTemp.fZ );

    fprintf ( pFile, "Nametag text: %s\n", pPlayer->GetNametagText () );

    unsigned char ucR, ucG, ucB;
    pPlayer->GetNametagColor ( ucR, ucG, ucB );
    fprintf ( pFile, "Nametag color: %u %u %u\n", ucR, ucG, ucB );

    fprintf ( pFile, "Nametag show: %u\n", pPlayer->IsNametagShowing () );

    fprintf ( pFile, "Local player: %u\n", pPlayer->IsLocalPlayer () );
    fprintf ( pFile, "Dead: %u\n", pPlayer->IsDead () );

    fprintf ( pFile, "Exp aim: %u\n", pPlayer->IsExtrapolatingAim () );
    fprintf ( pFile, "Latency: %u\n", pPlayer->GetLatency () );

    fprintf ( pFile, "Last psync time: %u\n", pPlayer->GetLastPuresyncTime () );
    fprintf ( pFile, "Has con trouble: %u\n\n", pPlayer->HasConnectionTrouble () );

    CClientTeam* pTeam = pPlayer->GetTeam ();
    if ( pTeam )
        fprintf ( pFile, "Team: %s\n", pTeam->GetTeamName () );

    // Get the matrix
    CMatrix matPlayer;
    pPlayer->GetMatrix ( matPlayer );

    fprintf ( pFile, "Matrix: vecRoll: %f %f %f\n", matPlayer.vRight.fX, matPlayer.vRight.fY, matPlayer.vRight.fZ );
    fprintf ( pFile, "        vecDir:  %f %f %f\n", matPlayer.vFront.fX, matPlayer.vFront.fY, matPlayer.vFront.fZ );
    fprintf ( pFile, "        vecWas:  %f %f %f\n", matPlayer.vUp.fX, matPlayer.vUp.fY, matPlayer.vUp.fZ );
    fprintf ( pFile, "        vecPos:  %f %f %f\n\n", matPlayer.vPos.fX, matPlayer.vPos.fY, matPlayer.vPos.fZ );

    fprintf ( pFile, "Euler rot: %f\n", pPlayer->GetCurrentRotation () );
    fprintf ( pFile, "Cam rot: %f\n", pPlayer->GetCameraRotation () );

    pPlayer->GetMoveSpeed ( vecTemp );
    fprintf ( pFile, "Move speed: %f %f %f\n", vecTemp.fX, vecTemp.fY, vecTemp.fZ );

    pPlayer->GetTurnSpeed ( vecTemp );
    fprintf ( pFile, "Turn speed: %f %f %f\n", vecTemp.fX, vecTemp.fY, vecTemp.fZ );

    CControllerState State;
    pPlayer->GetControllerState ( State );

    signed short* pController = reinterpret_cast < signed short* > ( &State );
    fprintf ( pFile, "CContr state: \n" );
    for ( uiIndex = 0; uiIndex < 36; uiIndex++ )
    {
        fprintf ( pFile, "State [%u] = %i\n", uiIndex, pController [uiIndex] );
    }

    pPlayer->GetLastControllerState ( State );

    pController = reinterpret_cast < signed short* > ( &State );
    fprintf ( pFile, "LContr state: \n" );
    for ( uiIndex = 0; uiIndex < 36; uiIndex++ )
    {
        fprintf ( pFile, "State [%u] = %i\n", uiIndex, pController [uiIndex] );
    }

    fprintf ( pFile, "\nVeh IO state: %i\n", pPlayer->GetVehicleInOutState () );
    fprintf ( pFile, "Visible: %u\n", pPlayer->IsVisible () );
    fprintf ( pFile, "Health: %f\n", pPlayer->GetHealth () );
    fprintf ( pFile, "Armor: %f\n", pPlayer->GetArmor () );
    fprintf ( pFile, "On screen: %u\n", pPlayer->IsOnScreen () );
    fprintf ( pFile, "Frozen: %u\n", pPlayer->IsFrozen () );
    fprintf ( pFile, "Respawn state: %i\n", pPlayer->GetRespawnState () );

    fprintf ( pFile, "Cur weapon slot: %i\n", static_cast < int > ( pPlayer->GetCurrentWeaponSlot () ) );
    fprintf ( pFile, "Cur weapon type: %i\n", static_cast < int > ( pPlayer->GetCurrentWeaponType () ) );

    CWeapon* pWeapon = pPlayer->GetWeapon ();
    if ( pWeapon )
    {
        fprintf ( pFile, "Cur weapon state: %i\n", static_cast < int > ( pWeapon->GetState () ) );
        fprintf ( pFile, "Cur weapon ammo clip: %u\n", pWeapon->GetAmmoInClip () );
        fprintf ( pFile, "Cur weapon ammo total: %u\n", pWeapon->GetAmmoTotal () );
    }

    CTaskManager* pTaskMgr = pPlayer->GetTaskManager ();
    if ( pTaskMgr )
    {
        // Primary task
        CTask* pTask = pTaskMgr->GetTask ( TASK_PRIORITY_PRIMARY );
        if ( pTask )
        {
            fprintf ( pFile, "Primary task name: %s\n", pTask->GetTaskName () );
            fprintf ( pFile, "Primary task type: %i\n", pTask->GetTaskType () );
        }
        else
            fprintf ( pFile, "Primary task: none\n" );

        // Physical response task
        pTask = pTaskMgr->GetTask ( TASK_PRIORITY_PHYSICAL_RESPONSE );
        if ( pTask )
        {
            fprintf ( pFile, "Physical response task name: %s\n", pTask->GetTaskName () );
            fprintf ( pFile, "Physical response task type: %i\n", pTask->GetTaskType () );
        }
        else
            fprintf ( pFile, "Physical response task: none\n" );

        // Event response task temp
        pTask = pTaskMgr->GetTask ( TASK_PRIORITY_EVENT_RESPONSE_TEMP );
        if ( pTask )
        {
            fprintf ( pFile, "Event rsp tmp task name: %s\n", pTask->GetTaskName () );
            fprintf ( pFile, "Event rsp tmp task type: %i\n", pTask->GetTaskType () );
        }
        else
            fprintf ( pFile, "Event rsp tmp task: none\n" );

        // Event response task nontemp
        pTask = pTaskMgr->GetTask ( TASK_PRIORITY_EVENT_RESPONSE_NONTEMP );
        if ( pTask )
        {
            fprintf ( pFile, "Event rsp nontmp task name: %s\n", pTask->GetTaskName () );
            fprintf ( pFile, "Event rsp nontmp task type: %i\n", pTask->GetTaskType () );
        }
        else
            fprintf ( pFile, "Event rsp nontmp task: none\n" );

        // Event response task nontemp
        pTask = pTaskMgr->GetTask ( TASK_PRIORITY_DEFAULT );
        if ( pTask )
        {
            fprintf ( pFile, "Default task name: %s\n", pTask->GetTaskName () );
            fprintf ( pFile, "Default task type: %i\n", pTask->GetTaskType () );
        }
        else
            fprintf ( pFile, "Default task: none\n" );

        // Secondary attack
        pTask = pTaskMgr->GetTaskSecondary ( TASK_SECONDARY_ATTACK );
        if ( pTask )
        {
            fprintf ( pFile, "Secondary attack task name: %s\n", pTask->GetTaskName () );
            fprintf ( pFile, "Secondary attack task type: %i\n", pTask->GetTaskType () );
        }
        else
            fprintf ( pFile, "Secondary attack task task: none\n" );

        // Secondary duck
        pTask = pTaskMgr->GetTaskSecondary ( TASK_SECONDARY_DUCK );
        if ( pTask )
        {
            fprintf ( pFile, "Secondary duck task name: %s\n", pTask->GetTaskName () );
            fprintf ( pFile, "Secondary duck task type: %i\n", pTask->GetTaskType () );
        }
        else
            fprintf ( pFile, "Secondary duck task task: none\n" );

        // Secondary say
        pTask = pTaskMgr->GetTaskSecondary ( TASK_SECONDARY_SAY );
        if ( pTask )
        {
            fprintf ( pFile, "Secondary say task name: %s\n", pTask->GetTaskName () );
            fprintf ( pFile, "Secondary say task type: %i\n", pTask->GetTaskType () );
        }
        else
            fprintf ( pFile, "Secondary say task task: none\n" );

        // Secondary facial complex
        pTask = pTaskMgr->GetTaskSecondary ( TASK_SECONDARY_FACIAL_COMPLEX );
        if ( pTask )
        {
            fprintf ( pFile, "Secondary facial task name: %s\n", pTask->GetTaskName () );
            fprintf ( pFile, "Secondary facial task type: %i\n", pTask->GetTaskType () );
        }
        else
            fprintf ( pFile, "Secondary facial task task: none\n" );

        // Secondary partial anim
        pTask = pTaskMgr->GetTaskSecondary ( TASK_SECONDARY_PARTIAL_ANIM );
        if ( pTask )
        {
            fprintf ( pFile, "Secondary partial anim task name: %s\n", pTask->GetTaskName () );
            fprintf ( pFile, "Secondary partial anim task type: %i\n", pTask->GetTaskType () );
        }
        else
            fprintf ( pFile, "Secondary partial anim task task: none\n" );

        // Secondary IK
        pTask = pTaskMgr->GetTaskSecondary ( TASK_SECONDARY_IK );
        if ( pTask )
        {
            fprintf ( pFile, "Secondary IK task name: %s\n", pTask->GetTaskName () );
            fprintf ( pFile, "Secondary IK task type: %i\n", pTask->GetTaskType () );
        }
        else
            fprintf ( pFile, "Secondary IK task task: none\n" );
    }

    float fX, fY;
    pPlayer->GetAim ( fX, fY );
    fprintf ( pFile, "Aim: %f %f\n", fX, fY );

    vecTemp = pPlayer->GetAimSource ();
    fprintf ( pFile, "Aim source: %f %f %f\n", vecTemp.fX, vecTemp.fY, vecTemp.fZ );

    vecTemp = pPlayer->GetAimTarget ();
    fprintf ( pFile, "Aim target: %f %f %f\n", vecTemp.fX, vecTemp.fY, vecTemp.fZ );

    fprintf ( pFile, "Veh aim anim: %u\n", pPlayer->GetVehicleAimAnim () );
    fprintf ( pFile, "Ducked: %u\n", pPlayer->IsDucked () );
    fprintf ( pFile, "Wearing googles: %u\n", pPlayer->IsWearingGoggles () );
    fprintf ( pFile, "Has jetpack: %u\n", pPlayer->HasJetPack () );
    fprintf ( pFile, "In water: %u\n", pPlayer->IsInWater () );
    fprintf ( pFile, "On ground: %u\n", pPlayer->IsOnGround () );
    fprintf ( pFile, "Is climbing: %u\n", pPlayer->IsClimbing () );
    fprintf ( pFile, "Interiour: %u\n", pPlayer->GetInterior () );
    fprintf ( pFile, "Fight style: %u\n", static_cast < int > ( pPlayer->GetFightingStyle () ) );
    fprintf ( pFile, "Satchel count: %u\n", pPlayer->CountProjectiles ( WEAPONTYPE_REMOTE_SATCHEL_CHARGE ) );

    CRemoteDataStorage* pRemote = pPlayer->GetRemoteData ();
    if ( pRemote )
    {
        vecTemp = pRemote->GetAkimboTarget ();
        fprintf ( pFile, "Akimbo target: %f %f %f\n", vecTemp.fX, vecTemp.fY, vecTemp.fZ );
        fprintf ( pFile, "Akimbo aim up: %u\n", pRemote->GetAkimboTargetUp () );
    }
    else
        fprintf ( pFile, "Remote: no\n" );

    fprintf ( pFile, "Using gun: %u\n", pPlayer->IsUsingGun () );
    fprintf ( pFile, "Entering veh: %u\n", pPlayer->IsEnteringVehicle () );
    fprintf ( pFile, "Getting jacked: %u\n", pPlayer->IsGettingJacked () );
    fprintf ( pFile, "Alpha: %u\n", pPlayer->GetAlpha () );
    

    fprintf ( pFile, "Stats:\n" );

    for ( uiIndex = 0; uiIndex < NUM_PLAYER_STATS; uiIndex++ )
    {
        fprintf ( pFile, "Stat [%u] = %f\n", uiIndex, pPlayer->GetStat ( uiIndex ) );
    }

    fprintf ( pFile, "Streamed in: %u\n", pPlayer->IsStreamedIn () );


    fprintf ( pFile, "Model: %u\n", pPlayer->GetModel () );

    // Write model data
    CModelInfo* pInfo = g_pGame->GetModelInfo ( pPlayer->GetModel () );
    fprintf ( pFile, "Model ref count: %i\n", pInfo->GetRefCount () );
    fprintf ( pFile, "Model loaded: %u\n", pInfo->IsLoaded () );
    fprintf ( pFile, "Model valid: %u\n", pInfo->IsValid () );

    // End of player
    fprintf ( pFile, "%s", "\n*** END OF PLAYER ***\n\n\n\n" );
}


void COMMAND_DumpPlayers ( const char* szCmdLine )
{
    // Create a file to dump to
    SString strBuffer ( "%s/dump_%i.txt", g_pClientGame->GetModRoot (), GetTickCount32 () );
    FILE* pFile = fopen ( strBuffer, "w+" );
    if ( pFile )
    {
        // Write time now
        fprintf ( pFile, "Comments: %s\n", szCmdLine );
        fprintf ( pFile, "Time now: %u\n\n", CClientTime::GetTime () );
        fprintf ( pFile, "Objectcount: %u\n", g_pClientGame->GetObjectManager ()->Count () );
        fprintf ( pFile, "Playercount: %u\n", g_pClientGame->GetPlayerManager ()->Count () );
        fprintf ( pFile, "Vehiclecount: %u\n\n", g_pClientGame->GetVehicleManager ()->Count () );

        fprintf ( pFile, "Used building pool: %u\n", g_pGame->GetPools ()->GetNumberOfUsedSpaces ( BUILDING_POOL ) );
        fprintf ( pFile, "Used ped pool: %u\n", g_pGame->GetPools ()->GetNumberOfUsedSpaces ( PED_POOL ) );
        fprintf ( pFile, "Used object pool: %u\n", g_pGame->GetPools ()->GetNumberOfUsedSpaces ( OBJECT_POOL ) );
        fprintf ( pFile, "Used dummy pool: %u\n", g_pGame->GetPools ()->GetNumberOfUsedSpaces ( DUMMY_POOL ) );
        fprintf ( pFile, "Used vehicle pool: %u\n", g_pGame->GetPools ()->GetNumberOfUsedSpaces ( VEHICLE_POOL ) );
        fprintf ( pFile, "Used col model pool: %u\n", g_pGame->GetPools ()->GetNumberOfUsedSpaces ( COL_MODEL_POOL ) );
        fprintf ( pFile, "Used task pool: %u\n", g_pGame->GetPools ()->GetNumberOfUsedSpaces ( TASK_POOL ) );
        fprintf ( pFile, "Used event pool: %u\n", g_pGame->GetPools ()->GetNumberOfUsedSpaces ( EVENT_POOL ) );
        fprintf ( pFile, "Used task alloc pool: %u\n", g_pGame->GetPools ()->GetNumberOfUsedSpaces ( TASK_ALLOCATOR_POOL ) );
        fprintf ( pFile, "Used ped intelli pool: %u\n", g_pGame->GetPools ()->GetNumberOfUsedSpaces ( PED_INTELLIGENCE_POOL ) );
        fprintf ( pFile, "Used ped attractor pool: %u\n", g_pGame->GetPools ()->GetNumberOfUsedSpaces ( PED_ATTRACTOR_POOL ) );
        fprintf ( pFile, "Used entry info node pool: %u\n", g_pGame->GetPools ()->GetNumberOfUsedSpaces ( ENTRY_INFO_NODE_POOL ) );
        fprintf ( pFile, "Used node route pool: %u\n", g_pGame->GetPools ()->GetNumberOfUsedSpaces ( NODE_ROUTE_POOL ) );
        fprintf ( pFile, "Used patrol route pool: %u\n", g_pGame->GetPools ()->GetNumberOfUsedSpaces ( PATROL_ROUTE_POOL ) );
        fprintf ( pFile, "Used point route pool: %u\n", g_pGame->GetPools ()->GetNumberOfUsedSpaces ( POINT_ROUTE_POOL ) );
        fprintf ( pFile, "Used point double link pool: %u\n", g_pGame->GetPools ()->GetNumberOfUsedSpaces ( POINTER_DOUBLE_LINK_POOL ) );
        fprintf ( pFile, "Used point single link pool: %u\n\n\n", g_pGame->GetPools ()->GetNumberOfUsedSpaces ( POINTER_SINGLE_LINK_POOL ) );

        // Loop through all players
        vector < CClientPlayer* > ::const_iterator iter = g_pClientGame->GetPlayerManager ()->IterBegin ();
        for ( ; iter != g_pClientGame->GetPlayerManager ()->IterEnd (); iter++ )
        {
            // Write the player dump
            DumpPlayer ( *iter, pFile );
        }

        // End of the dump. Close it
        fclose ( pFile );
        g_pCore->GetConsole ()->Print ( "dumpplayers: Dumping successfull" );
    }
    else
        g_pCore->GetConsole ()->Print ( "dumpplayers: Unable to create file" );
}


void COMMAND_ShowSyncData ( const char* szCmdLine )
{
    // Grab the player
    CClientPlayer* pPlayer = g_pClientGame->GetPlayerManager ()->Get ( szCmdLine );
    if ( pPlayer )
    {
        g_pClientGame->GetSyncDebug ()->Attach ( *pPlayer );
    }
    else
    {
        g_pClientGame->GetSyncDebug ()->Detach ();
    }
}

void COMMAND_VoicePushToTalk ( const char* szCmdLine )
{
    if ( g_pClientGame->GetVoiceRecorder()->IsEnabled() )
        g_pClientGame->GetVoiceRecorder()-> UpdatePTTState ( atoi(szCmdLine) );
    else
    {
        // Show warning only once per server
        static bool bDone = false;
        if ( !bDone )
        {
            bDone = true;
            g_pCore->GetConsole ()->Print ( "voiceptt: This server does not have voice enabled" );
        }
    }
}

void COMMAND_ServerInfo ( const char* szCmdLine )
{
    SString strSpacer;
    strSpacer.insert( 0, 80, '-' );
    g_pCore->GetConsole ()->Print ( *strSpacer );
    g_pClientGame->OutputServerInfo();
    g_pCore->GetConsole ()->Print ( *strSpacer );
}

#if defined (MTA_DEBUG) || defined (MTA_BETA)

void COMMAND_ShowSyncing ( const char *szCmdLine )
{
    g_pClientGame->ShowSyncingInfo ( atoi ( szCmdLine ) == 1 );
}

#endif

#ifdef MTA_DEBUG

void COMMAND_Foo ( const char* szCmdLine )
{
    g_pClientGame->m_Foo.Test ( szCmdLine );
}

#endif

#if defined(MTA_DEBUG) || defined(MTA_DEBUG_COMMANDS)
void COMMAND_ShowWepdata ( const char* szCmdLine )
{
    if ( !(szCmdLine && szCmdLine[0]) )
        return;
    g_pClientGame->ShowWepdata ( szCmdLine );
}

void COMMAND_ShowTasks ( const char* szCmdLine )
{
    if ( !(szCmdLine && szCmdLine[0]) )
        return;
    g_pClientGame->ShowTasks ( szCmdLine );
}

void COMMAND_ShowPlayer ( const char* szCmdLine )
{
    if ( !(szCmdLine && szCmdLine[0]) )
        return;
    g_pClientGame->ShowPlayer ( szCmdLine );
}

void COMMAND_SetMimic ( const char* szCmdLine )
{
    if ( !(szCmdLine && szCmdLine[0]) )
        return;
    g_pClientGame->SetMimic ( atoi ( szCmdLine ) );
}

void COMMAND_SetMimicLag ( const char* szCmdLine )
{
    if ( !(szCmdLine && szCmdLine[0]) )
        return;
    g_pClientGame->SetMimicLag ( atoi ( szCmdLine ) == 1 );
}

void COMMAND_Paintballs ( const char* szCmdLine )
{
    if ( !(szCmdLine && szCmdLine[0]) )
        return;
    g_pClientGame->SetDoPaintballs ( atoi ( szCmdLine ) == 1 );
}

void COMMAND_Breakpoint ( const char* szCmdLine )
{
    if ( !(szCmdLine && szCmdLine[0]) )
        return;
    _asm int 3
    // Make our main pointer easily accessable
    // Added by slush:  You're a lazy ass if you use this.
    g_pClientGame;
}


void COMMAND_GiveWeapon ( const char *szCmdLine )
{
    if ( !(szCmdLine && szCmdLine[0]) )
        return;

    int nWeaponID = atoi(szCmdLine);
    /*
     * Check validity of the command line weapon id.
     */

    if ( !CClientPickupManager::IsValidWeaponID ( nWeaponID ) )
        return;

    CClientPed * pPed = g_pClientGame->GetManager()->GetPlayerManager()->GetLocalPlayer();
    if ( pPed )
    {
        CWeapon * pPlayerWeapon = pPed->GiveWeapon ( (eWeaponType)nWeaponID, 9999 );
        if ( pPlayerWeapon )
        {
            pPlayerWeapon->SetAsCurrentWeapon();
        }
    }
}

void COMMAND_ShowRPCs ( const char* szCmdLine )
{
    if ( !(szCmdLine && szCmdLine[0]) )
        return;
    g_pClientGame->GetRPCFunctions ()->m_bShowRPCs = ( atoi ( szCmdLine ) == 1 );
}


void COMMAND_ShowInterpolation ( const char* )
{
    g_pClientGame->ShowInterpolation ( ! g_pClientGame->IsShowingInterpolation () );
}

void COMMAND_Watch ( const char *szCmdLine )
{
    // Note: This code might be a little unsafe if the detouring done by the DLL happens to be done
    //       exactly on a call to WriteProcessMemory even though the chance is small.
    // adds a hook to a process and watches for WPMs to this one
    DWORD dwProcessIDs[250];
    DWORD pBytesReturned = 0;
    unsigned int uiListSize = 50;
    if ( EnumProcesses ( dwProcessIDs, 250 * sizeof(DWORD), &pBytesReturned ) )
    {
        for ( unsigned int i = 0; i < pBytesReturned / sizeof ( DWORD ); i++ )
        {
            // Open the process
            HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, 0, dwProcessIDs[i]);
            if ( hProcess )
            {
                HMODULE pModule;
                DWORD cbNeeded;
                if ( EnumProcessModules ( hProcess, &pModule, sizeof ( HMODULE ), &cbNeeded ) )
                {
                    char szModuleName[500];
                    if ( GetModuleFileNameEx( hProcess, pModule, szModuleName, 500 ) )
                    {
                        if ( stricmp ( szModuleName + strlen(szModuleName) - strlen(szCmdLine), szCmdLine ) == 0 )
                        {
                            g_pCore->GetConsole()->Printf ( "Attaching to %s with process id %d...", szModuleName, hProcess );
                            RemoteLoadLibrary ( hProcess, "C:/Program Files/Rockstar Games/GTA San Andreas/mta/wpmhookdll.dll" );
                            CloseHandle ( hProcess );
                            return;
                        } 
                    }
                }

                // Close the process
                CloseHandle ( hProcess );
            }
        }
    }
}


void COMMAND_Modules ( const char *szCmdLine )
{
    // Get the base address of the requested module
    // Take a snapshot of all modules in the specified process. 
    HANDLE hModuleSnap = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE, GetCurrentProcessId () ); 
    if ( hModuleSnap != INVALID_HANDLE_VALUE ) 
    {
        //  Set the size of the structure before using it. 
        MODULEENTRY32 ModuleEntry; 
        ModuleEntry.dwSize = sizeof ( MODULEENTRY32 ); 

        // Retrieve information about the first module, 
        // and exit if unsuccessful 
        if ( Module32First ( hModuleSnap, &ModuleEntry ) ) 
        {
            // Create a file
            FILE* pFile = fopen ( "modules.txt", "w+" );
            if ( pFile )
            {
                // Now walk the module list of the process, 
                // and display information about each module 
                do
                {
                    // Print it
                    fprintf ( pFile, "** MODULE **\n"
                                     "Name: %s\n"
                                     "Base: 0x%P\n"
                                     "Size: 0x%P\n"
                                     "\n",
                                     ModuleEntry.szModule,
                                     ModuleEntry.modBaseAddr,
                                     ModuleEntry.modBaseSize );
                }
                while ( Module32Next ( hModuleSnap, &ModuleEntry ) );

                // Close it
                fclose ( pFile );
            }
        }

        // Close the snapshot object
        CloseHandle( hModuleSnap ); 
    }
}

#include <CClientCorona.h>
CClientPickup* pPickupTest = NULL;
CClientCorona* pCoronaTest = NULL;
CVehicle * debugTrain = NULL;
CClientPlayer* pRonkert = NULL;
CObject * obj = NULL;

void COMMAND_Debug ( const char *szCmdLine )
{
    __debugbreak();

    return;

    CPools* pPools = g_pGame->GetPools ();
    int iEntryInfoNodeEntries = pPools->GetEntryInfoNodePool ()->GetNumberOfUsedSpaces ();
    int iPointerNodeSingleLinkEntries = pPools->GetPointerNodeSingleLinkPool ()->GetNumberOfUsedSpaces ();
    int iPointerNodeDoubleLinkEntries = pPools->GetPointerNodeDoubleLinkPool ()->GetNumberOfUsedSpaces ();

    g_pCore->GetConsole ()->Printf ( "entry info: %i", iEntryInfoNodeEntries );
    g_pCore->GetConsole ()->Printf ( "single node: %i", iPointerNodeSingleLinkEntries );
    g_pCore->GetConsole ()->Printf ( "dbl node: %i", iPointerNodeDoubleLinkEntries );
}

#include "CVehicleNames.h"

CVehicle* aaa = NULL;
CVehicle* bbb = NULL;

CMatrix *save = NULL;
float fTest = 0;

#include <crtdbg.h>
void COMMAND_Debug2 ( const char *szCmdLine )
{
    g_pGame->GetAudio ()->StopRadio ();
}

CClientPed* pTest = NULL;
CClientVehicle *v, *vnew;

void COMMAND_Debug3 ( const char *szCmdLine )
{
    _asm {
        pushad
        mov     ecx, 0x8CB6F8
        mov     eax, 0x4E7F80
        call    eax
        popad
    }
    g_pGame->GetAudio()->StopRadio();
    g_pGame->GetAudio()->StartRadio(1);
    return;
}

CVector origin22;
CObject *o;

void COMMAND_Debug4 ( const char* szCmdLine )
{
    g_pCore->GetConsole ()->Printf ( "debug4" );
    g_pClientGame->StartPlayback ();
    return;
}
#endif


void COMMAND_ShowCollision ( const char* szCmdLine )
{
    int iCmd = ( szCmdLine && szCmdLine [ 0 ] ) ? atoi ( szCmdLine ) : -1;
    bool bShow = ( iCmd == 1 ) ? true : ( iCmd == 0 ) ? false : !g_pClientGame->GetShowCollision ();
    g_pClientGame->SetShowCollision ( bShow );
    g_pCore->GetConsole ()->Printf ( "showcol is now set to %d", bShow ? 1 : 0 );
    if ( bShow && !g_pClientGame->GetDevelopmentMode () )
        g_pCore->GetConsole ()->Print ( "showcol will have no effect because development mode is off" );
}


void COMMAND_ShowSound ( const char* szCmdLine )
{
    int iCmd = ( szCmdLine && szCmdLine [ 0 ] ) ? atoi ( szCmdLine ) : -1;
    bool bShow = ( iCmd == 1 ) ? true : ( iCmd == 0 ) ? false : !g_pClientGame->GetShowSound ();
    g_pClientGame->SetShowSound ( bShow );
    g_pCore->GetConsole ()->Printf ( "showsound is now set to %d", bShow ? 1 : 0 );
    if ( bShow && !g_pClientGame->GetDevelopmentMode () )
        g_pCore->GetConsole ()->Print ( "showsound will have no effect because development mode is off" );
}
