#include <limbrary/utils.h>
#include <limbrary/log.h>
#include <fstream>
#include <sstream>
#include <stb_sprintf.h>
#include <stdarg.h>


namespace lim {
	std::string readStrFromFile(std::string_view path)
	{
		std::string text;
		std::ifstream ifile;
		std::stringstream ss;
		//ifile.exceptions(std::ifstream::failbit|std::ifstream::badbit);
		try {
			ifile.open(path.data());
			ss<<ifile.rdbuf(); // stream buffer
			ifile.close();
			text = ss.str();
		} catch( std::ifstream::failure& e ) {
			log::err("fail read : %s, what? %s\n", path.data(), e.what());
		}
		if( text.length()<1 ) {
			log::err("length<1 : %s\n",path.data());
		}
		return text;
	}
	void writeStrToFile(std::string_view path, std::string_view text)
	{
		std::ofstream ofile;
		try {
			ofile.open(path.data());
			ofile<<text;
			ofile.close();
		} catch( std::ifstream::failure& e ) {
			log::err("fail read : %s, what? %s\n", path, e.what());
		}
	}

	char* fmtStrToBuf(const char* format, ...)
	{
		static char buf[SPRINTF_BUF_SIZE];
		va_list args;
        va_start(args, format);
        stbsp_vsprintf(buf, format, args);
        va_end(args);

		return buf;
	}
}