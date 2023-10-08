/*

Program v2.0
2023-10-08 / im dong ye

Program은 쉐이더를 프로그램 링크후 삭제하고
Reloadable은 남겨두었다가 리로드할때 특정 쉐이더만 다시 컴파일한다.

Usage:
1. Auto Pathing 파일 이름만 있다면 설정해둔 home_dir에서 정해진 shader경로로 포맷에 맞춰 읽어온다.
   경로로 입력되면 Auto Pathing하지 않는다.
2. Chaining : shadowProgram.attatch("sp_shadow.vert").attatch("shadow.frag")
					 .attach("shadow.geom").link();

Note:
glDeleteShader은 모든 Progrma에 detach 되어야만 동작함.

Todo:
1. include 기능 pre processor

*/

#ifndef __program_h_
#define __program_h_

#include <string>
#include <glad/glad.h>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <functional>
#include <vector>
#include <limbrary/texture.h>

namespace lim
{
	class Program
	{
	public:
		std::string name = "nonamed";
		std::string home_dir = "assets";
		std::function<void(const Program&)> use_hook = [](const Program& p){};

	protected:
		struct Shader {
			GLuint sid = 0;
			GLenum type = 0;
			std::string path;
			void createAndCompile();
			void deinitGL();
		};
		bool reloadable = false;
		std::vector<Shader> shaders;
		GLuint pid = 0;
		mutable std::unordered_map<std::string, GLint> uniform_location_cache;

	protected:
		// Disable Copying and Assignment
		Program(const Program&) = delete;
		Program& operator=(const Program&) = delete;
	public:
		Program(std::string_view name="nonamed");
		Program(Program&& src) noexcept;
		Program& operator=(Program&& src) noexcept;
		virtual ~Program() noexcept;

		// chaining //
		Program& deinitGL();
		Program& deinitGLAll();
		Program& operator+=(const char *path);
		Program& attatch(std::string path);
		Program& link();
		// type : GL_VERTEX_SHADER, GL_FRAGMENT_SHADER ...
		Program& reload(GLenum type);

		const Program& use() const;

	private:
		GLint getUniformLocation(const std::string& vname) const;
	public:
		inline const Program& setUniform(const std::string& vname, const int v) const {
			glUniform1i(getUniformLocation(vname), v);
			return *this;
		}
		inline const Program& setUniform(const std::string& vname, int n, const int v[]) const {
			glUniform1iv(getUniformLocation(vname), n, (GLint*)v);
			return *this;
		}
		inline const Program& setUniform(const std::string& vname, const glm::ivec2& v) const {
			glUniform2iv(getUniformLocation(vname), 1, glm::value_ptr(v));
			return *this;
		}
		inline const Program& setUniform(const std::string& vname, const glm::ivec3& v) const {
			glUniform3iv(getUniformLocation(vname), 1, glm::value_ptr(v));
			return *this;
		}
		inline const Program& setUniform(const std::string& vname, const float v) const {
			glUniform1f(getUniformLocation(vname), v);
			return *this;
		}
		inline const Program& setUniform(const std::string& vname, const glm::vec2& v) const {
			glUniform2fv(getUniformLocation(vname), 1, glm::value_ptr(v));
			return *this;
		}
		inline const Program& setUniform(const std::string& vname, const glm::vec3& v) const {
			glUniform3fv(getUniformLocation(vname), 1, glm::value_ptr(v));
			return *this;
		}
		inline const Program& setUniform(const std::string& vname, const glm::vec4& v) const {
			glUniform4fv(getUniformLocation(vname), 1, glm::value_ptr(v));
			return *this;
		}
		inline const Program& setUniform(const std::string& vname, int n, const glm::vec3 *v) const {
			glUniform3fv(getUniformLocation(vname), n, (GLfloat*)v);
			return *this;
		}
		inline const Program& setUniform(const std::string& vname, const glm::mat3& v) const {
			glUniformMatrix3fv(getUniformLocation(vname), 1, 0, glm::value_ptr(v));
			return *this;
		}
		inline const Program& setUniform(const std::string& vname, const glm::mat4& v) const {
			glUniformMatrix4fv(getUniformLocation(vname), 1, 0, glm::value_ptr(v));
			return *this;
		}
		inline const Program& setUniform(const std::string& vname, const TexBase& tex, int activeSlot) const {
			glActiveTexture(GL_TEXTURE0 + activeSlot);
            glBindTexture(GL_TEXTURE_2D, tex.tex_id);
           	setUniform(vname, activeSlot);
			return *this;
		}
	};

	class ProgramReloadable: public Program
	{
	public:
		ProgramReloadable(std::string_view name="nonamed");
		ProgramReloadable(ProgramReloadable&& src);
		ProgramReloadable& operator=(ProgramReloadable&& src) noexcept;
		// type : GL_VERTEX_SHADER, GL_FRAGMENT_SHADER ...
		void reload(GLenum type);
	};
}

#endif
