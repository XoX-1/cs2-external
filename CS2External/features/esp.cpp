#include "esp.h"
#include "../config.h"
#include "imgui.h"
#include <cstdio>

namespace ESP {

    static void DrawBox(const Vector2& top, const Vector2& bottom, const float* color) {
        float height = bottom.y - top.y;
        float width = height / 2.4f;

        ImDrawList* draw = ImGui::GetBackgroundDrawList();
        ImU32 col = ImGui::ColorConvertFloat4ToU32(ImVec4(color[0], color[1], color[2], color[3]));
        ImU32 outline = ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 1));

        float left = top.x - width * 0.5f;
        float right = top.x + width * 0.5f;

        draw->AddRect(ImVec2(left - 1, top.y - 1), ImVec2(right + 1, bottom.y + 1), outline, 0, 0, 1.0f);
        draw->AddRect(ImVec2(left + 1, top.y + 1), ImVec2(right - 1, bottom.y - 1), outline, 0, 0, 1.0f);
        draw->AddRect(ImVec2(left, top.y), ImVec2(right, bottom.y), col, 0, 0, 1.0f);
    }

    static void DrawHealthBar(const Vector2& top, const Vector2& bottom, int health) {
        float height = bottom.y - top.y;
        float width = height / 2.4f;
        float barHeight = height * (health / 100.0f);

        float r = (100 - health) / 100.0f;
        float g = health / 100.0f;

        ImDrawList* draw = ImGui::GetBackgroundDrawList();
        ImU32 bgColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 0.6f));
        ImU32 hpColor = ImGui::ColorConvertFloat4ToU32(ImVec4(r, g, 0, 1));

        float barLeft = top.x - width * 0.5f - 6.0f;
        float barRight = barLeft + 3.0f;

        draw->AddRectFilled(ImVec2(barLeft - 1, top.y - 1), ImVec2(barRight + 1, bottom.y + 1), bgColor);
        draw->AddRectFilled(ImVec2(barLeft, bottom.y - barHeight), ImVec2(barRight, bottom.y), hpColor);

        if (health < 100) {
            char buf[8];
            snprintf(buf, sizeof(buf), "%d", health);
            ImVec2 textSize = ImGui::CalcTextSize(buf);
            float textY = bottom.y - barHeight - textSize.y * 0.5f;
            textY = (std::max)(textY, top.y - textSize.y);
            draw->AddText(ImVec2(barLeft - textSize.x - 2, textY),
                ImGui::ColorConvertFloat4ToU32(ImVec4(1, 1, 1, 1)), buf);
        }
    }

    static void DrawName(const Vector2& top, const char* name) {
        ImDrawList* draw = ImGui::GetBackgroundDrawList();
        ImVec2 textSize = ImGui::CalcTextSize(name);
        float x = top.x - textSize.x * 0.5f;
        float y = top.y - textSize.y - 2.0f;

        draw->AddText(ImVec2(x + 1, y + 1), ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 1)), name);
        draw->AddText(ImVec2(x, y), ImGui::ColorConvertFloat4ToU32(ImVec4(1, 1, 1, 1)), name);
    }

    static void DrawSkeleton(const Vector3* bones, const Matrix4x4& vm, int w, int h, const float* color) {
        ImDrawList* draw = ImGui::GetBackgroundDrawList();
        ImU32 col = ImGui::ColorConvertFloat4ToU32(ImVec4(color[0], color[1], color[2], color[3]));

        static const int connections[][2] = {
            // Spine
            { BoneIndex::PELVIS,         BoneIndex::SPINE_1 },
            { BoneIndex::SPINE_1,        BoneIndex::SPINE_2 },
            { BoneIndex::SPINE_2,        BoneIndex::CHEST },
            { BoneIndex::CHEST,          BoneIndex::NECK },
            { BoneIndex::NECK,           BoneIndex::HEAD },
            { BoneIndex::NECK,           BoneIndex::LEFT_SHOULDER },
            { BoneIndex::LEFT_SHOULDER,  BoneIndex::LEFT_ELBOW },
            { BoneIndex::LEFT_ELBOW,     BoneIndex::LEFT_HAND },
            { BoneIndex::NECK,           BoneIndex::RIGHT_SHOULDER },
            { BoneIndex::RIGHT_SHOULDER, BoneIndex::RIGHT_ELBOW },
            { BoneIndex::RIGHT_ELBOW,    BoneIndex::RIGHT_HAND },
            { BoneIndex::PELVIS,         BoneIndex::LEFT_HIP },
            { BoneIndex::LEFT_HIP,       BoneIndex::LEFT_KNEE },
            { BoneIndex::LEFT_KNEE,      BoneIndex::LEFT_FOOT },
            { BoneIndex::PELVIS,         BoneIndex::RIGHT_HIP },
            { BoneIndex::RIGHT_HIP,      BoneIndex::RIGHT_KNEE },
            { BoneIndex::RIGHT_KNEE,     BoneIndex::RIGHT_FOOT },
        };

        for (const auto& c : connections) {
            Vector2 from, to;
            if (WorldToScreen(bones[c[0]], from, vm, w, h) &&
                WorldToScreen(bones[c[1]], to, vm, w, h)) {
                draw->AddLine(ImVec2(from.x, from.y), ImVec2(to.x, to.y), col, 1.5f);
            }
        }
    }

    static void DrawSnapline(const Vector2& bottom, int screenW, int screenH, const float* color) {
        ImDrawList* draw = ImGui::GetBackgroundDrawList();
        ImU32 col = ImGui::ColorConvertFloat4ToU32(ImVec4(color[0], color[1], color[2], color[3]));
        draw->AddLine(ImVec2(screenW * 0.5f, static_cast<float>(screenH)), ImVec2(bottom.x, bottom.y), col, 1.0f);
    }

    static void DrawArmorBar(const Vector2& top, const Vector2& bottom, int armor) {
        float height = bottom.y - top.y;
        float width = height / 2.4f;
        float barWidth = width * (armor / 100.0f);

        ImDrawList* draw = ImGui::GetBackgroundDrawList();
        ImU32 bgColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 0.6f));
        ImU32 armorColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0.5f, 1.0f, 1));

        float barTop = bottom.y + 2.0f;
        float barBottom = barTop + 3.0f;
        float left = top.x - width * 0.5f;
        float right = top.x + width * 0.5f;

        draw->AddRectFilled(ImVec2(left - 1, barTop - 1), ImVec2(right + 1, barBottom + 1), bgColor);
        draw->AddRectFilled(ImVec2(left, barTop), ImVec2(left + barWidth, barBottom), armorColor);
    }

    static void DrawDistance(const Vector2& bottom, float distance) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.0fm", distance / 100.0f);
        ImDrawList* draw = ImGui::GetBackgroundDrawList();
        ImVec2 textSize = ImGui::CalcTextSize(buf);
        float x = bottom.x - textSize.x * 0.5f;
        float y = bottom.y + 6.0f;

        draw->AddText(ImVec2(x + 1, y + 1), ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 1)), buf);
        draw->AddText(ImVec2(x, y), ImGui::ColorConvertFloat4ToU32(ImVec4(0.8f, 0.8f, 0.8f, 1)), buf);
    }

    void Render(const EntityData* entities, int count, const Matrix4x4& viewMatrix, int screenWidth, int screenHeight) {
        if (!config.bEsp) return;

        for (int i = 0; i < count; i++) {
            const EntityData& ent = entities[i];
            if (!ent.valid) continue;

            Vector3 headTop = ent.headPos + Vector3{ 0, 0, 8.0f };
            Vector2 screenHead, screenFeet;

            if (!WorldToScreen(headTop, screenHead, viewMatrix, screenWidth, screenHeight)) continue;
            if (!WorldToScreen(ent.origin, screenFeet, viewMatrix, screenWidth, screenHeight)) continue;

            float boxHeight = screenFeet.y - screenHead.y;
            if (boxHeight < 4.0f) continue;

            if (config.bEspBox)
                DrawBox(screenHead, screenFeet, config.espBoxColor);

            if (config.bEspHealth)
                DrawHealthBar(screenHead, screenFeet, ent.health);

            if (config.bEspName && ent.name[0])
                DrawName(screenHead, ent.name);

            if (config.bEspSkeleton && ent.bonesValid)
                DrawSkeleton(ent.bones, viewMatrix, screenWidth, screenHeight, config.espSkeletonColor);

            if (config.bEspSnaplines)
                DrawSnapline(screenFeet, screenWidth, screenHeight, config.espSnaplineColor);

            if (config.bEspArmor && ent.armor > 0)
                DrawArmorBar(screenHead, screenFeet, ent.armor);

            if (config.bEspDistance)
                DrawDistance(screenFeet, ent.distance);
        }
    }
}
