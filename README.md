# MiniGL - Shadow Mapping

Exploring *Shadow Mapping* and it's various problems. for other examples search for "minigl" in my github repositories.

![demo](demos/minigl-hello-cube.png)

## TODO

- [x] Cube
- [ ] Plane
- [ ] Sphere
- [ ] Cylinder
- [ ] Blinn-Phong
- [ ] Camera Movement
- [ ] Shadow Mapping
- [ ] Bias
- [ ] Normal Bias
- [ ] PCF
- [ ] PCSS
- [ ] Jitter

Reference screenshots from Blender
|Name|Image|
|--|--|
Hard Shadow | ![Hard](demos/cube_hard_reference.png)
Soft Shadow | ![Soft](demos/cube_soft_reference.png)
Sphere PCSS | ![Sphere](demos/sphere_pcss_reference2.png)
Light Jitter (WTF) | ![Jitter](demos/all_jitter.png)

## Dependencies

- CMake
- GLFW
- glad
- imgui
- glm

## Build

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## Credits

- [PCSS paper by Nvidia](https://developer.download.nvidia.com/shaderlibrary/docs/shadow_PCSS.pdf)

By Gholamreza Dar 2025
