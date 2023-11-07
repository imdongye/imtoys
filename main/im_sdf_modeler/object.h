/*

2023-11-02 / im dong ye

*/

#ifndef __object_h_
#define __object_h_
#include <vector>
#include <limbrary/model_view/transform.h>

class group

class Combination: public TransformWithInv{
public:
    std::vector<Object> child;
public:
    Combination() {
        
    }

};

class Object : public TransformWithInv{

public:
    Object() {

    }

};

#endif