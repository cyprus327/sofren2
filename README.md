# sofren2

A high performance C, single header, software renderer built around modern CPU architectures,
using a multithreaded, tile based rendering pipeline with an advanced meshlet architecture, all designed around SIMD

---

<details>
  <summary>About...</summary>
  Realtime soft analytical point light shadows at 1080p
  
  Character model purchased from itch.io and animations from Mixamo, the scene here was made in Trenchbroom,
  with lighting baked using EricW Tools suite and then converted to custom QSF2 binary format.

  The character used is very low poly but this doesn't change shadow quality at all. The shadows aren't limited to
  just humanoid models but do require some skeleton for the shadow proxies (of which there are 10 in this video: 1 for head, 1 for torso, 2 for
  each leg, 2 for each arm) to be automatically generated perfectly like this. If there is no skeleton then a BVH will be
  generated and used, which will (probably) make a worse and more random fit or require more proxies (decreasing performance).

  Currently the max lights that can be casting a dynamic shadow onto one pixel is hardcoded to 16, this seems very overkill to me and could possibly change.

  You'll notice in the first half of the video that shadows do not go through walls, in some cases this can sort of clash with baked lighting and it also
  adds extra raycasts, so a preprocessing setting may be added to toggle this.
</details>
<div align="center">
  <video src="https://github.com/user-attachments/assets/9d05b8ce-95dd-490c-8081-01213af7eae6" width="100%" autoplay loop muted playsinline controls></video>
</div>

Sponza at 260 FPS (rendered at 1920x1080, no gamma correction)
<details>
  <summary>About...</summary>
  This looks washed out because gamma correction isn't in sofren2 yet.
  This was rendered with 32 worker threads and AVX2 on a laptop with an Intel i9-13900HX.
</details>
<img width="959" height="503" alt="image" src="https://github.com/user-attachments/assets/e70fd3fc-8ba0-40e3-bd87-35c72b39aa2e" />

## Core Features

<details>
  <summary><b>Multithreaded Rasterizer & Lock Free Binning</b></summary>
  Implements a deferred tile-based visibility buffer pipeline utilizing a custom job system. Threads process geometry and write 64-bit packed payloads (instance ID, meshlet ID, batch index, active lane mask) to thread-local chunk allocators, completely avoiding atomic locks during screen-space binning.
</details>

<details>
  <summary><b>Explicit SIMD Vectorization & SoA Layout</b></summary>
  Hardware accelerated processing via explicit AVX2 and ARM NEON intrinsics, all internal data structures utilize a Structure of Arrays layout to maximize register and lane utilization.
</details>

<details>
  <summary><b>Meshlet Driven Pipeline</b></summary>
  Geometry is preprocessed into optimized `SFT_VERTS_PER_MESHLET` (64 default) vertex meshlets, which enables aggressive early out frustum checks (via bounding spheres) and normal-cone backface culling before any vertices are (expensively) transformed or skinned.
</details>

<details>
  <summary><b>Analytical Soft Shadows & 8-Way SIMD BVH</b></summary>
  High performance perfectly soft shadows are calculated via raymarching against dynamic capsule proxies, entirely bypassing traditional shadow maps. Raycasting traverses an 8-branching BVH where each node stores 8 child AABBs in SoA layout, testing against 8 bounding boxes simultaneously.
</details>

<details>
  <summary><b>OS Level Virtual Memory Arenas</b></summary>
  Eliminates standard heap fragmentation and per frame `malloc` overhead by reserving large blocks of virtual address space upfront (`VirtualAlloc` / `mmap`), with physical memory pages strictly committed on demand when bump pointers cross page boundaries.
</details>

<details>
  <summary><b>Cache Optimized Texturing</b></summary>
  Textures are automatically swizzled into 4x4 Morton style blocks to maximize CPU cache hits during sampling. Supports mipmapping alongside bilinear and nearest filtering.
</details>

<details>
  <summary><b>Skeletal Animation</b></summary>
  Full hierarchical skeletal animation and 4-joint skinning evaluated directly within the meshlet pipeline.
</details>

<details>
  <summary><b>Immediate Mode Geometry (dmesh)</b></summary>
  Built in API for dynamically generating primitives (cubes, spheres, capsules, cylinders) and custom geometry on the fly.
</details>

<details>
  <summary><b>Built in Profilers & Anti Aliasing</b></summary>
  Realtime visualizers for wireframe, tile loads, depth buffers, light heatmaps, and checkerboard textures. Also features built in high performance 2x and 4x MSAA.
</details>

## Not Yet Implemented

All of these things exist in the original [sofren](https://github.com/cyprus327/sofren/), and would be fairly easy to port

*I could also be forgetting something here, this isn't guaranteed to be a 100% complete list*

- Gamma correction
- Skyboxes
- OBJ file loading
- PBR textures (metallic/roughness and normals), this probably won't be implemented due to how extreme the performance decrease is for fairly little visual benefit
- Standard shadowmaps

## Integration & Quick Start

Copy `sofren2.c` into your project and define `SFT_IMPL` in exactly **one** file

There is no real reason that it is a .c file instead of a .h file, it doesn't need to be a .c file and will work perfectly if you want to rename it to "sofren2.h" or something

```c
#define SFT_IMPL
#include "sofren2.c"
```

### Optional Dependencies

- Define `SFT_USE_CGLTF` and `SFT_CGLTF_PATH` (the location of `cgltf.h`) to enable `.glb` / `.gltf` model parsing
- Define `SFT_USE_STB_IMAGE` and `SFT_STB_IMAGE_PATH` (the location of `stb_image.h`) to enable loading `.png`, `.jpg`, etc

## Gallery

*More videos/screenshots coming*

Quake II rendering with [sofren](https://github.com/cyprus327/sofren/) (click to view video)
[![Quake II Demo Video](https://img.youtube.com/vi/hax09bEz7jo/maxresdefault.jpg)](https://www.youtube.com/watch?v=hax09bEz7jo)

## Licence

This project is under the MIT licence
