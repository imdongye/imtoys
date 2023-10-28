## Limbrary

'lim'brary is Extendable framework for model and texture viewer with imgui

and there are several toys that use limbrary

-   model simplifier, normal map baker [report link](https://imdongye.notion.site/Simplification-d21e692652104cb39ce3befde034fcd2?pvs=4)

-   color aware image viewer

-   PBR tester

-   real time hatching tester

-   astar visualizer

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

If you want to use the pre-maked imgui window settings, Copy main/exports/imgui.ini into the main folder.

```
cd main; ../build/Debug/executable/imtoys_exe; cd ..
```

## Dependency

c++17, glfw, glad, stb(image, write, truetype), assimp, imgui, nlohmann/json, fqms, nanovg, freetype

https://github.com/nothings/stb

https://github.com/ocornut/imgui

https://github.com/nlohmann/json

https://github.com/assimp/assimp

https://github.com/sp4cerat/Fast-Quadric-Mesh-Simplification

https://github.com/memononen/nanovg
