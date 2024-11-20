//
//	2022-08-07 / im dong ye
//
//	Todo:
//  1. fix with half edge data structure
//
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

#ifndef __simplify_h_
#define __simplify_h_


#include <limbrary/3d/model.h>

namespace lim
{
	void simplifyMesh(lim::Mesh& mesh, float lived_pct, int version = 0, int agressiveness=7, bool verbose=true);

	// agressiveness : sharpness to increase the threshold.
	//                 5..8 are good numbers
	//                 more iterations yield higher quality
	// version 0 : original (use agressiveness)
	//		   1 : lossless
	//		   2 : max_consider_thresold
	//
	void simplifyModel(lim::ModelData& model, float lived_pct = 0.8f, int version = 0, int agressiveness=7, bool verbose=true);

	void bakeNormalMap(const ModelData& src, ModelData& dst, int texSize);

	void convertBumpMapToNormalMap(ModelData& md);
}

#endif