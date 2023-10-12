
2022-09-05 / im dong ye
# Rule And Note

## [ 이름 규칙 ]

* namespace : snake_case
* class/struct 이름 : PascalCase
* 함수/멤버 함수 : camelCase
* 멤버 상수 : UPPER_SNAKE_CASE
* 멤버 static변수, 전역변수 : _snake_case
* 멤버 변수 : snake_case
* 지역 변수, 파라미터 : camelCase ( _ward )
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

타이핑을 줄이기 위해 구현이 있는 헤더파일에만 hpp를 붙인다.

수식을 작성할때 교환법칙이 성립하는 경우 변수명이 작은순으로 배치한다.

나중에 cpp와 헤더를 다른 폴더로 분리할 수 있기 때문에 limbrary source에서 #include <>로 절대경로만 참조할것.

OpenGL ID 가 들어가거나 삭제해야할 객체포인터를 맴버로 가지면은 참조복사생성자, 대입복사를 삭제해서 중복해서 삭제되는것을 막는다.

경로를 표기할때 폴더라도 뒤에 / 를 붙이지 않고 폴더 이름으로 끝낸다.

전 버전에는 객체들을 동적할당해서 포인터를 가지고 사용했는데 이때 생성순서를 직접 컨트롤할수있고 멤버 객체도 쉽게 포인터만 바꾸어 교체할수있고 필요할때만 생성할수도있었다.

그럼에도 전부 이동생성자를 만들고 기존 앱들을 갈아 엎은 이유는 메모리 누수 측면에서 살짝 신경쓸게 적어진다. 앱소멸자에서 멤버객체를 삭제하지 않아도됨. 그리고 귀찮은 -> 문법에서 벗어날수있다. 하지만 암시적으로 컴파일러에서 처리하는 생성자 우선순위, 임시객체, (N)RVO, Copy Elision 등을 신경써야한다. 그리고 이동생성자 뿐만아니라 이동대입연산자도 구현해줘야 vector erase에서 동작한다.

    Copy Elision 의 예, 클래스 선언부에서 멤버 객체 선언과 동시에 초기화( Cat cat = Cat("오돌이"); ) 에서 Cat생성자가 한번만 호출된다.

이 작업을 통해서 느낀건데 가독성만 떨어지고 신경쓸게 더 많아지는 것 같다. 하지만 이번에 생성자에 파라미터들을 없애면서 생성자에 파라미터로 멤버들을 초기화하는게 코드 가독성에 안좋았다는걸 느껴서 앞으로 생성자는 필요하다면 이름정도만 받아서 만드려고 한다. 꼭 필요한곳 아니면 생성자와 소멸자만 남기고 복사, 대입, 이동 생성자들을 delete하고 포인터로 사용하는게 좋겠다.

또한 dinamic array 에 객체들을 생성하면 각 객체의 주소가 바뀔수있기때문에 조심해야한다.

작성방법 : https://learn.microsoft.com/en-us/cpp/cpp/move-constructors-and-move-assignment-operators-cpp?view=msvc-170&redirectedfrom=MSDN

