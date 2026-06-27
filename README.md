# MiniGL - Shadow Mapping

Exploring _Shadow Mapping_ and it's various problems. for other examples search for "minigl" in my github repositories.

![demo](demos/change_ambient.png)

## TODO

- [x] Cube
- [x] Plane
- [x] Sphere
- [x] Cylinder
- [x] Camera Movement
- [x] Blinn-Phong
- [x] Shadow Mapping
- [x] Bias
- [x] Normal Bias
- [x] PCF
- [ ] PCSS
- [ ] Jitter

## Progress

| Stage                          | Image                                           |
| ------------------------------ | ----------------------------------------------- |
| PCF Poisson high radius        | ![pcfpoisson](demos/pcfpoisson_high_radius.png) |
| PCF 5x5 high radius            | ![pcfhighradius](demos/pcf5x5_high_radius.png)  |
| PCF 5x5                        | ![pcf](demos/pcf5x5.png)                        |
| Bias 0.0003, Normal Bias 0.002 | ![bias11](demos/bias0.0003_nbias_0.002.png)     |
| Bias 0.0003, Normal Bias 0     | ![bias10](demos/bias0.0003_nbias_0.png)         |
| Bias 0, Normal Bias 0          | ![bias00](demos/bias0_nbias_0.png)              |
| Bias                           | ![bias](demos/shadow_bias.png)                  |
| First Shadow                   | ![first_shadow](demos/first_shadow.png)         |
| Blinn-Phong                    | ![blinn_phong](demos/change_ambient.png)        |
| Scene done                     | ![scene_done](demos/scene_done.png)             |

## References

Reference screenshots from Blender

| Name               | Image                                       |
| ------------------ | ------------------------------------------- |
| Hard Shadow        | ![Hard](demos/cube_hard_reference.png)      |
| Soft Shadow        | ![Soft](demos/cube_soft_reference.png)      |
| Sphere PCSS        | ![Sphere](demos/sphere_pcss_reference2.png) |
| Light Jitter (WTF) | ![Jitter](demos/all_jitter.png)             |

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

```bash
time cmake --build build -j && build/Debug/minigl_shadow_mapping.exe
```

## Credits

- [PCSS paper by Nvidia](https://developer.download.nvidia.com/shaderlibrary/docs/shadow_PCSS.pdf)
- [Babylon.js Shadow Mapping Deep Dive](https://doc.babylonjs.com/features/featuresDeepDive/lights/shadows/)
- [The Witness Shadow Mapping](http://the-witness.net/news/2013/09/shadow-mapping-summary-part-1/)
- [Cherno Shadow Mapping](https://www.youtube.com/watch?v=qbDrqARX07o0)

By Gholamreza Dar 2025
