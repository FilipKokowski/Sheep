#ifndef SHAPE_H
#define SHAPE_H

#define GLEW_STATIC
#include <GL/glew.h>
#include <vector>

class Shape {
public:
    Shape();
    virtual ~Shape();
    void draw();

protected:
    void setupMesh(const std::vector<float>& vertices, const std::vector<float>& normals, const std::vector<unsigned int>& indices);

    unsigned int vao, vboPos, vboNorm, ebo;
    int indexCount;
};

#endif