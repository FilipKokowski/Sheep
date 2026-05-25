#pragma once

#include <string>

#include "GameObject.h"
#include <random>
#include "Icosphere.h"

class ParticleSystem {

public:
    std::vector<GameObject> trees;

    void generateTrees(int count, glm::vec3 planetCenter, float planetRadius, unsigned int shader, int seed, float freq, int octaves) {
        trees.clear();

        std::mt19937 rng(seed);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

        GameObject tree;
        tree.setShader(shader);
        tree.setModel("tree.obj", "tree.png");

        for (int i = 0; i < count; i++) {
            glm::vec3 randomDir;
            do {
                randomDir = glm::vec3(dist(rng), dist(rng), dist(rng));
            } while (glm::length(randomDir) < 0.1f);
            randomDir = glm::normalize(randomDir);

            float surfaceHeight = Icosphere::getPlanetSurfaceHeight(randomDir, planetRadius, seed, freq, octaves, EARTH) - .5f;
            float normHeight = surfaceHeight / planetRadius;

            if (normHeight < 1.002 || normHeight > 1.040) {
                i--; continue;
            }
        

            glm::vec3 treePos = planetCenter + (randomDir * surfaceHeight);

            tree.pos = treePos;

            float randomScale = 0.6f + (dist(rng) + 1.0f) * 0.4f;
            tree.scale = glm::vec3(randomScale);

            glm::vec3 localUp = glm::vec3(0.0f, 1.0f, 0.0f);
            glm::vec3 targetUp = randomDir;

            glm::vec3 rotationAxis = glm::cross(localUp, targetUp);
            float cosAngle = glm::dot(localUp, targetUp);

            glm::mat4 localRotation = glm::mat4(1.0f);

            if (glm::length(rotationAxis) > 0.001f) {
                rotationAxis = glm::normalize(rotationAxis);
                float angle = acos(glm::clamp(cosAngle, -1.0f, 1.0f));
                localRotation = glm::rotate(glm::mat4(1.0f), angle, rotationAxis);
            }

            std::uniform_real_distribution<float> rotDist(0.0f, glm::two_pi<float>());
            localRotation = glm::rotate(localRotation, rotDist(rng), glm::vec3(0.0f, 1.0f, 0.0f));

            tree.rotM = localRotation;

            trees.push_back(tree);
        }
    }

    void draw(glm::mat4 view, glm::mat4 projection, float planetRotationAngle) {
        glm::mat4 planetRotMat = glm::rotate(glm::mat4(1.0f), planetRotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));

        for (auto& tree : trees) {
            glm::vec3 originalPos = tree.pos;
            tree.pos = glm::vec3(planetRotMat * glm::vec4(originalPos, 1.0f));

            glm::mat4 originalRot = tree.rotM;
            tree.rotM = planetRotMat * originalRot;

            tree.draw(view, projection);

            tree.pos = originalPos;
            tree.rotM = originalRot;
        }
    }

};
