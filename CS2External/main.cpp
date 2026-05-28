#include <Windows.h>
#include <dwmapi.h>
#include <d3d11.h>
#include <cstdio>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include "memory.h"
#include "sdk.h"
#include "offsets.h"
#include "config.h"
#include "features/esp.h"
#include "features/aimbot.h"
#include "features/misc.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwmapi.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ── Globals ─────────────────────────────────────────────────────────────────
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static ID3D11RenderTargetView*  g_pRTV = nullptr;

static HWND     g_overlay = nullptr;
static HWND     g_gameWnd = nullptr;
static bool     g_menuVisible = true;
static bool     g_running = true;
static int      g_width = 1920;
static int      g_height = 1080;

static ImFont*  g_fontMain  = nullptr;
static ImFont*  g_fontBold  = nullptr;
static ImFont*  g_fontBrand = nullptr;

static EntityData   g_entities[64];
static int          g_entityCount = 0;

// ── Debug State ─────────────────────────────────────────────────────────────
static struct DebugInfo {
    bool enabled = true;
    uintptr_t clientBase = 0;
    uintptr_t engineBase = 0;
    DWORD pid = 0;
    HWND gameWnd = nullptr;
    uintptr_t localController = 0;
    uintptr_t localPawn = 0;
    uintptr_t entityList = 0;
    uint8_t localTeam = 0;
    int localHealth = 0;
    Vector3 localOrigin = {};
    Vector3 viewAngles = {};
    float viewMatrix00 = 0;
    int entityCount = 0;
    int controllersFound = 0;
    int pawnsResolved = 0;
    int aliveEnemies = 0;
    int dormantSkipped = 0;
    int bonesValid = 0;
    uintptr_t lastPawn = 0;
    int lastHealth = 0;
    uint8_t lastTeam = 0;
    Vector3 lastOrigin = {};
    uintptr_t lastSceneNode = 0;
    uintptr_t lastBoneArray = 0;
    char lastName[128] = {};
    struct EntityTrace {
        int index = 0;
        uintptr_t controller = 0;
        uint32_t pawnHandle = 0;
        uintptr_t pawn = 0;
        int health = 0;
        uint8_t team = 0;
        bool isDormant = false;
        const char* failReason = "";
    };
    EntityTrace traces[24];
    int traceCount = 0;
    int localCtrlFoundIdx = -1;
    uint32_t localCtrlPawnHandle = 0;
    int noPawnHandle = 0;
    int pawnNull = 0;
    int pawnEntryNull = 0;
} g_debug;

// ── DX11 Helpers ────────────────────────────────────────────────────────────
static void CreateRenderTarget() {
    ID3D11Texture2D* pBack = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBack));
    if (pBack) {
        g_pd3dDevice->CreateRenderTargetView(pBack, nullptr, &g_pRTV);
        pBack->Release();
    }
}

static void CleanupRenderTarget() {
    if (g_pRTV) { g_pRTV->Release(); g_pRTV = nullptr; }
}

static bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate = { 0, 1 };
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc = { 1, 0 };
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL fl;
    const D3D_FEATURE_LEVEL flArr[] = { D3D_FEATURE_LEVEL_11_0 };

    if (FAILED(D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            flArr, 1, D3D11_SDK_VERSION,
            &sd, &g_pSwapChain, &g_pd3dDevice, &fl, &g_pd3dContext)))
        return false;

    CreateRenderTarget();
    return true;
}

static void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dContext) { g_pd3dContext->Release(); g_pd3dContext = nullptr; }
    if (g_pd3dDevice)  { g_pd3dDevice->Release();  g_pd3dDevice = nullptr; }
}

