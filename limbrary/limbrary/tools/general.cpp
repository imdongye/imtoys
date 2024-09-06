#include <limbrary/tools/general.h>
#include <limbrary/tools/glim.h>
#include <limbrary/tools/log.h>
#include <fstream>
#include <sstream>
#include <stb_sprintf.h>
#include <stdarg.h>


namespace lim {
	std::string readStrFromFile(const char* path)
	{
		std::string text;
		std::ifstream ifile;
		std::stringstream ss;
		//ifile.exceptions(std::ifstream::failbit|std::ifstream::badbit);
		try {
			ifile.open(path);
			ss<<ifile.rdbuf(); // stream buffer
			ifile.close();
			text = ss.str();
		} catch( std::ifstream::failure& e ) {
			log::err("fail read : %s, what? %s\n", path, e.what());
		}
		if( text.empty() ) {
			log::warn("empty : %s\n",path);
		}
		return text;
	}
	void writeStrToFile(const char* path, const char* text)
	{
		std::ofstream ofile;
		try {
			ofile.open(path);
			ofile<<text;
			ofile.close();
		} catch( std::ifstream::failure& e ) {
			log::err("fail read : %s, what? %s\n", path, e.what());
		}
	}

	const char* fmtStrToBuf(const char* format, ...)
	{
		static char buf[SPRINTF_BUF_SIZE];
		va_list args;
        va_start(args, format);
        stbsp_vsprintf(buf, format, args);
        va_end(args);

		return buf;
	}
}