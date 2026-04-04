#pragma once
#include <cstddef>
#include <cstdint>

// Generated from cs2-dumper build 14141 (2026-04-04)
// https://github.com/a2x/cs2-dumper

namespace offsets {

    // Module: client.dll
    namespace client {
        constexpr std::ptrdiff_t dwCSGOInput = 0x231E330;
        constexpr std::ptrdiff_t dwEntityList = 0x24B3268;
        constexpr std::ptrdiff_t dwGameEntitySystem = 0x24B3268;
        constexpr std::ptrdiff_t dwGameRules = 0x2311ED0;
        constexpr std::ptrdiff_t dwGlobalVars = 0x2062540;
        constexpr std::ptrdiff_t dwGlowManager = 0x230ECD8;
        constexpr std::ptrdiff_t dwLocalPlayerController = 0x22F8028;
        constexpr std::ptrdiff_t dwLocalPlayerPawn = 0x206D9E0;
        constexpr std::ptrdiff_t dwPlantedC4 = 0x231BAB0;
        constexpr std::ptrdiff_t dwPrediction = 0x206D8F0;
        constexpr std::ptrdiff_t dwSensitivity = 0x230F7E8;
        constexpr std::ptrdiff_t dwSensitivity_sensitivity = 0x58;
        constexpr std::ptrdiff_t dwViewAngles = 0x231E9B8;
        constexpr std::ptrdiff_t dwViewMatrix = 0x2313F10;
        constexpr std::ptrdiff_t dwViewRender = 0x2314328;
    }

    // Module: engine2.dll
    namespace engine {
        constexpr std::ptrdiff_t dwBuildNumber = 0x60E514;
        constexpr std::ptrdiff_t dwNetworkGameClient = 0x9095D0;
        constexpr std::ptrdiff_t dwNetworkGameClient_signOnState = 0x230;
        constexpr std::ptrdiff_t dwWindowWidth = 0x90D998;
        constexpr std::ptrdiff_t dwWindowHeight = 0x90D99C;
    }

    // C_BaseEntity
    namespace entity {
        constexpr std::ptrdiff_t m_CBodyComponent = 0x38;
        constexpr std::ptrdiff_t m_pGameSceneNode = 0x338;
        constexpr std::ptrdiff_t m_iMaxHealth = 0x350;
        constexpr std::ptrdiff_t m_iHealth = 0x354;
        constexpr std::ptrdiff_t m_lifeState = 0x35C;
        constexpr std::ptrdiff_t m_iTeamNum = 0x3F3;      // uint8
        constexpr std::ptrdiff_t m_fFlags = 0x400;
        constexpr std::ptrdiff_t m_vecAbsVelocity = 0x404;
        constexpr std::ptrdiff_t m_vecVelocity = 0x438;
        constexpr std::ptrdiff_t m_hOwnerEntity = 0x528;
    }

    // CGameSceneNode
    namespace sceneNode {
        constexpr std::ptrdiff_t m_nodeToWorld = 0x10;
        constexpr std::ptrdiff_t m_vecOrigin = 0x88;
        constexpr std::ptrdiff_t m_vecAbsOrigin = 0xD0;
        constexpr std::ptrdiff_t m_angAbsRotation = 0xDC;
        constexpr std::ptrdiff_t m_bDormant = 0x10B;
    }

    // CSkeletonInstance / CModelState
    namespace skeleton {
        constexpr std::ptrdiff_t m_modelState = 0x160;
        constexpr std::ptrdiff_t m_boneArraySubOffset = 0x80;  // within CModelState
        // Combined: sceneNode + m_modelState + m_boneArraySubOffset
        constexpr std::ptrdiff_t m_pBoneArray = 0x1E0;
    }

    // C_BaseModelEntity
    namespace model {
        constexpr std::ptrdiff_t m_vecViewOffset = 0xD58;
        constexpr std::ptrdiff_t m_Glow = 0xCC0;
        constexpr std::ptrdiff_t m_Collision = 0xC10;
    }