// ── WndProc ─────────────────────────────────────────────────────────────────
static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        if (g_pd3dDevice && wParam != SIZE_MINIMIZED) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, LOWORD(lParam), HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// ── Entity Cache ────────────────────────────────────────────────────────────
static void UpdateEntityData(uintptr_t localPawn, uint8_t localTeam) {
    g_entityCount = 0;
    g_debug.controllersFound = 0;
    g_debug.pawnsResolved = 0;
    g_debug.aliveEnemies = 0;
    g_debug.dormantSkipped = 0;
    g_debug.bonesValid = 0;
    g_debug.localCtrlFoundIdx = -1;
    g_debug.localCtrlPawnHandle = 0;
    g_debug.noPawnHandle = 0;
    g_debug.pawnNull = 0;
    g_debug.pawnEntryNull = 0;

    uintptr_t entityList = mem.Read<uintptr_t>(mem.client + offsets::client::dwEntityList);
    g_debug.entityList = entityList;
    if (!entityList) return;

    uintptr_t localSN = mem.Read<uintptr_t>(localPawn + offsets::entity::m_pGameSceneNode);
    Vector3 localOrigin{};
    if (localSN)
        localOrigin = mem.Read<Vector3>(localSN + offsets::sceneNode::m_vecAbsOrigin);
    g_debug.localOrigin = localOrigin;

    g_debug.traceCount = 0;

    for (int i = 1; i <= 64 && g_entityCount < 64; i++) {
        uintptr_t listEntry = mem.Read<uintptr_t>(entityList + (8 * (i >> 9) + 16));
        if (!listEntry) continue;

        uintptr_t controller = mem.Read<uintptr_t>(listEntry + 112 * (i & 0x1FF));
        if (!controller || controller < 0x10000 || controller > 0x7FFFFFFFFFFF) continue;
        g_debug.controllersFound++;

        uint32_t pawnHandle = mem.Read<uint32_t>(controller + offsets::controller::m_hPlayerPawn);

        auto addTrace = [&](const char* reason, uintptr_t pawn = 0, int hp = 0, uint8_t tm = 0, bool dorm = false) {
            if (g_debug.traceCount < 24) {
                auto& t = g_debug.traces[g_debug.traceCount++];
                t.index = i;
                t.controller = controller;
                t.pawnHandle = pawnHandle;
                t.pawn = pawn;
                t.health = hp;
                t.team = tm;
                t.isDormant = dorm;
                t.failReason = reason;
            }
        };

        if (controller == g_debug.localController) {
            g_debug.localCtrlFoundIdx = i;
            g_debug.localCtrlPawnHandle = pawnHandle;
        }

        if (!pawnHandle || pawnHandle == 0xFFFFFFFF) { g_debug.noPawnHandle++; addTrace("no pawn handle"); continue; }

        uintptr_t pawnEntry = mem.Read<uintptr_t>(entityList + (8 * ((pawnHandle & 0x7FFF) >> 9) + 16));
        if (!pawnEntry) { g_debug.pawnEntryNull++; addTrace("pawnEntry null"); continue; }

        uintptr_t pawn = mem.Read<uintptr_t>(pawnEntry + 112 * ((pawnHandle & 0x7FFF) & 0x1FF));
        if (!pawn) { g_debug.pawnNull++; addTrace("pawn null"); continue; }
        if (pawn == localPawn) { addTrace("is local", pawn); continue; }
        g_debug.pawnsResolved++;

        int health = mem.Read<int>(pawn + offsets::entity::m_iHealth);
        uint8_t team = mem.Read<uint8_t>(pawn + offsets::entity::m_iTeamNum);

        if (health <= 0) { addTrace("dead (hp<=0)", pawn, health, team); continue; }
        if (health > 100) { addTrace("bad hp (>100)", pawn, health, team); continue; }
        g_debug.aliveEnemies++;

        if (config.bEspTeamCheck && team == localTeam) { addTrace("teammate", pawn, health, team); continue; }

        uintptr_t sceneNode = mem.Read<uintptr_t>(pawn + offsets::entity::m_pGameSceneNode);
        if (!sceneNode) { addTrace("no sceneNode", pawn, health, team); continue; }

        bool dormant = mem.Read<bool>(sceneNode + offsets::sceneNode::m_bDormant);
        if (dormant) { g_debug.dormantSkipped++; addTrace("dormant", pawn, health, team, true); continue; }

        addTrace("OK", pawn, health, team);

        EntityData& ent = g_entities[g_entityCount];
        ent.controller  = controller;
        ent.pawn        = pawn;
        ent.health      = health;
        ent.team        = team;
        ent.dormant     = false;
        ent.alive       = true;
        ent.valid       = true;
        ent.origin      = mem.Read<Vector3>(sceneNode + offsets::sceneNode::m_vecAbsOrigin);
        ent.armor       = mem.Read<int>(pawn + offsets::csPawn::m_ArmorValue);
        ent.hasHelmet   = mem.Read<bool>(controller + offsets::controller::m_bPawnHasHelmet);
        ent.isScoped    = mem.Read<bool>(pawn + offsets::csPawn::m_bIsScoped);
        ent.distance    = localOrigin.Distance(ent.origin);

        mem.ReadRaw(controller + offsets::controller::m_iszPlayerName, ent.name, sizeof(ent.name));
        ent.name[127] = '\0';

        struct RawBone { float data[8]; };
        RawBone rawBones[BoneIndex::BONE_COUNT]{};

        uintptr_t boneArray = mem.Read<uintptr_t>(sceneNode + offsets::skeleton::m_modelState + offsets::skeleton::m_boneArraySubOffset);
        if (boneArray && mem.ReadRaw(boneArray, rawBones, sizeof(rawBones))) {
            ent.bonesValid = true;
            g_debug.bonesValid++;
            for (int b = 0; b < BoneIndex::BONE_COUNT; b++)
                ent.bones[b] = { rawBones[b].data[0], rawBones[b].data[1], rawBones[b].data[2] };
            ent.headPos = ent.bones[BoneIndex::HEAD];
        } else {
            ent.bonesValid = false;
            ent.headPos = ent.origin + Vector3{ 0, 0, 72.0f };
        }

        g_debug.lastPawn = pawn;
        g_debug.lastHealth = health;
        g_debug.lastTeam = team;
        g_debug.lastOrigin = ent.origin;
        g_debug.lastSceneNode = sceneNode;
        g_debug.lastBoneArray = boneArray;
        memcpy(g_debug.lastName, ent.name, sizeof(g_debug.lastName));

        g_entityCount++;
    }
}

