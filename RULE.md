
2022-09-05 / im dong ye
# Rule And Note

## [ 이름 규칙 ]
* namespace : snake_case
* class/struct 이름 : PascalCase
* 함수/멤버 함수 : camelCase
* 멤버 상수 : UPPER_SNAKE_CASE
* 멤버 static변수, 전역변수 : snake_case
* 멤버 변수 : snake_case
* 지역 변수, 파라미터 : camelCase ( _ward )

* 함수의 선언부 매개변수에서 Class이름등으로 어떤객체인지 확실할때는 약어로 쓴다 ex) void init(Player p, int _age)

* 경로 뒤에 /를 붙이지 않는다.
* 포인터변수 선언에서 \*는 자료형 옆에 붙이고 한줄에서 여러 포인터 변수를 선언할때만 변수 앞에 붙인다. ex) int\* a; int  \*a1 \*a2;

### < 표준 위배 이유 >
https://en.cppreference.com/w/cpp/language/definition
m 또는 _ 를 멤버변수에 붙이지 않는 이유 : 개인적인 프로젝트이므로 간편하게 타이핑을 줄이기 위함.
멤버함수에 파스칼 안쓰는 이유 : 불필요한 shift 타이핑이라고 생각함.

## [ OpenGL / GLSL ]
glEnable/Disable은 사용후 초기값으로 복구 시켜두기

glsl 변수이름 camelCase

#### 접두어
* a : attribute
* m : model
* w : world

## [ 패키지 ]
```
#define STB_${pkg}_IMPLEMENTATION // 구현부는 stb.c에 있으므로 안해도됨.
// todo
#include <nanovg/nanovg.h>
#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg_gl.h>
```

## etc
나중에 cpp와 헤더를 다른 폴더로 분리할 수 있기 때문에 limbrary source에서 #include <>로 절대경로만 참조할것.