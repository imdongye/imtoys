
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
	GLuint ID;
private:
	GLuint vertID=0;
	GLuint fragID=0;
	GLuint geomID=0;
	GLuint compID=0;
private:
	GLuint createAuto(const std::string& filename) {
        auto index = filename.rfind(".");
        auto ext = filename.substr(index + 1);
        if (ext == "vert") {
			vertID = glCreateShader(GL_VERTEX_SHADER);
			return vertID;
		}
        else if (ext == "frag") {
			fragID = glCreateShader(GL_FRAGMENT_SHADER);
			return fragID;
		}
        else if (ext == "geom") {
			geomID = glCreateShader(GL_GEOMETRY_SHADER);
			return geomID;
		}
        else if (ext == "comp") {
			compID = glCreateShader(GL_COMPUTE_SHADER);
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
	// Disable Copying and Assignment
    Program(Program const &) = delete;
    Program & operator=(Program const &) = delete;
public:
	Program() {
		ID = glCreateProgram();
	}
	// chaining //
	Program& reset() {
		cleanUp();
		ID = glCreateProgram();
        return *this;
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
		GLuint sid = createAuto(path);
		const GLchar* ccode = scode.c_str();
		glShaderSource( sid, 1, &ccode, nullptr );
		glCompileShader( sid );
		printInfoShaderLog( sid );
        glAttachShader( ID, sid );
		return *this;
	}
	Program& link() {
		glLinkProgram( ID );
		glUseProgram ( ID );
		printInfoProgramLog( ID );
		return *this;
	}
	Program& activate() {
		glUseProgram(ID);
		return *this;
	}
	// todo: bind
	void cleanUp() {
		if( ID ) glDeleteProgram( ID );
		if( vertID ) glDeleteShader( vertID );
		if( fragID ) glDeleteShader( fragID );
		if( geomID ) glDeleteShader( geomID );
		if( compID ) glDeleteShader( compID );
		ID = vertID = fragID = geomID = compID = 0;
	}
	~Program() {
		cleanUp();
	}
};

#endif // !PROGRAM_H
