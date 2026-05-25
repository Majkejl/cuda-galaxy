# cuda-nbody

Real-time 3D N-body galaxy collision simulator — CUDA + OpenGL.

Two galaxies of ~16k–32k particles each collide under all-pairs O(N²) Newtonian gravity.
Physics runs entirely on the GPU; positions are written directly to an OpenGL VBO via
CUDA-OpenGL interop (no CPU round-trip). Runtime toggle between naive and shared-memory
tiled kernels enables a live benchmark comparison.

**Status:** In development

---

## Requirements

| Dependency | Version |
|---|---|
| CUDA Toolkit | 13.x |
| CMake | 3.29+ |
| VS Build Tools | 2022 (MSVC) |
| Ninja | any recent |
| GPU | NVIDIA, compute capability ≥ 7.0 (tested on cc 8.6) |

GLFW and GLAD are fetched automatically by CMake — no manual installs needed.

## Build

```bat
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Shaders are copied next to the executable by a post-build step.

## Controls

| Key | Action |
|---|---|
| Left-drag | Orbit camera |
| Scroll | Zoom |
| `K` | Toggle naive / tiled kernel |
| `Space` | Pause / resume |
| `+` / `-` | Increase / decrease softening ε |
| `[` / `]` | Decrease / increase timestep |

## Performance (target)

| N | Naive kernel | Tiled kernel (TILE=256) |
|---|---|---|
| 4 096 | ~60 fps | ~60 fps |
| 16 384 | ~? fps | ~60 fps |
| 32 768 | ~? fps | ~? fps |

*(Table filled in after benchmarking)*
