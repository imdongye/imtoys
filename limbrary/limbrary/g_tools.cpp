#include <limbrary/g_tools.h>
#include <limbrary/glm_tools.h>
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

float glim::intersectTriAndRay (
	const glm::vec3& O, const glm::vec3& D,
	const glm::vec3& A, const glm::vec3& B, const glm::vec3& C
)
{ 
	glm::vec3 E1, E2, N, AO, DAO;
	float det, invdet, u, v, t;
	E1 = B-A;
	E2 = C-A;
	N = glm::cross(E1,E2);
	det = -glm::dot(D, N);
	invdet = 1.f/det;
	AO  = O - A;
	DAO = glm::cross(AO, D);
	u =  glm::dot(E2,DAO) * invdet;
	v = -glm::dot(E1,DAO) * invdet;
	t =  glm::dot(AO,N)  * invdet; 
	if(det >= 1e-6 && t >= 0.0 && u >= 0.0 && v >= 0.0 && (u+v) <= 1.0)
		return t;
	return -1.f;
}