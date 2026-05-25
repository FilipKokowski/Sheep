#include "Icosphere.h"
#include <cmath>
#include <map>
#include <algorithm>

Icosphere::Icosphere() : VAO(0), VBO(0), EBO(0), isGenerating(false), hasNewData(false), indexCounter(0) {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
}

Icosphere::~Icosphere() {
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO) glDeleteBuffers(1, &EBO);
    if (calculationFuture.valid()) {
        calculationFuture.wait();
    }
}

unsigned int Icosphere::addVertex(glm::vec3 p) {
    baseVertices.push_back(glm::normalize(p));
    return indexCounter++;
}

unsigned int Icosphere::getMiddlePoint(unsigned int p1, unsigned int p2) {
    bool firstIsSmaller = p1 < p2;
    unsigned long long smallerIndex = firstIsSmaller ? p1 : p2;
    unsigned long long greaterIndex = firstIsSmaller ? p2 : p1;
    unsigned long long key = (smallerIndex << 32) + greaterIndex;

    auto it = middlePointIndexCache.find(key);
    if (it != middlePointIndexCache.end()) {
        return it->second;
    }

    glm::vec3 point1 = baseVertices[p1];
    glm::vec3 point2 = baseVertices[p2];
    glm::vec3 middle = glm::vec3(
        (point1.x + point2.x) / 2.0f,
        (point1.y + point2.y) / 2.0f,
        (point1.z + point2.z) / 2.0f
    );

    unsigned int i = addVertex(middle);
    middlePointIndexCache[key] = i;
    return i;
}

float Icosphere::calculateHeight(glm::vec3 sphereDir, float radius, int seed, float freq, int octaves, PlanetType planetType) {
    FastNoiseLite localNoise;
    localNoise.SetSeed(seed);
    localNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    localNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    localNoise.SetFrequency(freq);
    localNoise.SetFractalOctaves(octaves);

    float heightOffset = 0.0f;

    if (planetType != ASTEROID) {
        float continent = localNoise.GetNoise(sphereDir.x * 0.8f, sphereDir.y * 0.8f, sphereDir.z * 0.8f);
        float mountain = localNoise.GetNoise(sphereDir.x * 3.5f, sphereDir.y * 3.5f, sphereDir.z * 3.5f);
        float ridge = 1.0f - std::abs(mountain);

        if (continent > 0.15f) heightOffset = ((continent - 0.15f) * 0.10f) + (ridge * (continent - 0.15f) * 0.12f);
        else heightOffset = (continent - 0.15f) * 0.06f;
    }
    else {
        float baseShape = localNoise.GetNoise(sphereDir.x * .6f, sphereDir.y * .6f, sphereDir.z * .6f);
        float craters = localNoise.GetNoise(sphereDir.x * 4.0f, sphereDir.y * 4.0f, sphereDir.z * 4.0f);
        heightOffset = baseShape - craters * .5f;
    }

    return radius + (heightOffset * radius);
}

