#include <limbrary/program.h>
#include <limbrary/log.h>
#include <limbrary/utils.h>

namespace
{
	bool checkCompileErrors(GLuint shader, std::string_view path)
	{
		GLint success;
		GLchar infoLog[1024];

		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if( !success ) {
			glGetShaderInfoLog(shader, 1024, NULL, infoLog);
			lim::log::err("shader compile error in %s\n%s\n",path.data(), infoLog);
			//std::abort();
			return false;
		}
		return true;
	}
	bool checkLinkingErrors(GLuint pid)
	{
		GLint success = 0;
		GLchar infoLog[1024];

		glGetProgramiv(pid, GL_LINK_STATUS, &success);
		if( !success ) {
			glGetProgramInfoLog(pid, 1024, NULL, infoLog);
			lim::log::err("program linking error\n%s\n",infoLog);

			std::abort();
			return false;
		}
		return true;
	}
	GLenum getType(std::string_view path)
	{
		size_t idx = path.rfind(".");
		std::string_view ext = path.substr(idx+1);
		if( ext=="vert"||ext=="vs" )
			return GL_VERTEX_SHADER;
		else if( ext=="frag"||ext=="fs" )
			return GL_FRAGMENT_SHADER;
		else if( ext=="geom"||ext=="gs" )
			return GL_GEOMETRY_SHADER;
#ifndef __APPLE__
		else if( ext=="comp"||ext=="cs" )
			return GL_COMPUTE_SHADER;
#endif
		lim::log::err("%s extension is not supported.\n", ext.data());
		return 0;
	}
}

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

		deinitGL();

		name = std::move(src.name);
		home_dir = std::move(src.home_dir);
		use_hook = std::move(src.use_hook);

		pid = src.pid;
		src.pid = 0;// 바뀌는지 todo
		shaders = std::move(src.shaders);
		uniform_location_cache = std::move(src.uniform_location_cache);
		
		return *this;
	}
	Program::~Program() noexcept
	{
		deinitGL(); 
	}
	Program& Program::deinitGL()
	{
		for(auto& shader : shaders) {
			glDetachShader(pid, shader.sid);
			shader.deinitGL();
		}
		if( pid ) { glDeleteProgram(pid); pid = 0; }
		return *this;
	}


	void Program::Shader::createAndCompile()
	{
		if(sid) { glDeleteShader(sid); }
		sid = glCreateShader(type);
		std::string strSrc = readStrFromFile(path);
		const GLchar* src = strSrc.c_str();
		glShaderSource(sid, 1, &src, nullptr);
		glCompileShader(sid);
		if( !checkCompileErrors(sid, path) ) {
			deinitGL();
			return;
		}
		log::pure("%s compiled\n", path.c_str());
	}
	void Program::Shader::deinitGL()
	{
		if(sid) { glDeleteShader(sid); sid = 0; }
	}


	Program& Program::operator+=(const char* path)
	{
		return attatch(path);
	}
	Program& Program::attatch(std::string path)
	{
		// 파일이름만 들어왔을때 home_dir 상대경로로 설정
		if( path.find('/')==std::string::npos && path.find('\\')==std::string::npos ) {
			path = home_dir+"/shaders/"+path;
		}

		GLenum type = getType(path);
		if(type==0)
			return*this;

		bool isExist = false;
		for(auto& shader : shaders) {
			if(shader.type==type) {
				shader.path = path;
				shader.createAndCompile();
				isExist = true;
			}
		}
		if(!isExist) {
			shaders.push_back({0, type, path});
			shaders.back().createAndCompile();
		}

		return *this;
	}
	Program& Program::link()
	{
		if( pid ) { glDeleteProgram(pid); pid = 0; }
		pid = glCreateProgram();

		for(auto& shader : shaders) {
			glAttachShader(pid, shader.sid);
		}
		glLinkProgram(pid);
		if( !reloadable ) {
			for(auto& shader : shaders) {
				glDetachShader(pid, shader.sid);
				shader.deinitGL();
			}
			shaders.clear();
			shaders.shrink_to_fit();
		}
		if( !checkLinkingErrors(pid) ) {
			if( pid ) { glDeleteProgram(pid); pid = 0; }
			return *this;
		}
		log::pure("%s : linked\n\n", name.c_str());
		return *this;
	}
	const Program& Program::use() const
	{
		if( pid==0 ) { // todo
			log::err("program is not linked\n");
		}
		glUseProgram(pid);
		use_hook(*this);
		return *this;
	}
	// From: https://www.youtube.com/watch?v=nBB0LGSIm5Q
	GLint Program::getUniformLocation(const std::string& vname) const
	{
		if( uniform_location_cache.find(vname) != uniform_location_cache.end() ) {
			return uniform_location_cache[vname];
		}
		GLint loc = glGetUniformLocation(pid, vname.c_str());
		uniform_location_cache[vname] = loc;
		//if(loc<0) log::err("missing...");
		return loc;
	}

	ProgramReloadable::ProgramReloadable(std::string_view _name)
		: Program(_name)
	{
		reloadable = true;
	}
	ProgramReloadable::ProgramReloadable(ProgramReloadable&& src)
		: Program(std::move(src))
	{
		reloadable = true;
	}
	void ProgramReloadable::reload(GLenum type)
	{
		uniform_location_cache.clear();

		for(auto& shader : shaders) {
			if( shader.type == type ) {
				glDetachShader(pid, shader.sid);
				shader.createAndCompile();
				glAttachShader(pid, shader.sid);
			}
		}
		glLinkProgram(pid);
		if( !checkLinkingErrors(pid) ) {
			if( pid ) { glDeleteProgram(pid); pid = 0; }
			return;
		}
		log::pure("%s : reloaded\n\n", name.c_str());
	}
}