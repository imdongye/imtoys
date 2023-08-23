#include <limbrary/program.h>
#include <limbrary/logger.h>
#include <limbrary/utils.h>

namespace lim
{
	
	Program::Program(const char* name, const char* homeDir)
	: _name(name), home_dir(homeDir) {}
	Program::~Program() { clear(); }
	
	Program& Program::clear()
	{
		if( pid ) glDeleteProgram(pid);
		if( vert_id ) glDeleteShader(vert_id);
		if( frag_id ) glDeleteShader(frag_id);
		if( geom_id ) glDeleteShader(geom_id);
		if( comp_id ) glDeleteShader(comp_id);
		pid = vert_id = frag_id = geom_id = comp_id = 0;
		return *this;
	}
	Program& Program::operator+=(const char* path)
	{
		return attatch(path);
	}
	Program& Program::setHomeDir(std::string_view dir)
	{
		home_dir = dir;
		return *this;
	}
	Program& Program::attatch(std::string path)
	{
		if( pid==0 ) pid = glCreateProgram();

		auto [sid, type] = createShaderAuto(path);

		if( sid==0 ) {
			Logger::get()<<"[error] "<<type<<" extension is not supported.";
			return *this;
		}
		// AUTO PATHING : 파일이름만 들어오면 homedir경로로
		if( path.find('/')==std::string::npos && path.find('\\')==std::string::npos ) {
			// todo: 임시 객체 줄이기 최적화
			path = home_dir+"shader/"+path;
		}

		// load text
		std::string scode = getStrFromFile(path);

		// compile
		const GLchar* ccode = scode.c_str();
		glShaderSource(sid, 1, &ccode, nullptr);
		glCompileShader(sid);
		if( !checkCompileErrors(sid, path) ) {
			sid = 0;
			return *this;
		}
		glAttachShader(pid, sid);
		Logger::get().log("[program %s] attch %s success\n", _name.c_str(), path.c_str());

		return *this;
	}
	Program& Program::link()
	{
		glLinkProgram(pid);
		glUseProgram (pid);
		if( !checkLinkingErrors(pid) ) {
			pid = 0;
			return *this;
		}
		Logger::get().log("[program %s] linking success\n\n", _name.c_str());
		return *this;
	}
	GLuint Program::use() const
	{
		if( pid==0 ) {
			Logger::get()<<"[error] program is not linked"<<Logger::endll;
		}
		glUseProgram(pid);
		return pid;
	}

	bool Program::checkCompileErrors(GLuint shader, std::string_view path)
	{
		GLint success;
		GLchar infoLog[1024];

		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if( !success ) {
			glGetShaderInfoLog(shader, 1024, NULL, infoLog);
			Logger::get()<<"[error] shader compile error in "<< path.data()<<"\n"<<infoLog << Logger::endl;
			//std::abort();
			return false;
		}
		return true;
	}
	bool Program::checkLinkingErrors(GLuint shader)
	{
		GLint success = 0;
		GLchar infoLog[1024];

		glGetProgramiv(shader, GL_LINK_STATUS, &success);
		if( !success ) {
			glGetProgramInfoLog(shader, 1024, NULL, infoLog);
			Logger::get()<<"[error] shader linking error\n"<<infoLog<<Logger::endl;
			std::abort();
			return false;
		}
		return true;
	}
	// string_view는 char* char[]을 받아도 string으로 임시객체를 만들지 않고 포인터를 사용함
	std::tuple<int, const char*> Program::createShaderAuto(const std::string_view filename)
	{
		size_t index = filename.rfind(".");
		std::string_view ext = filename.substr(index + 1);
		if( ext=="vert"||ext=="vs" ) {
			vert_id = glCreateShader(GL_VERTEX_SHADER);
			return std::make_tuple(vert_id, "vertex");
		}
		else if( ext=="frag"||ext=="fs" ) {
			frag_id = glCreateShader(GL_FRAGMENT_SHADER);
			return std::make_tuple(frag_id, "fragment");
		}
		else if( ext=="geom"||ext=="gs" ) {
			geom_id = glCreateShader(GL_GEOMETRY_SHADER);
			return std::make_tuple(geom_id, "geometry");
		}
		else if( ext=="comp"||ext=="cs" ) {
#ifndef __APPLE__
			comp_id = glCreateShader(GL_COMPUTE_SHADER);
#endif
			return std::make_tuple(comp_id, "compute");
		}

		return std::make_tuple(0, "none");
	}
	// From: https://www.youtube.com/watch?v=nBB0LGSIm5Q
	GLint Program::getUniformLocation(const std::string_view name) const
	{
		std::string sname(name);
		if( uniform_location_cache.find(sname) != uniform_location_cache.end() ) {
			return uniform_location_cache[sname];
		}
		GLint loc = glGetUniformLocation(pid, sname.c_str());
		uniform_location_cache[sname] = loc;
		return loc;
	}
}