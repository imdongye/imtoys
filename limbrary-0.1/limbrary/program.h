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

#ifndef __program_h_
#define __program_h_

#include <string>
#include <glad/glad.h>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <limbrary/logger.h>

namespace lim
{
	class Program
	{
	public:
		std::string name;
		std::string home_dir;
		GLuint pid = 0;

	private:
		GLuint vert_id;
		GLuint frag_id;
		GLuint geom_id;
		GLuint comp_id;
		mutable std::unordered_map<std::string, GLint> uniform_location_cache;

	private:
		// Disable Copying and Assignment
		Program(const Program &) = delete;
		Program &operator=(const Program &) = delete;

	public:
		Program(const char *_name = "unnamed", const char *homeDir = "assets");
		~Program();

	public:
		// chaining //
		Program &clear();
		Program &operator+=(const char *path);
		Program &setHomeDir(std::string_view dir);
		Program &attatch(std::string path);
		Program &link();
		GLuint use() const;
		// todo: bind
		template <typename T>
		Program &bind(std::string const &vname, T &&value)
		{
			int location = glGetUniformLocation(pid, vname.c_str());
			if (location == -1)
				Log::get().log("missing uniform: %s\n", vname.c_str());
			else
				bind(location, std::forward<T>(value));
			return *this;
		}

	private:
		static bool checkCompileErrors(GLuint shader, std::string_view path);
		static bool checkLinkingErrors(GLuint shader);
		// string_view는 char* char[]을 받아도 string으로 임시객체를 만들지 않고 포인터를 사용함
		std::tuple<int, const char *> createShaderAuto(const std::string_view filename);
		// From: https://www.youtube.com/watch?v=nBB0LGSIm5Q
		GLint getUniformLocation(const std::string_view vname) const;

	public:
		inline void setUniform(const std::string_view vname, const int v) const
		{
			glUniform1i(getUniformLocation(vname), v);
		}
		inline void setUniform(const std::string_view vname, const int v[], int n) const
		{
			glUniform1iv(getUniformLocation(vname), n, (GLint *)v);
		}
		inline void setUniform(const std::string_view vname, const glm::ivec2 &v) const
		{
			glUniform2iv(getUniformLocation(vname), 1, glm::value_ptr(v));
		}
		inline void setUniform(const std::string_view vname, const glm::ivec3 &v) const
		{
			glUniform3iv(getUniformLocation(vname), 1, glm::value_ptr(v));
		}
		inline void setUniform(const std::string_view vname, const float v) const
		{
			glUniform1f(getUniformLocation(vname), v);
		}
		inline void setUniform(const std::string_view vname, const glm::vec2 &v) const
		{
			glUniform2fv(getUniformLocation(vname), 1, glm::value_ptr(v));
		}
		inline void setUniform(const std::string_view vname, const glm::vec3 &v) const
		{
			glUniform3fv(getUniformLocation(vname), 1, glm::value_ptr(v));
		}
		inline void setUniform(const std::string_view vname, const glm::vec4 &v) const
		{
			glUniform4fv(getUniformLocation(vname), 1, glm::value_ptr(v));
		}
		inline void setUniform(const std::string_view vname, const glm::vec3 *v, int n) const
		{
			glUniform3fv(getUniformLocation(vname), n, (GLfloat *)v);
		}
		inline void setUniform(const std::string_view vname, const glm::mat3 &v) const
		{
			glUniformMatrix3fv(getUniformLocation(vname), 1, 0, glm::value_ptr(v));
		}
		inline void setUniform(const std::string_view vname, const glm::mat4 &v) const
		{
			glUniformMatrix4fv(getUniformLocation(vname), 1, 0, glm::value_ptr(v));
		}
	};
	static inline void setUniform(GLuint pid, const std::string_view vname, const int v)
	{
		glUniform1i(glGetUniformLocation(pid, vname.data()), v);
	}
	static inline void setUniform(GLuint pid, const std::string_view vname, const glm::ivec2 &v)
	{
		glUniform2iv(glGetUniformLocation(pid, vname.data()), 1, glm::value_ptr(v));
	}
	static inline void setUniform(GLuint pid, const std::string_view vname, const glm::ivec3 &v)
	{
		glUniform3iv(glGetUniformLocation(pid, vname.data()), 1, glm::value_ptr(v));
	}
	static inline void setUniform(GLuint pid, const std::string_view vname, const glm::vec2 &v)
	{
		glUniform2fv(glGetUniformLocation(pid, vname.data()), 1, glm::value_ptr(v));
	}
	static inline void setUniform(GLuint pid, const std::string_view vname, const glm::vec3 &v)
	{
		glUniform3fv(glGetUniformLocation(pid, vname.data()), 1, glm::value_ptr(v));
	}
	static inline void setUniform(GLuint pid, const std::string_view vname, const glm::vec4 &v)
	{
		glUniform4fv(glGetUniformLocation(pid, vname.data()), 1, glm::value_ptr(v));
	}
	static inline void setUniform(GLuint pid, const std::string_view vname, const glm::vec3 *v, int n)
	{
		glUniform3fv(glGetUniformLocation(pid, vname.data()), n, (GLfloat *)v);
	}
	static inline void setUniform(GLuint pid, const std::string_view vname, const glm::mat3 &v)
	{
		glUniformMatrix3fv(glGetUniformLocation(pid, vname.data()), 1, 0, glm::value_ptr(v));
	}
	static inline void setUniform(GLuint pid, const std::string_view vname, const glm::mat4 &v)
	{
		glUniformMatrix4fv(glGetUniformLocation(pid, vname.data()), 1, 0, glm::value_ptr(v));
	}
}

#endif
