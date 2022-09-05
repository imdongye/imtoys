//
//	2022-07-20 / im dong ye
//	edit learnopengl code + toys.h by prof shin
//
//	to use : shadowProgram.attatch("sp_shadow.vert").attatch("shadow.frag")
//						 .attach("shadow.geom").link();
//
//	TODO list:
//  1. bind with method ( 일단 gltools의 오버로딩된 bind함수 복붙 )
//	2. include 기능
//

#ifndef PROGRAM_H
#define PROGRAM_H

#include "limclude.h"

namespace lim
{
	class Program
	{
	public:
		std::string name;
		GLuint ID;
	private:
		GLuint vertID;
		GLuint fragID;
		GLuint geomID;
		GLuint compID;
	private:
		// Disable Copying and Assignment
		Program(Program const&) = delete;
		Program& operator=(Program const&) = delete;
	public:
		Program(const char* _name = "unnamed"): ID(0), name(_name) {}
		~Program() { clear(); }

		// chaining //
		Program& reset()
		{
			clear();
			ID = glCreateProgram();
			return *this;
		}
		Program& operator+=(const char* path)
		{
			return attatch(path);
		}
		Program& attatch(const char* path)
		{
			if( ID==0 ) ID = glCreateProgram();

			std::string type;
			GLuint sid = createShaderAuto(path, type);
			if( sid==0 )
			{
				std::cerr<<"[error] "<<type<<" extension is not supported.";
				return *this;
			}

			// load text
			std::string scode;
			std::ifstream file;
			std::stringstream ss;
			file.exceptions(std::ifstream::failbit|std::ifstream::badbit);
			try
			{
				file.open(path);
				ss<<file.rdbuf();
				file.close();
				scode = ss.str();
			} catch( std::ifstream::failure& e )
			{
				std::cerr<<"[error] fail read : "<<path<<". what? "<<e.what()<<std::endl;
			}
			if( scode.length()<1 )
			{
				std::cerr<<"[error]"<<path<<" shader code is not loaded properly"<<std::endl;
				return *this;
			}
			// compile
			const GLchar* ccode = scode.c_str();
			glShaderSource(sid, 1, &ccode, nullptr);
			glCompileShader(sid);
			checkCompileErrors(sid, type);
			glAttachShader(ID, sid);
			fprintf(stdout, "[program %s] attch %s success\n", name.c_str(), path);

			return *this;
		}
		Program& link()
		{
			glLinkProgram(ID);
			glUseProgram (ID);
			checkCompileErrors(ID, "program");
			clearWithoutID(); // 링크된 후 필요없음
			fprintf(stdout, "[program %s] linking success\n\n", name.c_str());
			return *this;
		}
		GLuint use() const
		{
			glUseProgram(ID);
			return ID;
		}
		// todo: bind
		template<typename T> Program& bind(std::string const& name, T&& value)
		{
			int location = glGetUniformLocation(ID, name.c_str());
			if( location == -1 ) fprintf(stderr, "missing uniform: %s\n", name.c_str());
			else bind(location, std::forward<T>(value));
			return *this;
		}
		void clear()
		{
			if( ID ) glDeleteProgram(ID);
			ID = 0;
			clearWithoutID();
		}
	private:
		void clearWithoutID()
		{
			if( vertID ) glDeleteShader(vertID);
			if( fragID ) glDeleteShader(fragID);
			if( geomID ) glDeleteShader(geomID);
			if( compID ) glDeleteShader(compID);
			vertID = fragID = geomID = compID = 0;

		}
		static inline void checkCompileErrors(GLuint shader, std::string type)
		{
			GLint success;
			GLchar infoLog[1024];

			if( type != "program" )
			{
				glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
				if( !success )
				{
					glGetShaderInfoLog(shader, 1024, NULL, infoLog);
					std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog
						<< "\n -- --------------------------------------------------- -- " << std::endl;
				}
			}
			else
			{
				glGetProgramiv(shader, GL_LINK_STATUS, &success);
				if( !success )
				{
					glGetProgramInfoLog(shader, 1024, NULL, infoLog);
					std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog
						<< "\n -- --------------------------------------------------- -- " << std::endl;
				}
			}
		}
		GLuint createShaderAuto(const std::string_view filename, std::string_view get_type)
		{
			auto index = filename.rfind(".");
			auto ext = filename.substr(index + 1);
			if( ext=="vert"||ext=="vs" )
			{
				vertID = glCreateShader(GL_VERTEX_SHADER);
				get_type = "vertex";
				return vertID;
			}
			else if( ext=="frag"||ext=="fs" )
			{
				fragID = glCreateShader(GL_FRAGMENT_SHADER);
				get_type = "fragment";
				return fragID;
			}
			else if( ext=="geom"||ext=="gs" )
			{
				geomID = glCreateShader(GL_GEOMETRY_SHADER);
				get_type = "geometry";
				return geomID;
			}
			else if( ext=="comp"||ext=="cs" )
			{
				compID = glCreateShader(GL_COMPUTE_SHADER);
				get_type = "compute";
				return compID;
			}
			else
			{
				get_type = ext;
				return 0;
			}
		}
	};

	static inline void setUniform(GLuint prog, const std::string& name, const int& v)
	{
		glUniform1i(glGetUniformLocation(prog, name.c_str()), v);
	}
	static inline void setUniform(GLuint prog, const std::string& name, const float& v)
	{
		glUniform1f(glGetUniformLocation(prog, name.c_str()), v);
	}
	static inline void setUniform(GLuint prog, const std::string& name, const glm::ivec2& v)
	{
		glUniform2iv(glGetUniformLocation(prog, name.c_str()), 1, glm::value_ptr(v));
	}
	static inline void setUniform(GLuint prog, const std::string& name, const glm::ivec3& v)
	{
		glUniform3iv(glGetUniformLocation(prog, name.c_str()), 1, glm::value_ptr(v));
	}
	static inline void setUniform(GLuint prog, const std::string& name, const glm::vec2& v)
	{
		glUniform2fv(glGetUniformLocation(prog, name.c_str()), 1, glm::value_ptr(v));
	}
	static inline void setUniform(GLuint prog, const std::string& name, const glm::vec3& v)
	{
		glUniform3fv(glGetUniformLocation(prog, name.c_str()), 1, glm::value_ptr(v));
	}
	static inline void setUniform(GLuint prog, const std::string& name, const glm::vec4& v)
	{
		glUniform4fv(glGetUniformLocation(prog, name.c_str()), 1, glm::value_ptr(v));
	}
	static inline void setUniform(GLuint prog, const std::string& name, const glm::mat3& v)
	{
		glUniformMatrix3fv(glGetUniformLocation(prog, name.c_str()), 1, 0, glm::value_ptr(v));
	}
	static inline void setUniform(GLuint prog, const std::string& name, const glm::mat4& v)
	{
		glUniformMatrix4fv(glGetUniformLocation(prog, name.c_str()), 1, 0, glm::value_ptr(v));
	}
	static inline void setUniform(GLuint prog, const std::string& name, const glm::vec3* v, int n)
	{
		glUniform3fv(glGetUniformLocation(prog, name.c_str()), n, (GLfloat*)v);
	}

} // namespace lim
#endif // !PROGRAM_H
