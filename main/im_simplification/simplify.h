//
//	2022-08-07 / im dong ye
//
//	TODO list:
//  1. fix with half edge data structure
//
//

#ifndef __simplify_h_
#define __simplify_h_


#include <limbrary/model_view/model.h>

namespace fqms
{
	void simplifyMesh(const lim::Mesh* mesh, float lived_pct, int version = 0, int agressiveness=7, bool verbose=true);

	// agressiveness : sharpness to increase the threshold.
	//                 5..8 are good numbers
	//                 more iterations yield higher quality
	// version 0 : original (use agressiveness)
	//		   1 : lossless
	//		   2 : max_consider_thresold
	//
	void simplifyModel(const lim::Model* model, float lived_pct = 0.8f, int version = 0, int agressiveness=7, bool verbose=true);
}

#endif