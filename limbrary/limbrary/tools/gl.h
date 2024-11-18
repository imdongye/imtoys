/*
	imdongye@naver.com
	fst: 2022-07-09
	lst: 2024-07-09

Note:
    glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
*/

#ifndef __gl_tools_h_
#define __gl_tools_h_

#include <glad/glad.h>
#include <cassert>

namespace lim {
    namespace gl {
        inline void safeDelBufs(GLuint* ids, int n=1) {
            for(int i=0; i<n; i++) {
                GLuint& id = ids[i];
                if( id!=0 ) {
                    glDeleteBuffers(n, &id);
                    id=0;
                }
            }
        }
        inline void safeDelVertArrs(GLuint* ids, int n=1) {
            for(int i=0; i<n; i++) {
                GLuint& id = ids[i];
                if( id!=0 ) {
                    glDeleteVertexArrays(n, &id);
                    id=0;
                }
            }
        }
        inline void safeDelXfbs(GLuint* ids, int n=1) {
            for(int i=0; i<n; i++) {
                GLuint& id = ids[i];
                if( id!=0 ) {
                    glDeleteTransformFeedbacks(n, &id);
                    id=0;
                }
            }
        }
        inline void safeDelTexs(GLuint* ids, int n=1) {
            for(int i=0; i<n; i++) {
                GLuint& id = ids[i];
                if( id!=0 ) {
                    glDeleteTextures(n, &id);
                    id=0;
                }
            }
        }

        inline void errorAssert() {
            assert(glGetError()==GL_NO_ERROR);
        }
    }
}

#endif