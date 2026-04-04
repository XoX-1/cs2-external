#include "misc.h"
#include "../config.h"
#include "../offsets.h"
#include "../memory.h"
#include "../sdk.h"
#include <Windows.h>
#include <chrono>

static void MouseDown() {
    INPUT inp{};
    inp.type = INPUT_MOUSE;
    inp.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    SendInput(1, &inp, sizeof(INPUT));
}

static void MouseUp() {
    INPUT inp{};
    inp.type = INPUT_MOUSE;
    inp.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(1, &inp, sizeof(INPUT));
}

namespace Misc {

    void Bhop(uintptr_t localPawn) {
        if (!config.bBhop) return;
        if (!(GetAsyncKeyState(VK_SPACE) & 0x8000)) return;

        uint32_t flags = mem.Read<uint32_t>(localPawn + offsets::entity::m_fFlags);
        if (flags & FL_ONGROUND) {
            mem.Write<int>(mem.client + offsets::buttons::jump, 65537);
        } else {
            mem.Write<int>(mem.client + offsets::buttons::jump, 256);
        }
    }

    void NoFlash(uintptr_t localPawn) {
        if (!config.bNoFlash) return;

        float flashAlpha = mem.Read<float>(localPawn + offsets::csPawnBase::m_flFlashMaxAlpha);
        if (flashAlpha > 0.0f) {
            mem.Write<float>(localPawn + offsets::csPawnBase::m_flFlashMaxAlpha, 0.0f);
        }
    }

    void Triggerbot(uintptr_t localPawn, uint8_t localTeam) {
        static bool mouseHeld = false;
        static std::chrono::steady_clock::time_point delayStart;
        static bool delayActive = false;

        auto release = [&]() {
            if (mouseHeld) {
                MouseUp();
                mouseHeld = false;
            }
            delayActive = false;
        };

        if (!config.bTriggerbot || !(GetAsyncKeyState(config.triggerKey) & 0x8000)) {
            release();
            return;
        }

        int crosshairEntityId = mem.Read<int>(localPawn + offsets::csPawn::m_iIDEntIndex);
        if (crosshairEntityId <= 0) {
            release();
            return;
        }

        uintptr_t entityList = mem.Read<uintptr_t>(mem.client + offsets::client::dwEntityList);
        if (!entityList) { release(); return; }

        uintptr_t listEntry = mem.Read<uintptr_t>(
            entityList + (8 * ((crosshairEntityId & 0x7FFF) >> 9) + 16)
        );
        if (!listEntry) { release(); return; }

        uintptr_t entity = mem.Read<uintptr_t>(listEntry + 112 * ((crosshairEntityId & 0x7FFF) & 0x1FF));
        if (!entity) { release(); return; }

        int health = mem.Read<int>(entity + offsets::entity::m_iHealth);
        uint8_t team = mem.Read<uint8_t>(entity + offsets::entity::m_iTeamNum);

        if (health <= 0 || team == localTeam) {
            release();
            return;
        }

        // Handle trigger delay
        if (config.triggerDelay > 0) {
            if (!delayActive) {
                delayStart = std::chrono::steady_clock::now();
                delayActive = true;
                return;
            }
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - delayStart
            ).count();
            if (elapsed < config.triggerDelay) return;
        }

        // Simulate mouse click via SendInput
        if (!mouseHeld) {
            MouseDown();
            mouseHeld = true;
        }
    }
}
