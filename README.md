## limbrary

'lim'brary is Extendable framework for model and texture viewer with imgui

and there are several toys that use limbrary

-   model simplifier, normal map baker

-   color aware image viewer

-   PBR tester

-   real time hatching tester

-   astar visualizer

## Build / Install Instructions

```
brew install cmake OR install @ https://cmake.org/download/

git clone https://github.com/imdongye/cmake-opengl-template.git
git submodule init
git submodule update

cmake -G["Xcode","Ninja","Visual Studio 17 2022"] -Bbuild .
cmake --build build --config Debug

cd main; ../buildx/Debug/executable/imtoys_exe; cd ..

Product -> Scheme -> Edit Scheme -> Use custom working directory

```

## Dependency

c++17, glfw, glad, stb(image, write, truetype), assimp, imgui, nlohmann/json, fqms, nanovg, freetype

https://github.com/nothings/stb

https://github.com/ocornut/imgui

https://github.com/nlohmann/json

https://github.com/assimp/assimp

https://github.com/sp4cerat/Fast-Quadric-Mesh-Simplification

https://github.com/memononen/nanovg
