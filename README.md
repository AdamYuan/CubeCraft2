# CubeCraft2
Another Minecraft clone also the rebuild version of [CubeCraft](https://github.com/AdamYuan/CubeCraft)

## Installation
### Linux (Unix)
```bash
git clone https://github.com/AdamYuan/CubeCraft2.git
cd CubeCraft2
cmake . -DCMAKE_BUILD_TYPE=Release
make -j4
```

## Screenshots
![alt text](https://raw.githubusercontent.com/AdamYuan/CubeCraft2/master/screenshots/1.png)
![alt text](https://raw.githubusercontent.com/AdamYuan/CubeCraft2/master/screenshots/2.png)

## Built With
* [GLEW](http://glew.sourceforge.net/) - For modern OpenGL methods
* [GLFW](http://www.glfw.org/) - Window creation and management
* [GLM](https://glm.g-truc.net/) - Maths calculations
* [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h) - Image loading
* [FastNoiseSIMD](https://github.com/Auburns/FastNoiseSIMD) - Terrain generation
* [ImGui](https://github.com/ocornut/imgui) - UI rendering

## References
* https://www.seedofandromeda.com/blogs/29-fast-flood-fill-lighting-in-a-blocky-voxel-game-pt-1
* https://0fps.net/2013/07/03/ambient-occlusion-for-minecraft-like-worlds/
* http://www.minecrafttexturepacks.com/xenocontendi/ - Block texture
