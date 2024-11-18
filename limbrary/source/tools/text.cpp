/*
Note:
	combine text and log for share text_buffer
*/
#include <limbrary/tools/text.h>
#include <limbrary/tools/log.h>
#include <limbrary/using_in_cpp/std.h>
#include <fstream>
#include <sstream>
#include <stb_sprintf.h>
#include <stdarg.h>


using namespace lim;


const char* lim::fmtStrToBuf(const char* format, ...)
{
	static char buf[FMT_STR_BUF_SIZE];
	va_list args;
	va_start(args, format);
	stbsp_vsprintf(buf, format, args);
	va_end(args);
	return buf;
}

string lim::readStrFromFile(const char* path)
{
	string text;
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

void lim::writeStrToFile(const char* path, const char* text)
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