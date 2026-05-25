<p align="center">
  <img src="https://img.shields.io/badge/Game-Counter--Strike%202-yellow?style=for-the-badge&logo=steam" />
  <img src="https://img.shields.io/badge/Type-External-blue?style=for-the-badge" />
  <img src="https://img.shields.io/badge/Language-C++20-00599C?style=for-the-badge&logo=cplusplus" />
  <img src="https://img.shields.io/badge/Overlay-DirectX%2011-green?style=for-the-badge" />
</p>

# 🎯 CS2 External

A feature-rich **external overlay** for Counter-Strike 2, built with **C++20**, **DirectX 11**, and a fully custom **ImGui** UI. Runs as a separate process — no DLL injection required.

---

## ✨ Features

### 👁️ Visuals (ESP)
| Feature | Description |
|---------|-------------|
| **Bounding Boxes** | 2D boxes around enemies with customizable color |
| **Skeleton** | Full bone-based skeleton rendering |
| **Health Bar** | Color-coded HP bar on each player |
| **Name Tags** | Player name displayed above each enemy |
| **Snaplines** | Lines from screen bottom to enemy position |
| **Armor Bar** | Visual armor indicator |
| **Distance** | Shows distance in meters to each enemy |
| **Team Filter** | Toggle to hide/show teammates |

### 🎯 Aimbot
| Feature | Description |
|---------|-------------|
| **Silent Aim** | Smoothed aim assistance toward targets |
| **Adjustable FOV** | Field of view radius (1° – 30°) |
| **Smoothing** | Humanized aim movement (1x – 20x) |
| **Bone Selection** | Target Head, Neck, or Chest |
| **RCS (Recoil Control)** | Automatic recoil compensation |
| **FOV Circle** | Visual FOV indicator on screen |
| **Custom Aim Key** | Bindable activation key |

### 🔫 Triggerbot
| Feature | Description |
|---------|-------------|
| **Auto Fire** | Automatically shoots when crosshair is on enemy |
| **Adjustable Delay** | Configurable reaction delay (0–100ms) |
| **Custom Trigger Key** | Bindable activation key |

### 🏃 Misc
| Feature | Description |
|---------|-------------|
| **Bunny Hop** | Automatic jump chaining for speed |
| **No Flash** | Removes flashbang screen effect |

---

## 🖥️ Menu Preview

The overlay features a **dark-themed sidebar UI** with smooth animations:

- **Sidebar Navigation** — Clean tabbed layout with Visuals, Aimbot, and Misc sections
- **Toggle Switches** — Animated on/off switches for every feature  
- **Sliders & Inputs** — Fine-tune FOV, smoothing, delays, and more
- **Color Pickers** — Full RGBA color customization for ESP elements
- **Key Bind Buttons** — Click-to-bind system for aim and trigger keys
- **Collapsible Groups** — Organized settings in expandable sections
- **Show/Hide Menu** — Press `INSERT` to toggle the menu

---

## 🔧 Build Instructions

### Requirements
- **Visual Studio 2022** (MSVC v143 toolset)
- **Windows SDK** (10.0+)
- **DirectX 11** (included with Windows)

### Build
1. Open `CS2External.sln` in Visual Studio 2022
2. Set configuration to **Release | x64**
3. Build the solution (`Ctrl+Shift+B`)
4. Output: `bin/Release/CS2External.exe`

Or build via command line:
```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" `
    CS2External.vcxproj /p:Configuration=Release /p:Platform=x64
```

---

## 🚀 Usage

1. Launch **Counter-Strike 2** and wait until you're in the main menu
2. Run `CS2External.exe` **as Administrator**
3. The overlay will automatically attach to the CS2 window
4. Press **INSERT** to toggle the menu
5. Configure your settings and play

---

## 📁 Project Structure

```
CS2External/
├── main.cpp           # Entry point, DX11 overlay, ImGui UI
├── memory.h           # Process memory read/write (RPM/WPM)
├── sdk.h              # Game structs (Vector3, Matrix4x4, bones)
├── offsets.h          # All game offsets (cs2-dumper based)
├── config.h           # Feature configuration struct
├── features/
│   ├── esp.h/cpp      # ESP rendering (boxes, skeleton, health, etc.)
│   ├── aimbot.h/cpp   # Aimbot with smoothing & bone selection
│   └── misc.h/cpp     # Bhop, NoFlash, Triggerbot
└── imgui/             # Dear ImGui library (v1.92+)
```

---

## ⚠️ Disclaimer

This project is for **educational purposes only**. Use at your own risk. The authors are not responsible for any consequences resulting from the use of this software. Using cheats in online games violates the terms of service and may result in a permanent ban.

---

## 📋 Offsets

Offsets are sourced from [a2x/cs2-dumper](https://github.com/a2x/cs2-dumper) and may need updating after CS2 game patches.

---

<p align="center">
  <sub>Built with ❤️</sub>
</p>
