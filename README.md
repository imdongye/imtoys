## Limbrary

'lim'brary is Extendable framework for model and texture viewer with imgui

and there are several toys that use limbrary

-   SDF modeler [report](https://drive.google.com/file/d/1fsZAitytSMygLGITExE0Y6ucwJFRNriO/view?usp=sharing) , [presentation](https://youtu.be/KKeihZ03pAs) , [demo](https://youtu.be/l02dHs1q9Jo)

-   model simplifier, normal map baker [report link](https://imdongye.notion.site/Simplification-d21e692652104cb39ce3befde034fcd2?pvs=4) , [demo](https://youtu.be/wZzI8Hjm5jQ)

-   PBR with IBL [demo](https://youtu.be/Yxrlhfb-fXo)

-   color aware image viewer

-   real time hatching tester

-   astar visualizer

-   [other demo videos](https://youtu.be/GvtG-AYt6d4)


## Note

The reason why inherit an application class and write a program is because when you run multiple programs on this one project To make explicite life cycle with constructor and deconstructor and use less data area by reducing global variables, and to manage memory leaks in the heap space by allocating and deleting memory of member variables.

## Build / Install Instructions

```
brew install cmake OR install @ https://cmake.org/download/

git clone https://github.com/imdongye/imtoys.git
cd imtoys
git submodule init
git submodule update

cmake -G["Xcode","Ninja","Visual Studio 17 2022"] -Bbuild .
cmake --build build --config Debug
```

## Run / Test

The program must run in the main folder.

```
cd main; ../build/Debug/executable/imtoys_exe; cd ..
```

F1 key to open app selector

## Dependency
in Core
* c++17
* [glad(0.1.36)](https://glad.dav1d.de) in 240927, gl4.1core by default, but use 4.6 core when using compute shader or ssbo. e.g. PBD
* [glfw(3.4)](https://github.com/glfw/glfw) in 240927
* [glm(1.0.1)](https://github.com/g-truc/glm) in 240927
* [stb](https://github.com/nothings/stb) in 240927
* [imgui(1.91.5) docking branch](https://github.com/ocornut/imgui) in 241109
* [imguizmo(1.83)](https://github.com/CedricGuillemet/ImGuizmo) in 240927
* [assimp(5.4.3)](https://github.com/assimp/assimp) in 240927
* [nlohmann/json(3.11.3)](https://github.com/nlohmann/json) in 240927

in Apps
* [eigen(3.4.0)](https://eigen.tuxfamily.org) in 240927
* [fqms](https://github.com/sp4cerat/Fast-Quadric-Mesh-Simplification)