// ── UI Helpers ──────────────────────────────────────────────────────────────
static int g_activeTab = 0;
static float g_tabAnim[3] = {};

static void DrawGlowLine(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 col, float thickness = 2.0f) {
    dl->AddLine(a, b, (col & 0x00FFFFFF) | 0x30000000, thickness + 4.0f);
    dl->AddLine(a, b, (col & 0x00FFFFFF) | 0x60000000, thickness + 2.0f);
    dl->AddLine(a, b, col, thickness);
}

static bool NavButton(const char* label, int idx, float w, float h) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    bool active = (g_activeTab == idx);

    float& anim = g_tabAnim[idx];
    float target = active ? 1.0f : 0.0f;
    anim += (target - anim) * 0.12f;

    ImGui::InvisibleButton(label, ImVec2(w, h));
    bool hovered = ImGui::IsItemHovered();
    bool clicked = ImGui::IsItemClicked();

    // Background highlight
    float alpha = active ? 0.25f : (hovered ? 0.12f : 0.0f);
    if (alpha > 0.0f)
        dl->AddRectFilled(pos, ImVec2(pos.x + w, pos.y + h),
            IM_COL32(140, 90, 235, (int)(alpha * 255)), 6.0f);

    // Left accent bar
    if (anim > 0.01f) {
        float barH = h * 0.6f * anim;
        float barY = pos.y + (h - barH) * 0.5f;
        dl->AddRectFilled(
            ImVec2(pos.x, barY),
            ImVec2(pos.x + 3.0f, barY + barH),
            IM_COL32(140, 90, 235, (int)(anim * 255)), 2.0f);
    }

    // Text
    ImU32 textCol = active ? IM_COL32(220, 200, 255, 255)
                 : hovered ? IM_COL32(180, 170, 200, 255)
                           : IM_COL32(130, 125, 150, 255);
    ImFont* nf = g_fontBold ? g_fontBold : ImGui::GetFont();
    ImVec2 ts = nf->CalcTextSizeA(nf->LegacySize, FLT_MAX, 0, label);
    dl->AddText(nf, nf->LegacySize, ImVec2(pos.x + 16.0f, pos.y + (h - ts.y) * 0.5f), textCol, label);

    if (clicked) g_activeTab = idx;
    return clicked;
}

static void SectionHeader(const char* text) {
    ImGui::Spacing();
    if (g_fontBold) ImGui::PushFont(g_fontBold);
    ImGui::TextColored(ImVec4(0.85f, 0.80f, 1.0f, 1.0f), "%s", text);
    if (g_fontBold) ImGui::PopFont();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float w = ImGui::GetContentRegionAvail().x;
    dl->AddLine(ImVec2(pos.x, pos.y), ImVec2(pos.x + w * 0.4f, pos.y),
        IM_COL32(140, 90, 235, 120), 1.5f);
    dl->AddLine(ImVec2(pos.x + w * 0.4f, pos.y), ImVec2(pos.x + w, pos.y),
        IM_COL32(140, 90, 235, 20), 1.0f);
    ImGui::Dummy(ImVec2(0, 6));
}

static bool ToggleSwitch(const char* label, bool* v) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float h = ImGui::GetFrameHeight();
    float w = h * 1.75f;
    float r = h * 0.38f;

    // Calculate button width: toggle track + gap + text, clamped to available space
    float textW = ImGui::CalcTextSize(label).x;
    float btnW = w + 10.0f + textW + 4.0f;
    float avail = ImGui::GetContentRegionAvail().x;
    if (btnW > avail) btnW = avail;

    ImGui::InvisibleButton(label, ImVec2(btnW, h));
    if (ImGui::IsItemClicked()) *v = !*v;
    bool hov = ImGui::IsItemHovered();

    // Track
    ImU32 trackCol = *v ? IM_COL32(140, 90, 235, 200) : IM_COL32(55, 55, 70, 200);
    if (hov) trackCol = *v ? IM_COL32(160, 110, 255, 220) : IM_COL32(75, 75, 95, 220);
    dl->AddRectFilled(ImVec2(pos.x, pos.y + 2), ImVec2(pos.x + w, pos.y + h - 2), trackCol, r);

    // Knob
    float knobX = *v ? (pos.x + w - r - 3.0f) : (pos.x + r + 3.0f);
    float knobY = pos.y + h * 0.5f;
    ImU32 knobCol = *v ? IM_COL32(255, 255, 255, 255) : IM_COL32(150, 150, 160, 255);
    if (*v) dl->AddCircleFilled(ImVec2(knobX, knobY), r + 2, IM_COL32(140, 90, 235, 50), 16);
    dl->AddCircleFilled(ImVec2(knobX, knobY), r, knobCol, 16);

    // Label
    dl->AddText(ImVec2(pos.x + w + 10.0f, pos.y + (h - ImGui::GetTextLineHeight()) * 0.5f),
        *v ? IM_COL32(220, 220, 230, 255) : IM_COL32(160, 160, 175, 255), label);

    return *v;
}

static void SliderLabel(const char* text) {
    ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.62f, 1.0f), "%s", text);
}

