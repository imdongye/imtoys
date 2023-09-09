//
//  2022-09-27 / im dong ye
//  edit learnopengl code
//
//	Assimp는 namespace도 없고 자료형의 이름이 camelCase이므로
//	Assimp에 한해서 지역변수 이름을 snake_case로 작성한다.
//
//  Todo:
//  1. model path texture path 그냥 하나로 합치기
//

#ifndef __model_importer_h_
#define __model_importer_h_

#include "model.h"

namespace lim
{
	Model* importModelFromFile(std::string_view path, bool makeNormalized = false);
}

#endif
