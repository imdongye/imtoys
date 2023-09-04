#include <limbrary/utils.h>
#include <limbrary/log.h>
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
			log::err("fail read : %s, what? %s\n", path.data(), e.what());
		}
		if( text.length()<1 ) {
			log::err("length<1 : %s\n",path.data());
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
			log::err("fail read : %s, what? %s", path, e.what());
		}
	}
}