# MiniGL - Shadow Mapping

Exploring _Shadow Mapping_ and it's various problems. for other examples search for "minigl" in my github repositories.

![demo](demos/change_ambient.png)

## TODO

- [x] Cube
- [x] Plane
- [x] Sphere
- [x] Cylinder
- [x] Camera Movement
- [ ] Blinn-Phong
- [ ] Shadow Mapping
- [ ] Bias
- [ ] Normal Bias
- [ ] PCF
- [ ] PCSS
- [ ] Jitter

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

## Progress

| Stage       | Image                                    |
| ----------- | ---------------------------------------- |
| Blinn-Phong | ![blinn_phong](demos/change_ambient.png) |
| Scene done  | ![scene_done](demos/scene_done.png)      |

## References

Reference screenshots from Blender

| Name               | Image                                       |
| ------------------ | ------------------------------------------- |
| Hard Shadow        | ![Hard](demos/cube_hard_reference.png)      |
| Soft Shadow        | ![Soft](demos/cube_soft_reference.png)      |
| Sphere PCSS        | ![Sphere](demos/sphere_pcss_reference2.png) |
| Light Jitter (WTF) | ![Jitter](demos/all_jitter.png)             |

## Credits

- [PCSS paper by Nvidia](https://developer.download.nvidia.com/shaderlibrary/docs/shadow_PCSS.pdf)

By Gholamreza Dar 2025
