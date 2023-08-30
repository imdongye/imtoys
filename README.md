## Limbrary

'lim'brary is Extendable framework for model and texture viewer with imgui

and there are several toys that use limbrary

-   model simplifier, normal map baker

-   color aware image viewer

-   PBR tester

-   real time hatching tester

-   astar visualizer

## Note

application 클래스를 상속해서 프로그램을 작성하는 이유는 여러 프로그램을 하나의 프로젝트에서 실행할때
class의 생성자 소멸자에 의한 직관적인 lifecycle과 전역변수를 줄여 Data영역을 적게 사용하고, 멤버변수의 메모리 할당과 삭제를 통해 heap공간에서 메모리 누수를 관리 하기 위함.

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
<settings.json>
"cmake.debugConfig": {
    "cwd": "${workspaceFolder}/main"
},
```

## Dependency

c++17, glfw, glad, stb(image, write, truetype), assimp, imgui, nlohmann/json, fqms, nanovg, freetype

https://github.com/nothings/stb

https://github.com/ocornut/imgui

https://github.com/nlohmann/json

https://github.com/assimp/assimp

https://github.com/sp4cerat/Fast-Quadric-Mesh-Simplification

https://github.com/memononen/nanovg