static const char* GetKeyName(int vk) {
    switch (vk) {
    case 0x01: return "Mouse1";
    case 0x02: return "Mouse2";
    case 0x04: return "Mouse3";
    case 0x05: return "Mouse4";
    case 0x06: return "Mouse5";
    case 0x10: return "Shift";
    case 0x11: return "Ctrl";
    case 0x12: return "Alt";
    case 0x14: return "CapsLock";
    case 0x20: return "Space";
    case 0x09: return "Tab";
    case VK_F1: return "F1"; case VK_F2: return "F2"; case VK_F3: return "F3";
    case VK_F4: return "F4"; case VK_F5: return "F5"; case VK_F6: return "F6";
    case VK_F7: return "F7"; case VK_F8: return "F8"; case VK_F9: return "F9";
    case VK_F10: return "F10"; case VK_F11: return "F11"; case VK_F12: return "F12";
    default:
        if (vk >= 0x30 && vk <= 0x39) { static char buf[2]; buf[0] = (char)vk; buf[1] = 0; return buf; }
        if (vk >= 0x41 && vk <= 0x5A) { static char buf[2]; buf[0] = (char)vk; buf[1] = 0; return buf; }
        static char hex[8]; snprintf(hex, sizeof(hex), "0x%02X", vk); return hex;
    }
}

static int* g_listeningKey = nullptr;  // which key binding is being listened for

static void KeyBindButton(const char* label, int* key) {
    bool listening = (g_listeningKey == key);
    char btnLabel[64];
    if (listening)
        snprintf(btnLabel, sizeof(btnLabel), "[...]##%s", label);
    else
        snprintf(btnLabel, sizeof(btnLabel), "[%s]##%s", GetKeyName(*key), label);

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button,
        listening ? ImVec4(0.55f, 0.33f, 0.92f, 0.6f) : ImVec4(0.16f, 0.16f, 0.22f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.33f, 0.92f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.45f, 0.25f, 0.80f, 1.0f));

    if (ImGui::Button(btnLabel)) {
        g_listeningKey = listening ? nullptr : key;
    }
    ImGui::PopStyleColor(3);

    // Listen for key press
    if (g_listeningKey == key) {
        // Check mouse buttons first (1-6)
        for (int m = 1; m <= 6; m++) {
            if (m == 1) continue; // skip left click so we don't capture the button click itself
            if (GetAsyncKeyState(m) & 1) {
                *key = m;
                g_listeningKey = nullptr;
                return;
            }
        }
        // Check keyboard keys
        for (int k = 0x08; k <= 0xFE; k++) {
            if (k == VK_INSERT || k == VK_END) continue; // reserved for menu toggle/exit
            if (k == VK_ESCAPE) { // cancel
                if (GetAsyncKeyState(k) & 1) {
                    g_listeningKey = nullptr;
                    return;
                }
                continue;
            }
            if (GetAsyncKeyState(k) & 1) {
                *key = k;
                g_listeningKey = nullptr;
                return;
            }
        }
    }
}

static void GroupBoxBegin(const char* label) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.07f, 0.07f, 0.10f, 0.90f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
    ImGui::BeginChild(label, ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);

    if (g_fontBold) ImGui::PushFont(g_fontBold);
    ImGui::TextColored(ImVec4(0.55f, 0.35f, 0.92f, 1.0f), "%s", label);
    if (g_fontBold) ImGui::PopFont();
    ImGui::Spacing();
}

static void GroupBoxEnd() {
    ImGui::Spacing();
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    ImGui::Spacing();
}

