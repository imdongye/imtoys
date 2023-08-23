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
```

## Run / Test

```
# Terminal
cd main; ../buildx/Debug/executable/imtoys_exe; cd ..
# Xcode
Product -> Scheme -> Edit Scheme -> Use custom working directory
# Visual Studio
프로젝트 속성 -> 디버깅 -> 작업 디렉터리
# VSCode
cmake와 c/c++ 확장설치 -> 왼쪽 실행및 디버그탭 -> 구성 선택 -> f5
ctrl(cmd)+, -> cmake build directory : ${workspaceFolder}/build/${buildType}
```

## Dependency

c++17, glfw, glad, stb(image, write, truetype), assimp, imgui, nlohmann/json, fqms, nanovg, freetype

https://github.com/nothings/stb

https://github.com/ocornut/imgui

https://github.com/nlohmann/json

https://github.com/assimp/assimp

https://github.com/sp4cerat/Fast-Quadric-Mesh-Simplification

https://github.com/memononen/nanovg