void Icosphere::buildSubdividedSphere(float radius, int seed, float freq, int octaves, glm::vec3 cameraPos, PlanetType planetType) {
    baseVertices.clear();
    middlePointIndexCache.clear();
    indexCounter = 0;

    float t = (1.0f + std::sqrt(5.0f)) / 2.0f;

    addVertex(glm::vec3(-1.0f, t, 0.0f));
    addVertex(glm::vec3(1.0f, t, 0.0f));
    addVertex(glm::vec3(-1.0f, -t, 0.0f));
    addVertex(glm::vec3(1.0f, -t, 0.0f));

    addVertex(glm::vec3(0.0f, -1.0f, t));
    addVertex(glm::vec3(0.0f, 1.0f, t));
    addVertex(glm::vec3(0.0f, -1.0f, -t));
    addVertex(glm::vec3(0.0f, 1.0f, -t));

    addVertex(glm::vec3(t, 0.0f, -1.0f));
    addVertex(glm::vec3(t, 0.0f, 1.0f));
    addVertex(glm::vec3(-t, 0.0f, -1.0f));
    addVertex(glm::vec3(-t, 0.0f, 1.0f));

    std::vector<TriangleIndices> faces;
    faces.push_back({ 0, 11, 5 });  faces.push_back({ 0, 5, 1 });   faces.push_back({ 0, 1, 7 });   faces.push_back({ 0, 7, 10 });  faces.push_back({ 0, 10, 11 });
    faces.push_back({ 1, 5, 9 });   faces.push_back({ 5, 11, 4 });  faces.push_back({ 11, 10, 2 }); faces.push_back({ 10, 7, 6 });  faces.push_back({ 7, 1, 8 });
    faces.push_back({ 3, 9, 4 });   faces.push_back({ 3, 4, 2 });   faces.push_back({ 3, 2, 6 });   faces.push_back({ 3, 6, 8 });   faces.push_back({ 3, 8, 9 });
    faces.push_back({ 4, 9, 5 });   faces.push_back({ 2, 4, 11 });  faces.push_back({ 6, 2, 10 });  faces.push_back({ 8, 6, 7 });   faces.push_back({ 9, 8, 1 });

    const int MAX_LOD_LEVEL = 6;

    for (int i = 0; i < MAX_LOD_LEVEL; i++) {
        std::vector<TriangleIndices> faces2;
        for (const auto& tri : faces) {
            glm::vec3 triCenter = (baseVertices[tri.v1] + baseVertices[tri.v2] + baseVertices[tri.v3]) / 3.0f;
            triCenter = glm::normalize(triCenter);

            glm::vec3 worldTriPos = triCenter * radius;
            float distToTri = glm::distance(cameraPos, worldTriPos);

            int targetLevel = 2;

            if (distToTri < radius * 0.6f)       targetLevel = 6;
            else if (distToTri < radius * 1.5f)  targetLevel = 5;
            else if (distToTri < radius * 2.5f)  targetLevel = 4; 
            else if (distToTri < radius * 3.5f)  targetLevel = 3;

            if (i < targetLevel) {
                unsigned int a = getMiddlePoint(tri.v1, tri.v2);
                unsigned int b = getMiddlePoint(tri.v2, tri.v3);
                unsigned int c = getMiddlePoint(tri.v3, tri.v1);

                faces2.push_back({ tri.v1, a, c });
                faces2.push_back({ tri.v2, b, a });
                faces2.push_back({ tri.v3, c, b });
                faces2.push_back({ a, b, c });
            }
            else {
                faces2.push_back(tri);
            }
        }
        faces = faces2;
    }

    std::vector<Vertex> localVertices;
    std::vector<unsigned int> localIndices;

    std::map<unsigned long long, std::pair<unsigned int, unsigned int>> edgeUsage;
    auto registerEdge = [&](unsigned int u1, unsigned int u2) {
        unsigned int minV = std::min(u1, u2);
        unsigned int maxV = std::max(u1, u2);
        unsigned long long edgeKey = ((unsigned long long)minV << 32) | maxV;
        if (edgeUsage.find(edgeKey) == edgeUsage.end()) {
            edgeUsage[edgeKey] = { 1, 0 };
        }
        else {
            edgeUsage[edgeKey].first++;
        }
        };

    for (const auto& tri : faces) {
        registerEdge(tri.v1, tri.v2);
        registerEdge(tri.v2, tri.v3);
        registerEdge(tri.v3, tri.v1);
    }

    localVertices.resize(baseVertices.size());
    for (size_t i = 0; i < baseVertices.size(); ++i) {
        glm::vec3 sphereDir = baseVertices[i];
        float h = calculateHeight(sphereDir, radius, seed, freq, octaves, planetType);

        localVertices[i].position = sphereDir * h;
        localVertices[i].normal = sphereDir;

        float heightRel = (h - radius) / (radius * 0.2f);
        if (planetType == EARTH) {
            if (heightRel < -0.01f) localVertices[i].color = glm::vec3(0.08f, 0.18f, 0.36f);
            else if (heightRel < 0.02f) localVertices[i].color = glm::vec3(0.12f, 0.32f, 0.58f);
            else if (heightRel < 0.05f) localVertices[i].color = glm::vec3(0.76f, 0.72f, 0.52f);
            else if (heightRel < 0.25f) localVertices[i].color = glm::vec3(0.18f, 0.48f, 0.16f);
            else if (heightRel < 0.45f) localVertices[i].color = glm::vec3(0.36f, 0.28f, 0.22f);
            else localVertices[i].color = glm::vec3(0.92f, 0.92f, 0.95f);
        }
        else {
            float c = 0.3f + heightRel * 0.4f;
            localVertices[i].color = glm::vec3(c, c, c * 1.05f);
        }
    }

    for (const auto& tri : faces) {
        localIndices.push_back(tri.v1);
        localIndices.push_back(tri.v2);
        localIndices.push_back(tri.v3);
    }

    unsigned int skirtStartIndex = static_cast<unsigned int>(localVertices.size());

    int subdivisions = 7;
    float skirtDepth = (radius * 0.04f) / (float)(subdivisions + 1);

    for (auto const& [edgeKey, info] : edgeUsage) {
        if (info.first == 1) {
            unsigned int v1_idx = (unsigned int)(edgeKey >> 32);
            unsigned int v2_idx = (unsigned int)(edgeKey & 0xFFFFFFFF);

            Vertex origV1 = localVertices[v1_idx];
            Vertex origV2 = localVertices[v2_idx];

            Vertex skirtV1 = origV1;
            Vertex skirtV2 = origV2;

            skirtV1.position -= glm::normalize(origV1.position) * skirtDepth;
            skirtV2.position -= glm::normalize(origV2.position) * skirtDepth;

            skirtV1.color *= 0.2f;
            skirtV2.color *= 0.2f;

            unsigned int newV1_idx = static_cast<unsigned int>(localVertices.size());
            localVertices.push_back(skirtV1);
            unsigned int newV2_idx = static_cast<unsigned int>(localVertices.size());
            localVertices.push_back(skirtV2);

            localIndices.push_back(v1_idx);
            localIndices.push_back(newV1_idx);
            localIndices.push_back(v2_idx);

            localIndices.push_back(v2_idx);
            localIndices.push_back(newV1_idx);
            localIndices.push_back(newV2_idx);
        }
    }

    for (size_t i = 0; i < faces.size(); ++i) {
        unsigned int i0 = localIndices[i * 3 + 0];
        unsigned int i1 = localIndices[i * 3 + 1];
        unsigned int i2 = localIndices[i * 3 + 2];

        glm::vec3 v0 = localVertices[i0].position;
        glm::vec3 v1 = localVertices[i1].position;
        glm::vec3 v2 = localVertices[i2].position;

        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 faceNormal = glm::normalize(glm::cross(edge1, edge2));

        localVertices[i0].normal += faceNormal;
        localVertices[i1].normal += faceNormal;
        localVertices[i2].normal += faceNormal;
    }

    for (size_t i = 0; i < skirtStartIndex; ++i) {
        localVertices[i].normal = glm::normalize(localVertices[i].normal);
    }
    for (size_t i = skirtStartIndex; i < localVertices.size(); ++i) {
        localVertices[i].normal = glm::normalize(localVertices[i].normal);
    }

    std::lock_guard<std::mutex> lock(dataMutex);
    threadVertices = std::move(localVertices);
    threadIndices = std::move(localIndices);
    hasNewData = true;
}

