//
// 2022-07-20 / im dong ye
// edit learnopengl code + toys.h by prof shin
//

#ifndef PROGRAM_H
#define PROGRAM_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cassert>
#include <memory>

struct Program {
public:
	GLuint ID=0;
private:
	GLuint vertID=0;
	GLuint fragID=0;
	GLuint geomID=0;
	GLuint compID=0;
private:
	GLuint createAuto(const std::string& filename, std::string& get_type) {
        auto index = filename.rfind(".");
        auto ext = filename.substr(index + 1);
        if (ext=="vert"||ext=="vs") {
			vertID = glCreateShader(GL_VERTEX_SHADER);
			get_type = "vertex";
			return vertID;
		}
        else if (ext=="frag"||ext=="fs") {
			fragID = glCreateShader(GL_FRAGMENT_SHADER);
			get_type = "fragment";
			return fragID;
		}
        else if (ext=="geom"||ext=="gs") {
			geomID = glCreateShader(GL_GEOMETRY_SHADER);
			get_type = "geometry";
			return geomID;
		}
        else if (ext=="comp"||ext=="cs") {
			compID = glCreateShader(GL_COMPUTE_SHADER);
			get_type = "compute";
			return compID;
		}
        else return 0;
    }
	static inline void printInfoProgramLog(GLuint obj )
	{
		int infologLength = 0, charsWritten  = 0;
		glGetProgramiv( obj, GL_INFO_LOG_LENGTH, &infologLength );
		if( infologLength <=0 ) return;
		std::cerr<<"Program Info:"<<std::endl;
		char *infoLog = new char[infologLength];
		glGetProgramInfoLog( obj, infologLength, &charsWritten, infoLog );
		std::cerr<<infoLog<<std::endl;
		delete [] infoLog;
	}
	static inline void printInfoShaderLog(GLuint obj )
	{
		int infologLength = 0, charsWritten  = 0;
		glGetProgramiv( obj, GL_INFO_LOG_LENGTH, &infologLength );
		if( infologLength <=0 ) return;
		std::cerr<<"Program Info:"<<std::endl;
		char* infoLog = new char[infologLength];
		glGetProgramInfoLog( obj, infologLength, &charsWritten, infoLog );
		std::cerr<<infoLog<<std::endl;
		delete [] infoLog;
	}
	static inline void checkCompileErrors(GLuint shader, std::string type) {
		GLint success;
        GLchar infoLog[1024];
        if (type != "program") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
	}
	// Disable Copying and Assignment
    Program(Program const &) = delete;
    Program & operator=(Program const &) = delete;
public:
	Program(): ID(0) {}
	// chaining //
	Program& reset() {
		cleanUp();
		ID = glCreateProgram();
	}
	Program& attatch(const char* path) {
		if( ID==0 ) ID = glCreateProgram();

		// load text
		std::string scode;
		std::ifstream file;
		file.exceptions(std::ifstream::failbit|std::ifstream::badbit);
		try {
			file.open(path);
			std::stringstream ss;
			ss<<file.rdbuf();
			file.close();
			scode = ss.str();
		} catch( std::ifstream::failure& e ) {
			std::cerr<< "[error] fail read : "<<path<<". what? "<<e.what()<<std::endl;
		}
		// compile
		std::string type;
		GLuint sid = createAuto(path, type);
		const GLchar* ccode = scode.c_str();
		glShaderSource( sid, 1, &ccode, nullptr );
		glCompileShader( sid );
		checkCompileErrors( sid, type );
		glAttachShader( ID, sid );

		return *this;
	}
	Program& link() {
		glLinkProgram( ID );
		glUseProgram ( ID );
		checkCompileErrors(ID, "program");
		cleanUpWithoutID();
		return *this;
	}
	Program& use() {
		glUseProgram(ID);
		return *this;
	}
	// todo: bind
	template<typename T> Program & bind(std::string const & name, T&& value)
    {
        int location = glGetUniformLocation(ID, name.c_str());
        if (location == -1) fprintf(stderr, "Missing Uniform: %s\n", name.c_str());
        else bind(location, std::forward<T>(value));
        return *this;
    }
	void cleanUp() {
		if( ID ) glDeleteProgram( ID );
		if( vertID ) glDeleteShader( vertID );
		if( fragID ) glDeleteShader( fragID );
		if( geomID ) glDeleteShader( geomID );
		if( compID ) glDeleteShader( compID );
		ID = vertID = fragID = geomID = compID = 0;
	}
	void cleanUpWithoutID() {
		if( vertID ) glDeleteShader( vertID );
		if( fragID ) glDeleteShader( fragID );
		if( geomID ) glDeleteShader( geomID );
		if( compID ) glDeleteShader( compID );
		vertID = fragID = geomID = compID = 0;
	}
	~Program() {
		cleanUp();
	}
};

#endif // !PROGRAM_H