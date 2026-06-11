# Bloczki-Gra-PWaG

A voxel renderer written in DirectX 11 and C++, developed as part of PwAG (Programowanie w API Graficznych — Graphics API Programming) at Silesian University of Technology.


## Showcase

https://github.com/user-attachments/assets/e1de29d3-730b-48e6-8c96-769cd4caaa6e

**[Download latest release](../../releases/latest)**

## Features
### Rendering
* Deferred rendering
    + Implemented to support numerous point lights
* PBR shading
* Lighting
    + Directional (Sun), Voxel-based, Point, 
* Parallax Occlusion Mapping
* Shadow mapping
* Reinhard luminance tonemapping
* Bloom

### Performance
* Chunking system
    + Divides the world into chunks, composed of 16x16x16 blocks each (chunk size can be changed in chunk.h)
    + Meshed geometry to reduce drawcalls
    + Chunk data stored in a 1D array for cache-friendly iteration during meshing
* Frustum culling
* Multithreading
    + Chunk mesh generation runs on dedicated worker threads via a producer-consumer job queue
    + The main thread queues dirty chunks for remeshing and uploads completed meshes to the GPU each update tick, keeping mesh generation off the main thread entirely

### Gameplay/Engine
* Basic collisions of the player with the world
* DDA-based raycasting for block interactions (highlighting, breaking, placement)

### Assets
* Hand-painted textures
* A mixture of generated + hand-painted PBR maps

## Building from Source
**Requirements:**
* Visual Studio 2022 or newer
* "Desktop development with C++" workload
* Windows SDK (any recent version)

**Steps:**
1. Open `Bloczki-Gra-PWaG.sln`
2. Select the `Release | x64` configuration
3. Build the solution

Shaders and textures are copied to the output directory automatically as a post-build step.

## Controls

WSAD - standard movement  
Space/Shift - Up/Down  
Left/Right click - Break/Place blocks  
Mouse wheel - change block type  
Arrow keys left/right - change in-game time of the day

#### Debug Controls
F1-F12 - change render mode

F1 - Default  
F2 - Albedo  
F3 - Normals  
F4 - Occlusion  
F5 - Roughness  
F6 - Metallic  
F7 - Skylight level  
F8 - Blocklight level  
F9 - Emissive  
F10 - Depth  
F11 - Bloom  
F12 - Wireframe  
