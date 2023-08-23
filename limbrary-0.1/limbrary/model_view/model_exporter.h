//
//  2022-09-27 / im dong ye
//
//	edit from : https://github.com/assimp/assimp/issues/203
//

#ifndef __model_exporter_h_
#define __model_exporter_h_

#include "model.h"
#include <assimp/cexport.h>

namespace lim
{
	int getNrExportFormats();
	const aiExportFormatDesc *getExportFormatInfo(int idx);
	void exportModel(std::string_view exportDir, Model *model, size_t pIndex);
}
#endif