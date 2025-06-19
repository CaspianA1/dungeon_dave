# Dungeon Dave!

---

- A first-person FPS in the style of games like Doom and Quake.
- The Dungeon Dave engine is built fully from scratch, ensuring optimal performance for all platforms.

---

### Rendering Techniques

- **Normal Mapping**
- **Parallax Occlusion Mapping** (heightmaps are generated at runtime based on objects' albedo textures - currently disabled though)

- **Exponential Shadow Mapping** (for soft shadows)
- **Cascaded Shadow Mapping** (with blended depth layers - this makes transitions between depth layers a smooth fade)
- **God Rays** (based on [this](https://developer.nvidia.com/gpugems/gpugems3/part-ii-light-and-shadows/chapter-13-volumetric-light-scattering-post-process) technique from Nvidia, but works in light-space rather than in screen-space, which allows for the light volumes to remain when the camera is turned away from the sun)
- **Precomputed Raytraced Ambient Occlusion** (a shader traces rays from each point in the scene to compute a set of occlusion values - and caches this on disk - and at runtime, the main shader tricubically interpolates from a 3D texture to fetch these occlusion values)

### More To Know
- The renderer is also fully physically based, and employs a metallic/roughness material system.
- Shadows and god rays work perfectly for translucent objects!
- Billboard sprites are fully animated.
- Levels use a set of original soundtracks developed by Adam Winograd. In-game sound effects use OpenAL for a surround-sound effect.
- I drew most of the pixel art with Aseprite, including the skyboxes (which employ cylindrical projection).

---

### Screenshots

![Screenshot 2023-01-07 at 14 40 57](https://user-images.githubusercontent.com/41955769/211393863-fac34033-8377-4559-989e-6f2f726d44de.png)

![Screenshot 2022-12-14 at 01 05 02](https://user-images.githubusercontent.com/41955769/211393898-6750e749-dbda-4547-b651-a633f4665d5c.png)

![Screenshot 2022-12-14 at 00 46 17](https://user-images.githubusercontent.com/41955769/211393912-fadcdc3e-531c-4dba-adee-b0cea7dabb25.png)

---

### Dependencies

- To install any dependencies, run `install.sh`. Note that only MacOS and Fedora Linux are officially supported.

### Building

- Simply run `build.sh`, passing the build type as the first argument (`debug` or `release`).
- If you wish to run the project as well, you can specify the second argument to be `run`.

### Movement Keybindings

- Head movement: mouse/trackpad
- Use weapon: click mouse/trackpad
- Forward/backward: w/s
- Strafe: a/d
- Sprint for forward/backward movement: left/right shift
- Jump: spacebar

### Debug Keybindings

- Fly: 1
- Toggle wireframe mode: 2
- Print position: 3
- Print direction: 4
- Print SDL error: 5
- Print OpenGL error: 6
- Print OpenAL error: 7
- Print OpenAL context error: 8

---