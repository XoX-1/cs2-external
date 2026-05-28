#include "aimbot.h"
#include "../config.h"
#include "../offsets.h"
#include "../memory.h"
#include <Windows.h>
#include <cmath>

namespace Aimbot {

    void Run(const EntityData* entities, int count, uintptr_t localPawn, int screenWidth, int screenHeight) {
        if (!config.bAimbot) return;
        if (!(GetAsyncKeyState(config.aimKey) & 0x8000)) return;

        uintptr_t sceneNode = mem.Read<uintptr_t>(localPawn + offsets::entity::m_pGameSceneNode);
        if (!sceneNode) return;

        Vector3 localOrigin = mem.Read<Vector3>(sceneNode + offsets::sceneNode::m_vecAbsOrigin);
        Vector3 viewOffset = mem.Read<Vector3>(localPawn + offsets::model::m_vecViewOffset);

        if (viewOffset.z < 1.0f || viewOffset.z > 100.0f)
            viewOffset = { 0.0f, 0.0f, 64.06f };

        Vector3 eyePos = localOrigin + viewOffset;
        Vector3 viewAngles = mem.Read<Vector3>(mem.client + offsets::client::dwViewAngles);

        uintptr_t aimpunchservices = mem.Read<uintptr_t>(localPawn + offsets::csPawn::m_pAimPunchServices);

        if (!aimpunchservices) return;

        // Read punch angle
        Vector3 punchAngle = {};
        if (config.bRcs) {
            int shotsFired = mem.Read<int>(localPawn + offsets::csPawn::m_iShotsFired);
            if (shotsFired > 1) {
                punchAngle = mem.Read<Vector3>(aimpunchservices + offsets::csPawn::m_predictableBaseAngle);
            }
        }

        // For FOV check: compare against where crosshair visually points
        // Visual crosshair = viewAngles + punchAngle * 2
        Vector3 visualAngles = viewAngles;
        visualAngles.x += punchAngle.x * 2.0f;
        visualAngles.y += punchAngle.y * 2.0f;
        NormalizeAngles(visualAngles);

        float bestFov = config.aimFov;
        Vector3 bestRawAngle = {};
        bool found = false;

        for (int i = 0; i < count; i++) {
            const EntityData& ent = entities[i];
            if (!ent.valid || !ent.bonesValid) continue;

            Vector3 targetPos = ent.bones[config.aimBone];
            if (targetPos.IsZero()) continue;

            // Raw angle to target (no RCS)
            Vector3 aimAngle = CalculateAngle(eyePos, targetPos);
            NormalizeAngles(aimAngle);

            // FOV check against visual crosshair position
            float fov = GetFov(visualAngles, aimAngle);
            if (fov < bestFov) {
                bestFov = fov;
                bestRawAngle = aimAngle;
                found = true;
            }
        }

        if (found) {
            // Re-read for freshest data
            viewAngles = mem.Read<Vector3>(mem.client + offsets::client::dwViewAngles);

            // Apply RCS: we need viewAngles such that viewAngles + punch*2 = target
            // So: viewAngles = target - punch*2
            Vector3 compensated = bestRawAngle;
            compensated.x -= punchAngle.x * 2.0f;
            compensated.y -= punchAngle.y * 2.0f;
            NormalizeAngles(compensated);

            Vector3 delta = compensated - viewAngles;
            NormalizeAngles(delta);
            float dist = std::sqrt(delta.x * delta.x + delta.y * delta.y);

            Vector3 finalAngle;
            if (config.aimSmooth <= 1.0f || dist < 0.15f) {
                finalAngle = compensated;
            } else {
                finalAngle = SmoothAngle(viewAngles, compensated, config.aimSmooth);
            }
            NormalizeAngles(finalAngle);
            mem.Write<Vector3>(mem.client + offsets::client::dwViewAngles, finalAngle);
        }
    }
}