    // C_BasePlayerPawn
    namespace playerPawn {
        constexpr std::ptrdiff_t m_pWeaponServices = 0x13D8;
        constexpr std::ptrdiff_t m_pItemServices = 0x13E0;
        constexpr std::ptrdiff_t m_pCameraServices = 0x1410;
        constexpr std::ptrdiff_t m_pMovementServices = 0x1418;
        constexpr std::ptrdiff_t v_angle = 0x1490;
        constexpr std::ptrdiff_t m_hController = 0x15A0;
    }

    // C_CSPlayerPawnBase
    namespace csPawnBase {
        constexpr std::ptrdiff_t m_flFlashBangTime = 0x15E4;
        constexpr std::ptrdiff_t m_flFlashMaxAlpha = 0x15F4;
        constexpr std::ptrdiff_t m_flFlashDuration = 0x15F8;
        constexpr std::ptrdiff_t m_hOriginalController = 0x1648;
    }

    // C_CSPlayerPawn
    namespace csPawn {
        constexpr std::ptrdiff_t m_aimPunchAngle = 0x16CC;
        constexpr std::ptrdiff_t m_aimPunchAngleVel = 0x16D8;
        constexpr std::ptrdiff_t m_aimPunchTickBase = 0x16E4;
        constexpr std::ptrdiff_t m_bIsScoped = 0x26F8;
        constexpr std::ptrdiff_t m_bIsDefusing = 0x26FA;
        constexpr std::ptrdiff_t m_bIsGrabbingHostage = 0x26FB;
        constexpr std::ptrdiff_t m_entitySpottedState = 0x26E0;
        constexpr std::ptrdiff_t m_iShotsFired = 0x270C;
        constexpr std::ptrdiff_t m_flVelocityModifier = 0x2714;
        constexpr std::ptrdiff_t m_ArmorValue = 0x272C;
        constexpr std::ptrdiff_t m_angEyeAngles = 0x3DD0;
        constexpr std::ptrdiff_t m_iIDEntIndex = 0x3EAC;
    }

    // CCSPlayerController
    namespace controller {
        constexpr std::ptrdiff_t m_iPing = 0x828;
        constexpr std::ptrdiff_t m_iCompTeammateColor = 0x848;
        constexpr std::ptrdiff_t m_szClan = 0x858;
        constexpr std::ptrdiff_t m_sSanitizedPlayerName = 0x860;
        constexpr std::ptrdiff_t m_hPlayerPawn = 0x90C;
        constexpr std::ptrdiff_t m_hObserverPawn = 0x910;
        constexpr std::ptrdiff_t m_bPawnIsAlive = 0x914;
        constexpr std::ptrdiff_t m_iPawnHealth = 0x918;
        constexpr std::ptrdiff_t m_iPawnArmor = 0x91C;
        constexpr std::ptrdiff_t m_bPawnHasDefuser = 0x920;
        constexpr std::ptrdiff_t m_bPawnHasHelmet = 0x921;
        constexpr std::ptrdiff_t m_iMVPs = 0x950;
        // Inherited from CBasePlayerController
        constexpr std::ptrdiff_t m_hPawn = 0x6C4;
        constexpr std::ptrdiff_t m_iszPlayerName = 0x6F8;    // char[128]
        constexpr std::ptrdiff_t m_steamID = 0x780;
        constexpr std::ptrdiff_t m_bIsLocalPlayerController = 0x788;
    }


    // Buttons (Module: client.dll)
    namespace buttons {
        constexpr std::ptrdiff_t attack = 0x2066760;
        constexpr std::ptrdiff_t attack2 = 0x20667F0;
        constexpr std::ptrdiff_t back = 0x2066A30;
        constexpr std::ptrdiff_t duck = 0x2066D00;
        constexpr std::ptrdiff_t forward = 0x20669A0;
        constexpr std::ptrdiff_t jump = 0x2066C70;
        constexpr std::ptrdiff_t left = 0x2066AC0;
        constexpr std::ptrdiff_t reload = 0x20666D0;
        constexpr std::ptrdiff_t right = 0x2066B50;
        constexpr std::ptrdiff_t sprint = 0x2066640;
        constexpr std::ptrdiff_t use = 0x2066BE0;
    }
}
