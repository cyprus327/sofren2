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
  This looks washed out because gamma correction isn't in sofren2, it would take like 10 lines to add it I just haven't yet.
  This was rendered with 32 worker threads and AVX2 on a laptop with an Intel i9-13900HX.
</details>
<img width="959" height="503" alt="image" src="https://github.com/user-attachments/assets/e70fd3fc-8ba0-40e3-bd87-35c72b39aa2e" />

## Core Features

*See the [Architecture](#architecture) section for more info on these*

- **Multithreaded Rasterizer:** Tile based multithreaded rendering pipeline utilizing a custom job system and lock free binning
- **Explicit SIMD Vectorization:** Hardware accelerated processing via AVX2 and ARM NEON, with everything designed around a SoA layout
- **Meshlet Driven Pipeline:** Geometry is pre processed into optimized 64 vertex meshlets, enabling aggressive early out frustum and normal cone backface culling
- **Analytical Soft Shadows:** High performance soft shadows calculated via raymarching against dynamic capsule proxies, completely bypassing traditional shadow maps
- **Skeletal Animation:** Full hierarchical skeletal animation and 4 joint skinning evaluated directly within the meshlet pipeline
- **Cache Optimized Texturing:** Textures are automatically swizzled into 4x4 blocks to maximize CPU cache hits, supporting mipmapping, bilinear, and nearest filtering
- **Dynamic & Baked Lighting:** Supports physical point and directional lights (Blinn-Phong) alongside overbrightened baked lightmap atlases
- **Immediate Mode Geometry (`dmesh`):** Built in API for dynamically generating primitives (cubes, spheres, capsules, cylinders) and custom geometry on the fly
- **BVH Accelerated Raycasting:** Integrated raycasting against the exact posed geometry of active instances for precise collision and picking
- **Custom Virtual Memory Arenas:** Sub allocates from massive up front OS level virtual memory reservations to eliminate standard heap fragmentation
- **Asset Pipelines:** Parses GLB/glTF assets natively via `cgltf`, alongside a custom `QSF2` binary format
- **Built in Profilers:** Real time visualizers for wireframe, tile loads, depth buffers, light heatmaps, and checkerboard texture

## Not Yet Implemented

All of these things exist in the original [sofren](https://github.com/cyprus327/sofren/), and would be fairly easy to port

*I could also be forgetting something here, this isn't guaranteed to be a 100% complete list*

- Gamma correction
- Skyboxes
- OBJ file loading
- PBR textures (metallic/roughness and normals), this probably won't be implemented due to how extreme the performance decrease is for fairly little visual benefit
- Standard shadowmaps

## Architecture

sofren2 is structured around a parallel tile based visibility buffer pipeline, where instead of rendering triangles immediately to a global framebuffer
the engine processes geometry into a cached state, bins triangles into screen space tiles, and then resolves the final image tile by tile

### The Rendering Pipeline

#### 1: Geometry Processing & Binning

The main thread pushes meshlet jobs to a global lock free queue, and worker threads begin pulling them

- **Meshlet Culling & Skinning:** Each job evaluates up to 8 meshlets, threads first check the meshlet's bounding sphere and normal cone against the camera frustum,
if the meshlet survives, its vertices are transformed and skinned
- **Screen-Space Caching:** Transformed world space positions, normals, and projected clip space coordinates are written to a large contiguous "G buffer" array
- **Clipping & Binning:** Triangles intersecting the near plane are dynamically clipped into sub triangles, valid triangles have their 2D bounding boxes
calculated and are assigned to underlying screen tiles
- **Lock Free Payloads:** Instead of storing full triangle data in tiles, the engine packs a 64 bit payload containing the instance ID, meshlet ID,
batch index, and an active lane mask, these payloads are written to thread local chunk allocators to completely avoid atomic locks during binning

Before proceeding, the main thread intercepts the pipeline to sort the tiles by their payload counts (heaviest to lightest), ensuring optimal load balancing for the rasterization phase

#### 2: Rasterization (Visibility Buffer)

Workers switch to grabbing tiles from the sorted queue

- **SIMD Edge Equations:** For each tile, threads iterate through its linked list of payload chunks, payloads are unpacked and the cached screen space vertices
are fetched, the engine evaluates edge equations and depth gradients, rasterizing up to 8 triangles simultaneously depending on the hardware target
- **Local Depth Buffers:** Depth and visibility data (the unpacked payload IDs) are written to tile local arrays

#### 3: Shading & Resolve

Once a tile's geometry is fully rasterized, the thread resolves the final pixel colors

- **Barycentric Reconstruction:** For every visible pixel, the engine unpacks the stored ID, fetches the triangle vertices from the global cache,
and calculates perspective correct barycentric coordinates
- **Texture & Lightmap Sampling:** Using analytic gradients calculated from the barycentric coordinates, the engine selects the appropriate
mipmap level, texture data is fetched from swizzled 4x4 Morton style blocks via nearest or bilinear filtering
- **Analytical Soft Shadows:** If dynamic shadows are enabled, the engine calculates analytical capsule based penumbras, it builds a tight 2D frustum
from the active pixel cluster to the light source, checks against a TLAS of bounding boxes, and traces rays against overlapping geometry and dynamic joint capsules
- **Final Output:** The shaded pixels are written to the global color buffer, and the thread pulls the next tile

### Memory Architecture

sofren2 eliminates traditional heap fragmentation and the overhead of per frame `malloc` calls by implementing OS level virtual memory arenas

- **Virtual Reservations:** Arenas reserve massive blocks of virtual address space up front
- **On Demand Commits:** Physical memory pages are strictly committed only when the bump pointer crosses a physical page boundary
(handling Windows `VirtualAlloc` explicitly and POSIX `mmap` page faults implicitly)
- **Transient Lifetimes:** Arenas are categorized by lifespan, for example some are wiped clean at the start of every frame simply by
resetting their bump pointers to zero
- **Texture Heap Allocator:** Rather than relying on individual texture allocations, the engine sub allocates from a large continuous
virtual memory block using a linked list block allocator, with block splitting and merging to prevent fragmentation during runtime loading and unloading

### SIMD & Data Layout

The engine is written around a custom explicit SIMD wrapper supporting AVX2 (8 wide), ARM NEON (4 wide), and scalar fallbacks, and to feed these
registers efficiently all internal data structures use a SoA layout

- **8 way BVH:** To maximize SIMD utilization during raycasting and analytical shadow evaluation, an 8 way BVH is used where each node stores 8
child AABBs in parallel arrays, allowing the engine to test a ray or frustum against 8 bounding boxes simultaneously

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
