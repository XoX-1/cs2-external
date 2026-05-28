#pragma once

struct Config {
    // ESP
    bool bEsp = false;
    bool bEspBox = true;
    bool bEspSkeleton = true;
    bool bEspHealth = true;
    bool bEspName = true;
    bool bEspSnaplines = false;
    bool bEspArmor = false;
    bool bEspDistance = false;
    bool bEspTeamCheck = true;
    float espBoxColor[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
    float espSkeletonColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float espSnaplineColor[4] = { 1.0f, 1.0f, 0.0f, 1.0f };

    // Aimbot
    bool bAimbot = false;
    int aimKey = 0x02;          // VK_RBUTTON
    float aimFov = 5.0f;
    float aimSmooth = 5.0f;
    int aimBone = 7;            // BoneIndex::HEAD
    bool bRcs = true;
    bool bFovCircle = false;

    // Triggerbot
    bool bTriggerbot = false;
    int triggerKey = 0x12;      // VK_MENU (ALT)
    int triggerDelay = 10;      // ms

    // Misc
    bool bBhop = false;
    bool bNoFlash = false;

};

inline Config config;
