#include <limbrary/utils.h>
#include <limbrary/logger.h>
#include <fstream>
#include <sstream>


namespace lim {
	std::string getStrFromFile(std::string_view path)
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
			Log::get()<<"[error] fail read : "<<path.data()<<", what? "<<e.what()<<Log::endl;
		}
		if( text.length()<1 ) {
			Log::get()<<"[error] length<1 : "<<path.data()<<Log::endl;
		}
		return text;
	}
	void setStrToFile(std::string_view path, std::string_view text)
	{
		std::ofstream ofile;
		try {
			ofile.open(path.data());
			ofile<<text;
			ofile.close();
		} catch( std::ifstream::failure& e ) {
			Log::get()<<"[error] fail read : "<<path.data()<<", what? "<<e.what();
		}
	}
	std::string fmToStr(const char* format, ...)
	{
		static char buffer[512]={0};
		va_list ap;
		va_start(ap, format);
		vsprintf(buffer, format, ap);
		va_end(ap);
		return std::string(buffer);
	}
}