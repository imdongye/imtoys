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
	bool checkLinkingErrors(GLuint shader)
	{
		GLint success = 0;
		GLchar infoLog[1024];

		glGetProgramiv(shader, GL_LINK_STATUS, &success);
		if( !success ) {
			glGetProgramInfoLog(shader, 1024, NULL, infoLog);
			lim::log::err("shader linking error\n%s\n",infoLog);

			std::abort();
			return false;
		}
		return true;
	}
	GLenum getType(std::string_view path)
	{
		size_t index = path.rfind(".");
		std::string_view ext = path.substr(index + 1);
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
		uniform_location_cache = std::move(src.uniform_location_cache);

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

		// 파일이름만 들어왔을때 home_dir 상대경로로 설정
		if( path.find('/')==std::string::npos && path.find('\\')==std::string::npos ) {
			path = home_dir+"/shaders/"+path;
		}

		GLenum type = getType(path);
		if(type==0)
			return*this;
		GLuint sid = glCreateShader(type);
		std::string strSrc = readStrFromFile(path);
		const GLchar* src = strSrc.c_str();
		glShaderSource(sid, 1, &src, nullptr);
		glCompileShader(sid);
		if( !checkCompileErrors(sid, path) ) {
			glDeleteShader(sid);
			return *this;
		}

		glAttachShader(pid, sid);
		log::pure("%s prog : atatched %s\n", name.c_str(), path.c_str());

		switch (type)
		{
		case GL_VERTEX_SHADER:
			vert_id = sid;
			vert_path = path;
			break;
		case GL_FRAGMENT_SHADER:
			frag_id = sid;
			frag_path = path;
			break;
		case GL_GEOMETRY_SHADER:
			geom_id = sid;
			geom_path = path;
			break;
		case GL_COMPUTE_SHADER:
			comp_id = sid;
			comp_path = path;
			break;
		}

		return *this;
	}
	Program& Program::link()
	{
		glLinkProgram(pid);
		if( !checkLinkingErrors(pid) ) {
			return *this;
		}
		log::pure("%s prog: linked\n\n", name.c_str());
		return *this;
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
		if( uniform_location_cache.find(vname) != uniform_location_cache.end() ) {
			return uniform_location_cache[vname];
		}
		GLint loc = glGetUniformLocation(pid, vname.c_str());
		uniform_location_cache[vname] = loc;
		//if(loc<0) log::err("missing...");
		return loc;
	}
		
	Program& Program::reload(GLenum type)
	{
		GLuint* curSid;
		const char* path;
		switch (type)
		{
		case GL_VERTEX_SHADER:
			curSid = &vert_id;
			path = vert_path.c_str();
			break;
		case GL_FRAGMENT_SHADER:
			curSid = &frag_id;
			path = frag_path.c_str();
			break;
		case GL_GEOMETRY_SHADER:
			curSid = &geom_id;
			path = geom_path.c_str();
			break;
#ifndef __APPLE__
		case GL_COMPUTE_SHADER:
			curSid = &comp_id;
			path = comp_path.c_str();
			break;
#endif
		default:
			log::err("can't reload that type\n");
			return *this;
		}

		GLuint oldSid = *curSid;
		glDetachShader(pid, oldSid);
		
		GLuint newSid = glCreateShader(type);
		std::string strSrc = readStrFromFile(path);
		const GLchar* src = strSrc.c_str();
		glShaderSource(newSid, 1, &src, nullptr);
		glCompileShader(newSid);
		if( !checkCompileErrors(newSid, path) ) {
			return *this;
		}
		glAttachShader(pid, newSid);
		glLinkProgram(pid);
		if( !checkLinkingErrors(pid) ) {
			return *this;
		}
		glDeleteShader(oldSid);
		*curSid = newSid;
		log::pure("%s prog: reload at %s\n", name.c_str(), path);
		return *this;
	}
}