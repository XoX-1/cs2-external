#pragma once
#include <cstddef>
#include <cstdint>

// Generated from cs2-dumper build 14141 (2026-04-04)
// https://github.com/a2x/cs2-dumper

namespace offsets {

    // Module: client.dll
    namespace client {
        constexpr std::ptrdiff_t dwEntityList = 0x24D4E80;
        constexpr std::ptrdiff_t dwLocalPlayerController = 0x230E5C0;
        constexpr std::ptrdiff_t dwLocalPlayerPawn = 0x205A700;
        constexpr std::ptrdiff_t dwViewAngles = 0x23444F8;
        constexpr std::ptrdiff_t dwViewMatrix = 0x2334850;
    }

    // C_BaseEntity
    namespace entity {
        constexpr std::ptrdiff_t m_pGameSceneNode = 0x330;
        constexpr std::ptrdiff_t m_iHealth = 0x34C;
        constexpr std::ptrdiff_t m_iTeamNum = 0x3EB;      // uint8
        constexpr std::ptrdiff_t m_fFlags = 0x3F8;
    }

    // CGameSceneNode
    namespace sceneNode {
        constexpr std::ptrdiff_t m_vecAbsOrigin = 0xC8;
        constexpr std::ptrdiff_t m_bDormant = 0x103;
    }

    // CSkeletonInstance / CModelState
    namespace skeleton {
        constexpr std::ptrdiff_t m_modelState = 0x150;
        constexpr std::ptrdiff_t m_boneArraySubOffset = 0x80;  // within CModelState
        // Combined: sceneNode + m_modelState + m_boneArraySubOffset
    }

    // C_BaseModelEntity
    namespace model {
        constexpr std::ptrdiff_t m_vecViewOffset = 0xE70;
    }

    // C_CSPlayerPawnBase
    namespace csPawnBase {
        constexpr std::ptrdiff_t m_flFlashMaxAlpha = 0x13FC;
    }

    // C_CSPlayerPawn
    namespace csPawn {
        constexpr std::ptrdiff_t m_pAimPunchServices = 0x1490; // CCSPlayer_AimPunchServices*
        constexpr std::ptrdiff_t m_predictableBaseAngle = 0x50;
        constexpr std::ptrdiff_t m_bIsScoped = 0x1C50;
        constexpr std::ptrdiff_t m_iShotsFired = 0x1C64;
        constexpr std::ptrdiff_t m_ArmorValue = 0x1C7C;
        constexpr std::ptrdiff_t m_iIDEntIndex = 0x344C;
    }

    // CCSPlayerController
    namespace controller {
        constexpr std::ptrdiff_t m_hPlayerPawn = 0x90C;
        constexpr std::ptrdiff_t m_bPawnHasHelmet = 0x921;
        // Inherited from CBasePlayerController
        constexpr std::ptrdiff_t m_iszPlayerName = 0x6F4;    // char[128]
    }


    // Buttons (Module: client.dll)
    namespace buttons {
        constexpr std::ptrdiff_t jump = 0x204FEA0;
    }
}
