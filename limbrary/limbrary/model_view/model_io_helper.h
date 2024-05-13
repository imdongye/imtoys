/*

rapping and capsulation assimp data

*/


#ifndef __model_io_helper_h_
#define __model_io_helper_h_

#include <string>
#include <assimp/cexport.h>

namespace lim
{
    // get format data
	int getNrImportFormats();
	const char* getImportFormat(int idx);
	std::string findModelInDirectory(std::string_view path);
	int getNrExportFormats();
	const aiExportFormatDesc* getExportFormatInfo(int idx);
}
#endif