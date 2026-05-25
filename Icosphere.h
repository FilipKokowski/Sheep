#ifndef ICOSPHERE_H
#define ICOSPHERE_H

#define GLEW_STATIC
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <future>
#include <mutex>
#include <map>

#include "FastNoiseLite.h"

enum PlanetType {
    EARTH,
    ASTEROID
};

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
};

class Icosphere {
public:
    Icosphere();
    ~Icosphere();

    void updateGLBuffers();

    bool generateAsync(float radius, int seed, float freq, int octaves, glm::vec3 cameraPos, PlanetType planetType);
    static float getPlanetSurfaceHeight(glm::vec3 position, float radius, int seed, float freq, int octaves, PlanetType planetType = EARTH);

    void draw();

private:
    struct TriangleIndices {
        unsigned int v1, v2, v3;
    };

    unsigned int addVertex(glm::vec3 p);
    unsigned int getMiddlePoint(unsigned int p1, unsigned int p2);

    float calculateHeight(glm::vec3 lodPos, float radius, int seed, float freq, int octaves, PlanetType planetType);

    void buildSubdividedSphere(float radius, int seed, float freq, int octaves, glm::vec3 cameraPos, PlanetType planetType);

    unsigned int VAO, VBO, EBO;
    std::vector<Vertex> meshVertices;
    std::vector<unsigned int> meshIndices;

    std::vector<Vertex> threadVertices;
    std::vector<unsigned int> threadIndices;

    std::future<void> calculationFuture;
    std::mutex dataMutex;
    bool isGenerating;
    bool hasNewData;

    std::vector<glm::vec3> baseVertices;
    std::map<unsigned long long, unsigned int> middlePointIndexCache;
    unsigned int indexCounter;
};

#endif