// ── Render Menu ─────────────────────────────────────────────────────────────
static void RenderMenu() {
    const float sideW = 140.0f;

    ImGui::SetNextWindowSize(ImVec2(600, 480), ImGuiCond_FirstUseEver);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06f, 0.06f, 0.08f, 0.98f));

    ImGui::Begin("##mindext", &g_menuVisible,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar
        | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoTitleBar);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 winPos = ImGui::GetWindowPos();
    ImVec2 winSize = ImGui::GetWindowSize();

    // ── Window border glow ──
    dl->AddRect(winPos, ImVec2(winPos.x + winSize.x, winPos.y + winSize.y),
        IM_COL32(140, 90, 235, 45), 8.0f, 0, 1.5f);
    dl->AddRect(
        ImVec2(winPos.x + 1, winPos.y + 1),
        ImVec2(winPos.x + winSize.x - 1, winPos.y + winSize.y - 1),
        IM_COL32(140, 90, 235, 18), 7.0f, 0, 1.0f);

    // ── Left sidebar ──
    {
        ImVec2 sideA = winPos;
        ImVec2 sideB = ImVec2(winPos.x + sideW, winPos.y + winSize.y);
        dl->AddRectFilled(sideA, sideB, IM_COL32(10, 10, 14, 255), 8.0f, ImDrawFlags_RoundCornersLeft);
        dl->AddLine(ImVec2(sideB.x, sideA.y + 8), ImVec2(sideB.x, sideB.y - 8),
            IM_COL32(140, 90, 235, 30));

        // Brand
        if (g_fontBrand && g_fontBold) {
            const char* b1 = "SLOP";
            const char* b2 = "EXTERNAL";
            ImVec2 ts1 = g_fontBrand->CalcTextSizeA(g_fontBrand->LegacySize, FLT_MAX, 0, b1);
            ImVec2 ts2 = g_fontBold->CalcTextSizeA(g_fontBold->LegacySize, FLT_MAX, 0, b2);
            dl->AddText(g_fontBrand, g_fontBrand->LegacySize,
                ImVec2(winPos.x + (sideW - ts1.x) * 0.5f, winPos.y + 16),
                IM_COL32(140, 90, 235, 255), b1);
            dl->AddText(g_fontBold, g_fontBold->LegacySize,
                ImVec2(winPos.x + (sideW - ts2.x) * 0.5f, winPos.y + 16 + ts1.y + 2),
                IM_COL32(90, 90, 110, 200), b2);
        }

        DrawGlowLine(dl,
            ImVec2(winPos.x + 16, winPos.y + 56),
            ImVec2(winPos.x + sideW - 16, winPos.y + 56),
            IM_COL32(140, 90, 235, 100), 1.0f);

        // Navigation
        ImGui::SetCursorPos(ImVec2(8, 66));
        ImGui::BeginChild("##nav", ImVec2(sideW - 16, winSize.y - 66 - 40), false,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);

        ImGui::Spacing();
        NavButton("Visuals", 0, sideW - 16, 34);
        ImGui::Spacing();
        NavButton("Aimbot",  1, sideW - 16, 34);
        ImGui::Spacing();
        NavButton("Misc",    2, sideW - 16, 34);

        ImGui::EndChild();

        // Version
        {
            const char* ver = "v1.0";
            ImVec2 vs = ImGui::CalcTextSize(ver);
            dl->AddText(ImVec2(winPos.x + (sideW - vs.x) * 0.5f, winPos.y + winSize.y - 26),
                IM_COL32(60, 60, 75, 150), ver);
        }
    }

    // ── Right content ──
    ImGui::SetCursorPos(ImVec2(sideW + 14, 14));
    ImGui::BeginChild("##content", ImVec2(winSize.x - sideW - 28, winSize.y - 28), false,
        ImGuiWindowFlags_NoBackground);

    // ═══════════════════════════════════════════════════
    // TAB 0: VISUALS
    // ═══════════════════════════════════════════════════
    if (g_activeTab == 0) {
        SectionHeader("ESP");

        ToggleSwitch("Enable ESP", &config.bEsp);
        ToggleSwitch("Team Check", &config.bEspTeamCheck);
        ImGui::Spacing();

        GroupBoxBegin("Elements");
        {
            float half = ImGui::GetContentRegionAvail().x * 0.5f;
            ImGui::BeginGroup();
            ImGui::PushItemWidth(half - 10);
            ToggleSwitch("Box", &config.bEspBox);
            ToggleSwitch("Skeleton", &config.bEspSkeleton);
            ToggleSwitch("Health Bar", &config.bEspHealth);
            ToggleSwitch("Name", &config.bEspName);
            ImGui::PopItemWidth();
            ImGui::EndGroup();

            ImGui::SameLine(half);

            ImGui::BeginGroup();
            ImGui::PushItemWidth(half - 10);
            ToggleSwitch("Armor Bar", &config.bEspArmor);
            ToggleSwitch("Snaplines", &config.bEspSnaplines);
            ToggleSwitch("Distance", &config.bEspDistance);
            ImGui::PopItemWidth();
            ImGui::EndGroup();
        }
        GroupBoxEnd();

        GroupBoxBegin("Colors");
        ImGui::ColorEdit4("Box##c", config.espBoxColor,
            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
        ImGui::SameLine(0, 20);
        ImGui::ColorEdit4("Skeleton##c", config.espSkeletonColor,
            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
        ImGui::SameLine(0, 20);
        ImGui::ColorEdit4("Snapline##c", config.espSnaplineColor,
            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
        GroupBoxEnd();
    }

    // ═══════════════════════════════════════════════════
    // TAB 1: AIMBOT
    // ═══════════════════════════════════════════════════
    else if (g_activeTab == 1) {
        SectionHeader("Aimbot");

        ToggleSwitch("Enable Aimbot", &config.bAimbot);
        ImGui::Spacing();

        GroupBoxBegin("Targeting");
        SliderLabel("Field of View");
        ImGui::SetNextItemWidth(-1);
        ImGui::SliderFloat("##fov", &config.aimFov, 1.0f, 30.0f, "%.1f deg");

        ImGui::Spacing();
        SliderLabel("Smoothing");
        ImGui::SetNextItemWidth(-1);
        ImGui::SliderFloat("##smooth", &config.aimSmooth, 1.0f, 20.0f, "%.1f");

        ImGui::Spacing();
        SliderLabel("Target Bone");
        const char* bones[] = { "Head", "Neck", "Chest" };
        int sel = 0;
        if (config.aimBone == BoneIndex::NECK)   sel = 1;
        if (config.aimBone == BoneIndex::SPINE_2) sel = 2;
        ImGui::SetNextItemWidth(-1);
        if (ImGui::Combo("##bone", &sel, bones, IM_ARRAYSIZE(bones))) {
            const int map[] = { BoneIndex::HEAD, BoneIndex::NECK, BoneIndex::SPINE_2 };
            config.aimBone = map[sel];
        }
        GroupBoxEnd();

        GroupBoxBegin("Options");
        ToggleSwitch("Recoil Control (RCS)", &config.bRcs);
        ToggleSwitch("FOV Circle", &config.bFovCircle);
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.62f, 1.0f), "Aim Key");
        KeyBindButton("aimkey", &config.aimKey);
        GroupBoxEnd();
    }

    // ═══════════════════════════════════════════════════
    // TAB 2: MISC
    // ═══════════════════════════════════════════════════
    else if (g_activeTab == 2) {
        SectionHeader("Miscellaneous");

        GroupBoxBegin("Triggerbot");
        ToggleSwitch("Enable Triggerbot", &config.bTriggerbot);
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.62f, 1.0f), "Trigger Key");
        KeyBindButton("trigkey", &config.triggerKey);
        ImGui::Spacing();
        SliderLabel("Delay (ms)");
        ImGui::SetNextItemWidth(-1);
        ImGui::SliderInt("##trigdelay", &config.triggerDelay, 0, 100, "%d ms");
        GroupBoxEnd();

        GroupBoxBegin("Movement");
        ToggleSwitch("Bunny Hop", &config.bBhop);
        GroupBoxEnd();

        GroupBoxBegin("Visual");
        ToggleSwitch("No Flash", &config.bNoFlash);
        GroupBoxEnd();
    }

    ImGui::EndChild(); // ##content
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