bool Icosphere::generateAsync(float radius, int seed, float freq, int octaves, glm::vec3 cameraPos, PlanetType planetType) {
    if (isGenerating) {
        if (calculationFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            calculationFuture.get();
            isGenerating = false;
        }
        else {
            return false;
        }
    }

    isGenerating = true;
    hasNewData = false;

    calculationFuture = std::async(std::launch::async, &Icosphere::buildSubdividedSphere, this, radius, seed, freq, octaves, cameraPos, planetType);
    return true;
}

void Icosphere::updateGLBuffers() {
    std::lock_guard<std::mutex> lock(dataMutex);
    if (!hasNewData) return;

    meshVertices = std::move(threadVertices);
    meshIndices = std::move(threadIndices);
    hasNewData = false;

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, meshVertices.size() * sizeof(Vertex), meshVertices.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, meshIndices.size() * sizeof(unsigned int), meshIndices.data(), GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

    glBindVertexArray(0);
}

void Icosphere::draw() {
    if (meshIndices.empty()) return;

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(meshIndices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

float Icosphere::getPlanetSurfaceHeight(glm::vec3 position, float radius, int seed, float freq, int octaves, PlanetType planetType) {
    FastNoiseLite localNoise;
    localNoise.SetSeed(seed);
    localNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    localNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    localNoise.SetFrequency(freq);
    localNoise.SetFractalOctaves(octaves);

    glm::vec3 sphereDir = glm::normalize(position);
    float heightOffset = 0.0f;

    if (planetType != ASTEROID) {
        float continent = localNoise.GetNoise(sphereDir.x * 0.8f, sphereDir.y * 0.8f, sphereDir.z * 0.8f);
        float mountain = localNoise.GetNoise(sphereDir.x * 3.5f, sphereDir.y * 3.5f, sphereDir.z * 3.5f);
        float ridge = 1.0f - std::abs(mountain);

        if (continent > 0.15f) heightOffset = ((continent - 0.15f) * 0.10f) + (ridge * (continent - 0.15f) * 0.12f);
        else heightOffset = (continent - 0.15f) * 0.06f;
    }
    else {
        float baseShape = localNoise.GetNoise(sphereDir.x * .6f, sphereDir.y * .6f, sphereDir.z * .6f);
        float craters = localNoise.GetNoise(sphereDir.x * 4.0f, sphereDir.y * 4.0f, sphereDir.z * 4.0f);
        heightOffset = baseShape - craters * .5f;
    }

    return radius + (heightOffset * radius);
}