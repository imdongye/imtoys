//
//  2022-10-16 / im dong ye
// 
//	edit from : https://github.com/assimp/assimp/issues/203
//
//	target model의 normal을 저장하고 원본 모델의 world space normal을 계산한뒤
//	target normal으로 tbn을 계산해서 tangent space로 바꾼다.
// 
//	주의 bump_map이 아닌 texture은 원본과 공유한다.
//
//	Todo:
//	노멀맵이 없는경우 diffuse맵등으로 구분하여 새로운 노멀맵에 그려주고
//	assimp scene의 texture을 조작해서 연결한뒤 export해야함.


#ifndef __map_backer_h_
#define __map_backer_h_

#include <limbrary/model_view/model.h>

namespace lim
{
	void bakeNormalMap(std::string_view exportPath, Model* original, Model* target);
}

#endif