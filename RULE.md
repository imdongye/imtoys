
2022-09-05 / im dong ye
# Rule And Note

## [ 줄임말 ]

* md : model
* prog : program
* scn : scene
* mat : material
* tf : transform
* ms : mesh
* lit : light
* nd : Node
* src : source
* dst : destination
* mtx : matrix
* nr : number of
* vp : viewport

## [ 이름 규칙 ]

* namespace : snake_case
* class/struct 이름 : PascalCase
* 함수/멤버 함수 : camelCase
* 멤버 상수 : UPPER_SNAKE_CASE
* 멤버 static변수, 전역변수 : s_snake_case, g_snake_case
* 멤버 변수 : snake_case
* Uniform변수에도 사용되는 멤버변수는 glsl 규칙과 동일하게
* 지역 변수, 파라미터 : camelCase or _single
* 함수의 선언부 매개변수에서 Class이름등으로 어떤객체인지 확실할때는 약어로 쓴다 ex) void init(Player p, int _age)
* 경로 뒤에 /를 붙이지 않는다.
* 포인터변수 선언에서 \*는 자료형 옆에 붙이고 한줄에서 여러 포인터 변수를 선언할때만 변수 앞에 붙인다. ex) int\* a; int  \*a1 \*a2;
* 함수만 정의하는 cpp의 전역변수에서는 _snake_case대신 snake_case를 사용해도 된다.

### 표준 위배 이유

https://en.cppreference.com/w/cpp/language/definition

m 또는 _ 를 멤버변수에 붙이지 않는 이유 : 개인적인 프로젝트이므로 간편하게 타이핑을 줄이기 위함.

멤버함수에 파스칼 안쓰는 이유 : 불필요한 shift 타이핑이라고 생각함.

getter / setter 은 간단한 프로젝트에서 예방의 효과보다 생산성저하가 더 크게 느껴진다.

## [ OpenGL / GLSL ]

glEnable/Disable은 사용후 초기값으로 복구 시켜두기

attribute 변수 : aPos, aNor, ...
varying 변수 : mPos(model space), cPos(camera space), wPos(world space), lPos(light)
uniform 변수 : map_Bump, mtx_ShadowVp ( single_PascalCase )
uniform struct : mat.Shininess ( single.PascalCase )
전역 변수 : snake_case
전역 상수 : SNAKE_CASE


## [ 패키지 ]
```
#define STB_${pkg}_IMPLEMENTATION // 구현부는 stb.c에 있으므로 안해도됨.
// todo
#include <nanovg/nanovg.h>
#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg_gl.h>
```

## etc

타이핑을 줄이기 위해 구현이 있는 헤더파일에만 hpp를 붙인다.

수식을 작성할때 교환법칙이 성립하는 경우 변수명이 작은순으로 배치한다.

나중에 cpp와 헤더를 다른 폴더로 분리할 수 있기 때문에 limbrary source에서 #include <>로 절대경로만 참조할것.

OpenGL ID 가 들어가거나 삭제해야할 객체포인터를 맴버로 가지면은 참조복사생성자, 대입복사를 삭제해서 중복해서 삭제되는것을 막는다.

경로를 표기할때 폴더라도 뒤에 / 를 붙이지 않고 폴더 이름으로 끝낸다.


#  

move구현시 장 단점

이동생성자와 이동대입연산자(vector erase에 필요)를 구현하면 vector에서 객체를 관리할수있다.

장점 

1. delete 필요없음 
2. →문법 사용빈도 감소 
3. 메모리에서 연속성으로 캐쉬히트 증가
4. 소유권이 확실해짐

단점 

1. 다른 객체에서 참조할때 erase가 발생하면 주소가 바뀜.
2. 멤버 수정시 move에서도 고려해야함.
3. 이동생성자와 이동대입연산자에서 중복코드 또는 추가 함수필요.
4. 상속관계에서 move부분 코드 가독성 떨어짐
5. 암시적으로 컴파일러에서 처리하는 생성자 우선순위, 임시객체, (N)RVO, Copy Elision 신경써야함.

#


헤더에서 멤버변수 초기화화하면 가독성은 좋지만 멤버값 바꿀때마다, 포함하는 모든소스 다시 컴파일해서 안좋긴한데 그렇다고 멤버이니셜라이져나 생성자에서 초기화하기엔 멤버수정될때마다 생성자 관리해줘야하고 읽을때 파일 두번 열어야하고 중복코드가 많이생김.

own_ prefix가 있는 포인터는 소유권있어서 소멸자에서 삭제 필요. 없으면 소유권없음



작성방법 : https://learn.microsoft.com/en-us/cpp/cpp/move-constructors-and-move-assignment-operators-cpp?view=msvc-170&redirectedfrom=MSDN

https://en.wikipedia.org/wiki/Fail-fast