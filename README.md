# Image-Based Lighting — Real-Time PBR Renderer

A real-time rendering pipeline implementing image-based lighting (IBL) with
a physically based material model (Cook-Torrance microfacet BRDF).
Built for the *Advanced Computer Graphics 2025/26* seminar (2.9 Image-based lighting).

## Features

- **HDR environment loading** — equirectangular `.hdr` images via stb_image
- **Equirectangular → cubemap** conversion (512² per face)
- **Diffuse irradiance map** — cosine-weighted hemisphere convolution (32² per face)
- **Specular prefiltered environment map** — GGX importance sampling across 5 mip levels (128² base)
- **BRDF integration LUT** — split-sum approximation (512²), precomputed via Monte Carlo
- **PBR shader** — Cook-Torrance: GGX NDF, Schlick Fresnel, Smith geometry
- **Reinhard tonemapping + gamma correction**
- **ImGui controls** — roughness, metallic, albedo, exposure, environment selection,
  diffuse/specular/background toggles, grid vs single-sphere mode

## Requirements

- C++17 compiler (Clang, GCC, MSVC)
- CMake 3.16+
- Python 3 (for GLAD GL loader generation at build time)
- OpenGL 3.3+ capable GPU

All other dependencies (GLFW, GLM, GLAD, Dear ImGui, stb_image) are fetched
automatically by CMake.

## Build

```bash
git clone <repo-url> && cd nrg-seminar
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

On macOS replace `$(nproc)` with `$(sysctl -n hw.ncpu)`.

## Run

Download at least one HDR environment map and place it in `assets/env/`:

```bash
# Example: download from Poly Haven (CC0)
curl -L -o assets/env/abandoned_parking.hdr \
  "https://dl.polyhaven.org/file/ph-assets/HDRIs/hdr/1k/abandoned_parking_1k.hdr"
```

Then run:

```bash
cd build
./nrg_seminar
```

### Controls

| Input | Action |
|-------|--------|
| Left mouse drag | Orbit camera |
| Scroll wheel | Zoom |
| Escape | Quit |
| ImGui panel | Material parameters, exposure, IBL toggles |

## Pipeline Overview

```
HDR file ──► equirect texture ──► environment cubemap (512²)
                                       │
                        ┌──────────────┼──────────────┐
                        ▼              ▼              ▼
                 irradiance map  prefiltered map  BRDF LUT
                    (32²)        (128² + mips)    (512²)
                        │              │              │
                        └──────┬───────┘              │
                               ▼                      │
                      PBR fragment shader ◄───────────┘
                               │
                          tonemapping
                               │
                           framebuffer
```

## Project Structure

```
├── CMakeLists.txt
├── src/
│   ├── main.cpp              Entry point, render loop
│   ├── gl_context.cpp/.h     Shader loading, cube/quad primitives
│   ├── env_map.cpp/.h        HDR loading, equirect→cubemap
│   ├── irradiance.cpp/.h     Diffuse irradiance convolution
│   ├── prefilter.cpp/.h      Specular prefilter (GGX importance sampling)
│   ├── brdf_lut.cpp/.h       BRDF split-sum LUT generation
│   ├── pbr_renderer.cpp/.h   Sphere mesh, orbit camera
│   └── ui.cpp/.h             Dear ImGui interface
├── shaders/
│   ├── equirect_to_cube.*    Equirectangular → cubemap
│   ├── irradiance_conv.*     Hemisphere convolution
│   ├── prefilter_specular.*  GGX importance sampling
│   ├── brdf_lut.*            Split-sum integration
│   ├── pbr.*                 Cook-Torrance PBR + IBL
│   └── skybox.*              Environment background
├── assets/env/               Place .hdr files here
└── docs/                     Asset attribution
```

## License

MIT — see [LICENSE](LICENSE).

HDR environment maps from [Poly Haven](https://polyhaven.com/hdris) are CC0.
