/*
    imdongye / 2024.07.09
*/

#ifndef __gl_tools_h_
#define __gl_tools_h_

#include <glad/glad.h>

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
}
}


#endif