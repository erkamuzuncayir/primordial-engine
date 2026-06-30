# Primordial Engine

![Language](https://img.shields.io/badge/language-C%2B%2B20-blue)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-lightgrey)
![Graphics API](https://img.shields.io/badge/graphics-Vulkan%20%7C%20DirectX%2011-red)
![License](https://img.shields.io/badge/license-MIT-green)

> [!IMPORTANT]
> **This GitHub repository is a read-only mirror.**
> All active development, issue tracking, and contributions are hosted on my personal [forgejo](https://forgejo.org/) instance. Please visit **[git.uzuncayir.dev/erkam/primordial-engine](https://git.uzuncayir.dev/erkam/primordial-engine)** to access the latest source code, report bugs, or submit pull requests.

**Primordial Engine** is an under-development C++ game engine project aimed at high-performance, data-oriented design. Currently in its `v0.x` phase, the project features a custom ECS architecture and is actively building out its core rendering foundation to support both Vulkan and DirectX 11 backends.

## 🚧 Active Development
> *This section tracks my immediate focus. For the macro-level overview, visit the [**Primordial Engine Roadmap**](https://git.uzuncayir.dev/erkam/primordial-engine/projects/1).*

- [ ] ⚙️ **Physics:** Start to implement physics module
- [ ] 🏗️ **Architecture:** Refactor Engine to Dynamic Library (DLL)

---

## 🎯 Philosophy & Roadmap

### Pragmatic C++ Approach
I leverage modern C++ (C++20) features selectively. Rather than chasing the latest standards for the sake of novelty, I adopt new features only when they offer tangible architectural, maintainability, or performance benefits.

### Development Mindset
My development process balances two core pillars:
1.  **Fundamental Engine Features:** Implementing essential systems required for any robust engine, prioritizing rapid decision-making and implementation over extensive pre-documentation.
2.  **"Dogfooding":** Validating and expanding these features by building actual games.

---

## 🎮 Demo & Showcase

If you download, build, and run the project (or use the pre-built binaries from Releases), you will be greeted by the primary demo scene.
*Note: Runtime scene changes via the Inspector are currently non-persistent. To make permanent changes, please edit the `{sceneName}.ini` located in the `assets/demo-scenes/{sceneName}` directory.*

### 🏜️ Featured Scene: The Desert Snow Globe
The main demo simulates a unique environment: a snow globe situated in a desert biome. It integrates core systems to create a reactive world:
- **Dynamic Weather & Seasons:** A Day/Night and Season system directly communicates with the GPU particle engine.
- **Reactive Environment:** Trees organically grow during precipitation and shrink during prolonged sunlight.

### 🕹️ Default Viewport Controls

Use these default keybindings to navigate the demo scene:

| Action                       | Keybinding                                    |
| :--------------------------- | :-------------------------------------------- |
| **Select Camera 1, 2, 3**    | `F1`, `F2`, `F3`                              |
| **Rotate Camera**            | `W` (Up), `S` (Down), `A` (Left), `D` (Right) |
| **Pan Camera**               | `Ctrl` + `W/S/A/D`                            |
| **Move Camera Forward**      | `Ctrl` + `Q`                                  |
| **Move Camera Backward**     | `Ctrl` + `E`                                  |
| **Trigger Particle Emitter** | `F4`                                          |
| **Reset Scene**              | `R`                                           |

### 📼 Technical Demonstrations

> **Note:** Click on the thumbnails below to watch the full demonstration videos on YouTube.
> 
<table>
  <tr>
    <td align="center" width="50%" valign="top">
      <h3>1. Shader Variety</h3>
      <br>
      <a href="https://youtu.be/kWjpq85oEes">
        <img src="https://img.youtube.com/vi/kWjpq85oEes/maxresdefault.jpg" alt="Shader Variety" style="width:100%;">
      </a>
      <br><br>
      <p><em>Demonstration of <b>Unlit</b>, <b>Gouraud Lit</b>, and <b>Phong Lit</b> shaders working seamlessly.</em></p>
    </td>
    <td align="center" width="50%" valign="top">
      <h3>2. Normal Mapping</h3>
      <br>
      <a href="https://youtu.be/OqJbEtCg8QU">
        <img src="https://img.youtube.com/vi/OqJbEtCg8QU/maxresdefault.jpg" alt="Normal Mapping" style="width:100%;">
      </a>
      <br><br>
      <p><em>Showcasing normal mapping in different objects in scene.</em></p>
    </td>
  </tr>
  <tr>
    <td align="center" width="50%" valign="top">
      <h3>3. Advanced Material Effects</h3>
      <br>
      <a href="https://youtu.be/_U6o-T8-XqU">
        <img src="https://img.youtube.com/vi/_U6o-T8-XqU/maxresdefault.jpg" alt="Advanced Material Effects" style="width:100%;">
      </a>
      <br><br>
      <p><em>A double-sided shader demonstration used for the snow globe (Skybox inner pass, Fresnel glass outer pass).</em></p>
    </td>
    <td align="center" width="50%" valign="top">
      <h3>4. Dynamic Shadows & Transforms</h3>
      <br>
      <a href="https://youtu.be/uSxcTGjZ-4A">
        <img src="https://img.youtube.com/vi/uSxcTGjZ-4A/maxresdefault.jpg" alt="Dynamic Shadows & Transforms" style="width:100%;">
      </a>
      <br><br>
      <p><em>Real-time shadow mapping reacting to object scaling/transformation.</em></p>
    </td>
  </tr>
  <tr>
    <td align="center" width="50%" valign="top">
      <h3>5. GPU Particle System</h3>
      <br>
      <a href="https://youtu.be/MHAIRJkfjI8">
        <img src="https://img.youtube.com/vi/MHAIRJkfjI8/maxresdefault.jpg" alt="GPU Particle System" style="width:100%;">
      </a>
      <br><br>
      <p><em>High-performance GPU-instanced particles generating fire and smoke effects.</em></p>
    </td>
    <td align="center" width="50%" valign="top">
      <h3>6. Editor Integration</h3>
      <br>
      <a href="https://youtu.be/Etl8kPQbWW4">
        <img src="https://img.youtube.com/vi/Etl8kPQbWW4/maxresdefault.jpg" alt="Editor Integration" style="width:100%;">
      </a>
      <br><br>
      <p><em>ImGui-based editor demonstrating real-time component inspection.</em></p>
    </td>
  </tr>
</table>

---

## ⚙️ Technical Features

### Core Systems
- **Custom ECS:** High-performance Entity-Component-System.
- **Logging:** Custom console logger with file rotation support.
- **Math:** A GLM wrapper providing essential math functionalities.
- **Timer:** High-resolution delta-time calculation.

### Input System
- **Event-Driven:** Subscription-based input handling via GLFW callbacks.

### Scene Management
- **Serialization:** Full scene creation via custom `.ini` files.
- **Hot-Reload:** Runtime reloading for entities and components.
    - *Note: Asset/Resource (Mesh, Texture, Shader) hot-reload is currently not supported.*
- **Environment:** Real-time Day/Night & Season progression logic.

### 🎨 Rendering

#### Vulkan (Primary Backend)
- **Shading Models:** Unlit, Gouraud (Per-Vertex), Phong (Per-Pixel).
- **Error Fallback:** Automatic "Hot-Pink" shader for failed resources.
- **Lighting:** Directional lights with Shadow Mapping.
- **Particles:** GPU-instanced particle system.
- **Texture Mapping:** Albedo and Normal mapping support.
- **Mesh Generation:** Procedural primitives (Cube, Sphere, Geosphere, Cylinder, Quad, Grid).
- **Camera:** Multi-camera management system.

#### DirectX 11 (Experimental Backend)
> *Status: Currently disabled in build configuration. Requires development for feature parity.*
- **Shading Models:** Basic Per-Pixel shading.
- **Texture Mapping:** Albedo support.

### Asset Management
- **Cached Registry:** Efficient management for Models, Textures, and Shaders.
- **Importers:**
    - Models: Custom `.obj` loader with limited material support.
    - Textures: `stb_image` integration.

### Editor Tools (ImGui)
- **Scene Graph:** Hierarchical view of active entities.
- **Inspector:** Real-time editable properties for components.
- **Asset Browser:** Read-only view of loaded resources.

---

## 🛠️ Build Instructions

This project uses **CMake Presets** to simplify configuration across different platforms and compilers. All presets utilize **Ninja** as the build generator for maximum performance.

### Prerequisites
Ensure you have the following installed and available in your system PATH:
1.  **CMake** (3.20+)
2.  **Ninja Build System** (Required for all presets)
3.  **Git**
4.  **Compiler:**
    * *Windows:* Visual Studio 2022 (MSVC) **OR** LLVM (Clang-CL)
    * *Linux:* Clang++ **OR** GCC

### 1. Clone the Repository
```bash
git clone --recursive https://github.com/erkamuzuncayir/primordial-engine.git
cd primordial-engine
```

### 2. Build using Presets

Select the preset that matches your OS and compiler preference.

#### 🪟 Windows (Clang-CL)

Uses `clang-cl.exe` and `lld-link.exe` with Ninja.

```bash
# Configure (Debug)
cmake --preset Debug-Clangd-Windows

# Build
cmake --build build/Debug-Clangd-Windows

```

#### 🪟 Windows (MSVC)

Uses the standard `cl.exe` compiler with Ninja.

```bash
# Configure (Debug)
cmake --preset Debug-MSVC

# Build
cmake --build build/Debug-MSVC

```

#### 🐧 Linux (Clang)

Uses `clang` and `clang++` with Ninja.

```bash
# Configure (Debug)
cmake --preset Debug-Clang-Linux

# Build
cmake --build build/Debug-Clang-Linux

```

#### 🐧 Linux (GCC)

Uses the standard `gcc` and `g++` compilers with Ninja.

```bash
# Configure (Debug)
cmake --preset Debug-GCC

# Build
cmake --build build/Debug-GCC

```

> **Note:** To build for **Release** mode, simply replace `Debug` with `Release` in the preset names (e.g., `Release-Clangd-Windows` or `Release-Clang-Linux`).

---

## 🏃 Run Dependencies

### Windows

* Visual C++ Runtime (Usually installed with Visual Studio)

### Linux (Debian/Ubuntu)

If you compiled the project from source, the build dependencies should cover runtime needs. If running a pre-built binary, ensure you have:

```bash
sudo apt-get install libvulkan1 mesa-vulkan-drivers libwayland-client0 libwayland-cursor0 libwayland-egl1
```
