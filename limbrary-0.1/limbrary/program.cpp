#include <limbrary/program.h>
#include <limbrary/log.h>
#include <limbrary/utils.h>

namespace lim
{
	Program::Program(std::string_view _name)
		: name(_name)
	{
	}
	Program::Program(Program&& src) noexcept
	{
		*this=std::move(src);
	}
	Program& Program::operator=(Program&& src) noexcept
	{
		if(this == &src)
			return *this;
		Program::~Program();

		name = std::move(src.name);
		home_dir = std::move(src.home_dir);
		use_hook = std::move(src.use_hook);
		// for( const auto [k, v] : src.uniform_location_cache )
		// 	uniform_location_cache.insert(std::make_pair(k,v));

		pid = src.pid;
		vert_id = src.vert_id;
		frag_id = src.frag_id;
		geom_id = src.geom_id;
		comp_id = src.comp_id;
		src.pid = src.vert_id =src.frag_id = src.geom_id = src.comp_id = 0;
		return *this;
	}
	Program::~Program() noexcept
	{
		deinitGL(); 
	}
	Program& Program::deinitGL()
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
	Program& Program::attatch(std::string path)
	{
		if( pid==0 ) pid = glCreateProgram();

		auto [sid, type] = createShaderAuto(path);

		if( sid==0 ) {
			log::err("%s extension is not supported.\n", type);
			return *this;
		}
		// AUTO PATHING : 파일이름만 들어오면 homedir경로로
		if( path.find('/')==std::string::npos && path.find('\\')==std::string::npos ) {
			// todo: 임시 객체 줄이기 최적화
			path = home_dir+"/shaders/"+path;
		}

		// load text
		std::string scode = readStrFromFile(path);

		// compile
		const GLchar* ccode = scode.c_str();
		glShaderSource(sid, 1, &ccode, nullptr);
		glCompileShader(sid);
		if( !checkCompileErrors(sid, path) ) {
			sid = 0;
			return *this;
		}
		glAttachShader(pid, sid);
		log::pure("[program %s] attch %s success\n", name.c_str(), path.c_str());

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
		log::pure("[program %s] linking success\n\n", name.c_str());
		return *this;
	}
	bool Program::checkCompileErrors(GLuint shader, std::string_view path)
	{
		GLint success;
		GLchar infoLog[1024];

		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if( !success ) {
			glGetShaderInfoLog(shader, 1024, NULL, infoLog);
			log::err("shader compile error in %s\n%s\n",path.data(), infoLog);
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
			log::err("shader linking error\n%s\n",infoLog);

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


	const Program& Program::use() const
	{
		if( pid==0 ) {
			log::err("program is not linked\n");
		}
		glUseProgram(pid);
		use_hook(*this);
		return *this;
	}

	// From: https://www.youtube.com/watch?v=nBB0LGSIm5Q
	GLint Program::getUniformLocation(const std::string& vname) const
	{
		// if( uniform_location_cache.find(vname) != uniform_location_cache.end() ) {
		// 	return uniform_location_cache[vname];
		// }
		GLint loc = glGetUniformLocation(pid, vname.c_str());
		//uniform_location_cache[vname] = loc;
		//if(loc<0) log::err("missing...");
		return loc;
	}
}