// ── Entry Point ─────────────────────────────────────────────────────────────
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int) {
    SetProcessDPIAware();
    AllocConsole();
    FILE* fp = nullptr;
    freopen_s(&fp, "CONOUT$", "w", stdout);

    printf("[*] Waiting for Counter-Strike 2...\n");
    while (!mem.Init(L"cs2.exe")) {
        Sleep(1000);
    }
    printf("[+] CS2 found  PID: %u\n", mem.pid);
    printf("[+] client.dll  0x%llX\n", static_cast<unsigned long long>(mem.client));
    printf("[+] engine2.dll 0x%llX\n", static_cast<unsigned long long>(mem.engine));

    g_debug.pid = mem.pid;
    g_debug.clientBase = mem.client;
    g_debug.engineBase = mem.engine;

    if (!mem.client) { printf("[-] FATAL: client.dll base is NULL!\n"); }
    if (!mem.engine)  { printf("[-] FATAL: engine2.dll base is NULL!\n"); }

    g_gameWnd = FindWindowA(nullptr, "Counter-Strike 2");
    if (!g_gameWnd) {
        printf("[-] CS2 window not found\n");
        return 1;
    }

    RECT gr;
    GetWindowRect(g_gameWnd, &gr);
    g_width  = gr.right - gr.left;
    g_height = gr.bottom - gr.top;

    // ── Overlay window ──
    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance      = hInst;
    wc.lpszClassName  = L"CS2Overlay";
    RegisterClassExW(&wc);

    g_overlay = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED,
        wc.lpszClassName, L"", WS_POPUP,
        gr.left, gr.top, g_width, g_height,
        nullptr, nullptr, hInst, nullptr
    );

    SetLayeredWindowAttributes(g_overlay, RGB(0, 0, 0), 255, LWA_ALPHA);
    MARGINS margins = { -1, -1, -1, -1 };
    DwmExtendFrameIntoClientArea(g_overlay, &margins);
    SetWindowDisplayAffinity(g_overlay, WDA_EXCLUDEFROMCAPTURE);
    ShowWindow(g_overlay, SW_SHOWDEFAULT);
    UpdateWindow(g_overlay);

    // ── DX11 + ImGui ──
    if (!CreateDeviceD3D(g_overlay)) {
        printf("[-] DX11 init failed\n");
        CleanupDeviceD3D();
        UnregisterClassW(wc.lpszClassName, hInst);
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;

    // Init backends FIRST (sets RendererHasTextures flag)
    ImGui_ImplWin32_Init(g_overlay);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dContext);

    // ── Fonts (AFTER backend init — no manual Build() needed) ──
    auto TryLoadFont = [&](const char* paths[], int count, float size, const ImFontConfig* cfg) -> ImFont* {
        for (int i = 0; i < count; i++) {
            DWORD attr = GetFileAttributesA(paths[i]);
            if (attr != INVALID_FILE_ATTRIBUTES)
                return io.Fonts->AddFontFromFileTTF(paths[i], size, cfg);
        }
        return nullptr;
    };

    ImFontConfig fontCfg{};
    fontCfg.OversampleH = 3;
    fontCfg.OversampleV = 2;
    fontCfg.PixelSnapH = false;

    const char* regularPaths[] = {
        "C:\\Windows\\Fonts\\segoeui.ttf",
        "C:\\Windows\\Fonts\\calibri.ttf",
        "C:\\Windows\\Fonts\\arial.ttf"
    };
    const char* boldPaths[] = {
        "C:\\Windows\\Fonts\\seguisb.ttf",
        "C:\\Windows\\Fonts\\segoeuib.ttf",
        "C:\\Windows\\Fonts\\calibrib.ttf",
        "C:\\Windows\\Fonts\\arialbd.ttf"
    };

    ImFont* mainFont  = TryLoadFont(regularPaths, 3, 15.0f, &fontCfg);
    if (!mainFont) mainFont = io.Fonts->AddFontDefault();

    ImFont* boldFont  = TryLoadFont(boldPaths, 4, 15.0f, &fontCfg);
    ImFont* brandFont = TryLoadFont(boldPaths, 4, 24.0f, &fontCfg);

    printf("[Font] main=%p bold=%p brand=%p\n",
           (void*)mainFont, (void*)boldFont, (void*)brandFont);

    g_fontMain  = mainFont;
    g_fontBold  = boldFont ? boldFont : mainFont;
    g_fontBrand = brandFont ? brandFont : (boldFont ? boldFont : mainFont);

    // ── Style ──
    ImGui::StyleColorsDark();
    ImGuiStyle& s = ImGui::GetStyle();

    s.AntiAliasedLines = true;
    s.AntiAliasedLinesUseTex = true;
    s.AntiAliasedFill = true;

    s.WindowRounding    = 8.0f;
    s.ChildRounding     = 8.0f;
    s.FrameRounding     = 6.0f;
    s.GrabRounding      = 6.0f;
    s.PopupRounding     = 6.0f;
    s.ScrollbarRounding = 6.0f;
    s.TabRounding       = 6.0f;

    s.WindowPadding     = ImVec2(12, 12);
    s.FramePadding      = ImVec2(8, 5);
    s.ItemSpacing       = ImVec2(8, 6);
    s.ItemInnerSpacing  = ImVec2(6, 4);
    s.WindowBorderSize  = 0.0f;
    s.FrameBorderSize   = 0.0f;
    s.ScrollbarSize     = 10.0f;
    s.GrabMinSize       = 8.0f;

    ImVec4 accent       = ImVec4(0.55f, 0.33f, 0.92f, 1.00f);
    ImVec4 accentHover  = ImVec4(0.65f, 0.45f, 1.00f, 1.00f);
    ImVec4 accentActive = ImVec4(0.45f, 0.25f, 0.80f, 1.00f);
    ImVec4 bg           = ImVec4(0.08f, 0.08f, 0.10f, 0.96f);
    ImVec4 childBg      = ImVec4(0.09f, 0.09f, 0.12f, 1.00f);
    ImVec4 frameBg      = ImVec4(0.13f, 0.13f, 0.17f, 1.00f);
    ImVec4 frameBgHov   = ImVec4(0.17f, 0.17f, 0.23f, 1.00f);
    ImVec4 frameBgAct   = ImVec4(0.21f, 0.21f, 0.29f, 1.00f);
    ImVec4 border       = ImVec4(0.22f, 0.22f, 0.30f, 0.40f);

    s.Colors[ImGuiCol_WindowBg]             = bg;
    s.Colors[ImGuiCol_ChildBg]              = childBg;
    s.Colors[ImGuiCol_PopupBg]              = ImVec4(0.09f, 0.09f, 0.13f, 0.96f);
    s.Colors[ImGuiCol_Border]               = border;
    s.Colors[ImGuiCol_BorderShadow]         = ImVec4(0, 0, 0, 0);
    s.Colors[ImGuiCol_Text]                 = ImVec4(0.90f, 0.90f, 0.93f, 1.00f);
    s.Colors[ImGuiCol_TextDisabled]         = ImVec4(0.50f, 0.50f, 0.58f, 1.00f);
    s.Colors[ImGuiCol_FrameBg]              = frameBg;
    s.Colors[ImGuiCol_FrameBgHovered]       = frameBgHov;
    s.Colors[ImGuiCol_FrameBgActive]        = frameBgAct;
    s.Colors[ImGuiCol_TitleBg]              = ImVec4(0.06f, 0.06f, 0.08f, 1.00f);
    s.Colors[ImGuiCol_TitleBgActive]        = ImVec4(0.10f, 0.08f, 0.16f, 1.00f);
    s.Colors[ImGuiCol_TitleBgCollapsed]     = ImVec4(0.06f, 0.06f, 0.08f, 0.75f);
    s.Colors[ImGuiCol_Button]               = ImVec4(0.16f, 0.16f, 0.22f, 1.00f);
    s.Colors[ImGuiCol_ButtonHovered]        = accentHover;
    s.Colors[ImGuiCol_ButtonActive]         = accentActive;
    s.Colors[ImGuiCol_Header]               = ImVec4(0.16f, 0.14f, 0.24f, 0.80f);
    s.Colors[ImGuiCol_HeaderHovered]        = accentHover;
    s.Colors[ImGuiCol_HeaderActive]         = accentActive;
    s.Colors[ImGuiCol_Tab]                  = ImVec4(0.12f, 0.12f, 0.18f, 1.00f);
    s.Colors[ImGuiCol_TabHovered]           = accentHover;
    s.Colors[ImGuiCol_TabSelected]          = accent;
    s.Colors[ImGuiCol_TabDimmed]            = ImVec4(0.10f, 0.10f, 0.14f, 1.00f);
    s.Colors[ImGuiCol_TabDimmedSelected]    = ImVec4(0.28f, 0.20f, 0.48f, 1.00f);
    s.Colors[ImGuiCol_CheckMark]            = accent;
    s.Colors[ImGuiCol_SliderGrab]           = accent;
    s.Colors[ImGuiCol_SliderGrabActive]     = accentActive;
    s.Colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.06f, 0.06f, 0.08f, 0.40f);
    s.Colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.28f, 0.28f, 0.36f, 1.00f);
    s.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.38f, 0.38f, 0.48f, 1.00f);
    s.Colors[ImGuiCol_ScrollbarGrabActive]  = accent;
    s.Colors[ImGuiCol_Separator]            = border;
    s.Colors[ImGuiCol_SeparatorHovered]     = accentHover;
    s.Colors[ImGuiCol_SeparatorActive]      = accent;
    s.Colors[ImGuiCol_ResizeGrip]           = ImVec4(0.24f, 0.24f, 0.32f, 0.40f);
    s.Colors[ImGuiCol_ResizeGripHovered]    = accentHover;
    s.Colors[ImGuiCol_ResizeGripActive]     = accent;

    g_debug.gameWnd = g_gameWnd;

    printf("[+] Overlay ready\n");
    printf("[*] INSERT  toggle menu\n");
    printf("[*] END     exit\n");

    // Close the console window now that we're running
    if (fp) { fclose(fp); fp = nullptr; }
    FreeConsole();

    // ── Main Loop ──
    MSG msg{};
    while (g_running) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) g_running = false;
        }
        if (!g_running) break;

        if (GetAsyncKeyState(VK_END) & 1) { g_running = false; break; }
        if (GetAsyncKeyState(VK_INSERT) & 1) g_menuVisible = !g_menuVisible;

        LONG_PTR exStyle = g_menuVisible
            ? (WS_EX_TOPMOST | WS_EX_LAYERED)
            : (WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED);
        SetWindowLongPtrW(g_overlay, GWL_EXSTYLE, exStyle);

        if (!IsWindow(g_gameWnd)) { g_running = false; break; }
        RECT r;
        GetWindowRect(g_gameWnd, &r);
        int w = r.right - r.left, h = r.bottom - r.top;
        if (w != g_width || h != g_height || r.left != gr.left || r.top != gr.top) {
            MoveWindow(g_overlay, r.left, r.top, w, h, TRUE);
            g_width = w; g_height = h;
            gr = r;
        }

        // ── Game data ──
        uintptr_t localController = mem.Read<uintptr_t>(mem.client + offsets::client::dwLocalPlayerController);
        uintptr_t localPawn       = mem.Read<uintptr_t>(mem.client + offsets::client::dwLocalPlayerPawn);

        g_debug.localController = localController;
        g_debug.localPawn = localPawn;

        Matrix4x4 viewMatrix{};
        uint8_t localTeam = 0;

        if (localController && localPawn) {
            localTeam = mem.Read<uint8_t>(localPawn + offsets::entity::m_iTeamNum);
            g_debug.localTeam = localTeam;
            g_debug.localHealth = mem.Read<int>(localPawn + offsets::entity::m_iHealth);
            g_debug.viewAngles = mem.Read<Vector3>(mem.client + offsets::client::dwViewAngles);

            UpdateEntityData(localPawn, localTeam);
            viewMatrix = mem.Read<Matrix4x4>(mem.client + offsets::client::dwViewMatrix);
            g_debug.viewMatrix00 = viewMatrix.m[0][0];
            g_debug.entityCount = g_entityCount;

            Misc::Bhop(localPawn);
            Misc::NoFlash(localPawn);
            Misc::Triggerbot(localPawn, localTeam);
        } else {
            g_entityCount = 0;
            g_debug.entityCount = 0;
        }

        // ── Render ──
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (localController && localPawn) {
            ESP::Render(g_entities, g_entityCount, viewMatrix, g_width, g_height);
            Aimbot::Run(g_entities, g_entityCount, localPawn, g_width, g_height);

            if (config.bAimbot && config.bFovCircle) {
                float radPerDeg = 3.14159265f / 180.0f;
                float screenRadius = tanf(config.aimFov * radPerDeg) / tanf(45.0f * radPerDeg) * (g_width * 0.5f);
                ImGui::GetBackgroundDrawList()->AddCircle(
                    ImVec2(g_width * 0.5f, g_height * 0.5f),
                    screenRadius, IM_COL32(255, 255, 255, 180), 64, 1.0f);
            }
        }

        if (g_menuVisible)
            RenderMenu();

        ImGui::Render();
        const float clear[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        g_pd3dContext->OMSetRenderTargets(1, &g_pRTV, nullptr);
        g_pd3dContext->ClearRenderTargetView(g_pRTV, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
    }

    // ── Cleanup ──
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    DestroyWindow(g_overlay);
    UnregisterClassW(wc.lpszClassName, hInst);
    mem.Cleanup();

    return 0;
}
