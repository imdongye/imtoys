//
//	2022-07-20 / im dong ye
//	edit learnopengl code + toys.h by prof shin
//
//	기능:
//	1. Auto Pathing 파일 이름만 있다면 설정해둔 home_dir에서 정해진 shader경로로 포맷에 맞춰 읽어온다.
//	   경로로 입력되면 Auto Pathing하지 않는다.
//	2. Chaining : shadowProgram.attatch("sp_shadow.vert").attatch("shadow.frag")
//						 .attach("shadow.geom").link();
// 
//	TODO list:
//  1. bind with method ( 일단 gltools의 오버로딩된 bind함수 복붙 )
//	2. include 기능 pre processor
//

#ifndef PROGRAM_H
#define PROGRAM_H

#include "limclude.h"

namespace lim
{
	class Program
	{
	public:
		std::string name="unnamed";
		std::string home_dir;
		GLuint pid=0;
	private:
		GLuint vert_id;
		GLuint frag_id;
		GLuint geom_id;
		GLuint comp_id;
	private:
		// Disable Copying and Assignment
		Program(const Program&) = delete;
		Program& operator=(const Program&) = delete;
	public:
		Program(const char* _name = "unnamed", const char* homeDir="common/"): name(_name), home_dir(homeDir) {}
		~Program() { clear(); }
	public:
		// chaining //
		Program& clear()
		{
            if( pid ) glDeleteProgram(pid);
            if( vert_id ) glDeleteShader(vert_id);
            if( frag_id ) glDeleteShader(frag_id);
            if( geom_id ) glDeleteShader(geom_id);
            if( comp_id ) glDeleteShader(comp_id);
			pid = vert_id = frag_id = geom_id = comp_id = 0;
			return *this;
		}
		Program& operator+=(const char* path)
		{
			return attatch(path);
		}
		Program& setHomeDir(std::string_view dir)
		{
			home_dir = dir;
			return *this;
		}
		Program& attatch(std::string path)
		{
			if( pid==0 ) pid = glCreateProgram();

			auto [sid, type] = createShaderAuto(path);

			if( sid==0 ) {
				Logger::get()<<"[error] "<<type<<" extension is not supported.";
				return *this;
			}
			// AUTO PATHING
			if( path.find('/')==std::string::npos && path.find('\\')==std::string::npos ) {
				// todo: 임시 객체 줄이기 최적화
				path = home_dir+"shader/"+std::string{type}+"/"+path;
			}

			// load text
			std::string scode;
			std::ifstream file;
			std::stringstream ss;
			file.exceptions(std::ifstream::failbit|std::ifstream::badbit);
			try {
				file.open(path.c_str());
				ss<<file.rdbuf();
				file.close();
				scode = ss.str();
			} catch( std::ifstream::failure& e ) {
				Logger::get()<<"[error] fail read : "<<path<<", what? "<<e.what()<<Logger::endl;
			}
			if( scode.length()<1 ) {
				Logger::get()<<"[error] "<<path<<" shader code is not loaded properly"<<Logger::endl;
				return *this;
			}

			// compile
			const GLchar* ccode = scode.c_str();
			glShaderSource(sid, 1, &ccode, nullptr);
			glCompileShader(sid);
			if( !checkCompileErrors(sid, path) ) {
				sid = 0;
				return *this;
			}
			glAttachShader(pid, sid);
			Logger::get().log("[program %s] attch %s success\n", name.c_str(), path.c_str());

			return *this;
		}
		Program& link()
		{
			glLinkProgram(pid);
			glUseProgram (pid);
			if( !checkLinkingErrors(pid) ) {
				pid = 0;
				return *this;
			}
            Logger::get().log("[program %s] linking success\n\n", name.c_str());
			return *this;
		}
		GLuint use() const
		{
			if( pid==0 ) {
				Logger::get()<<"[error] program is not linked"<<Logger::endll;
			}
			glUseProgram(pid);
			return pid;
		}
		// todo: bind
		template<typename T> Program& bind(std::string const& name, T&& value)
		{
			int location = glGetUniformLocation(pid, name.c_str());
			if( location == -1 ) Logger::get().log("missing uniform: %s\n", name.c_str());
			else bind(location, std::forward<T>(value));
			return *this;
		}
	private:
		static inline bool checkCompileErrors(GLuint shader, std::string_view path)
		{
			GLint success;
			GLchar infoLog[1024];

			glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
			if( !success ) {
				glGetShaderInfoLog(shader, 1024, NULL, infoLog);
				Logger::get()<<"[error] shader compile error in "<< path.data()<<"\n"<<infoLog << Logger::endl;
				return false;
			}
			return true;
		}
		static inline bool checkLinkingErrors(GLuint shader)
		{
			GLint success;
			GLchar infoLog[1024];

			glGetProgramiv(shader, GL_LINK_STATUS, &success);
			if( !success ) {
				glGetProgramInfoLog(shader, 1024, NULL, infoLog);
				Logger::get()<<"[error] shader linking error\n"<<infoLog<<Logger::endl;
				return false;
			}
			return true;
		}
		// string_view는 char* char[]을 받아도 string으로 임시객체를 만들지 않고 포인터를 사용함
		std::tuple<int, const char*> createShaderAuto(const std::string_view filename)
		{
			int index = filename.rfind(".");
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
	};

	static inline void setUniform(GLuint pid, const std::string_view name, const int& v)
	{
		glUniform1i(glGetUniformLocation(pid, name.data()), v);
	}
	static inline void setUniform(GLuint pid, const std::string_view name, const float& v)
	{
		glUniform1f(glGetUniformLocation(pid, name.data()), v);
	}
	static inline void setUniform(GLuint pid, const std::string_view name, const glm::ivec2& v)
	{
		glUniform2iv(glGetUniformLocation(pid, name.data()), 1, glm::value_ptr(v));
	}
	static inline void setUniform(GLuint pid, const std::string_view name, const glm::ivec3& v)
	{
		glUniform3iv(glGetUniformLocation(pid, name.data()), 1, glm::value_ptr(v));
	}
	static inline void setUniform(GLuint pid, const std::string_view name, const glm::vec2& v)
	{
		glUniform2fv(glGetUniformLocation(pid, name.data()), 1, glm::value_ptr(v));
	}
	static inline void setUniform(GLuint pid, const std::string_view name, const glm::vec3& v)
	{
		glUniform3fv(glGetUniformLocation(pid, name.data()), 1, glm::value_ptr(v));
	}
	static inline void setUniform(GLuint pid, const std::string_view name, const glm::vec4& v)
	{
		glUniform4fv(glGetUniformLocation(pid, name.data()), 1, glm::value_ptr(v));
	}
	static inline void setUniform(GLuint pid, const std::string_view name, const glm::mat3& v)
	{
		glUniformMatrix3fv(glGetUniformLocation(pid, name.data()), 1, 0, glm::value_ptr(v));
	}
	static inline void setUniform(GLuint pid, const std::string_view name, const glm::mat4& v)
	{
		glUniformMatrix4fv(glGetUniformLocation(pid, name.data()), 1, 0, glm::value_ptr(v));
	}
	static inline void setUniform(GLuint pid, const std::string_view name, const glm::vec3* v, int n)
	{
		glUniform3fv(glGetUniformLocation(pid, name.data()), n, (GLfloat*)v);
	}
}

#endif
