## Limbrary

'lim'brary is Extendable framework for model and texture viewer with imgui

and there are several toys that use limbrary

-   SDF modeler [report](https://drive.google.com/file/d/1fsZAitytSMygLGITExE0Y6ucwJFRNriO/view?usp=sharing) [presentation](https://youtu.be/KKeihZ03pAs) [demo](https://youtu.be/l02dHs1q9Jo)

-   model simplifier, normal map baker [report link](https://imdongye.notion.site/Simplification-d21e692652104cb39ce3befde034fcd2?pvs=4) [demo](https://youtu.be/wZzI8Hjm5jQ)

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

c++17, glfw, glad, [stb(image, write, truetype)](https://github.com/nothings/stb), [imgui](https://github.com/ocornut/imgui), [assimp](https://github.com/assimp/assimp), [nlohmann/json](https://github.com/nlohmann/json), [fqms](https://github.com/sp4cerat/Fast-Quadric-Mesh-Simplification)

Use gl4.1core by default, but use 4.6 core when using compute shader or ssbo. (PBD, canvas3d)
