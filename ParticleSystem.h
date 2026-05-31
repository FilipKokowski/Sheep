#pragma once
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include <random>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "GameObject.h"
#include "Icosphere.h"
#include "EntityHandler.h"
#include "Entity.h"

class ParticleSystem {
public:
    // Lista statycznych obiektów środowiskowych (np. drzewa, skały)
    std::vector<GameObject> staticObjects;

    // Uniwersalny generator obiektów statycznych (GameObject) w danym przedziale wysokości
    void spawnStaticObjects(int count, glm::vec3 planetCenter, float planetRadius,
        GameObject& templateObj, float minNormHeight, float maxNormHeight,
        int seed, float freq, int octaves)
    {
        std::mt19937 rng(seed);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

        for (int i = 0; i < count; i++) {
            glm::vec3 randomDir;
            do {
                randomDir = glm::vec3(dist(rng), dist(rng), dist(rng));
            } while (glm::length(randomDir) < 0.1f);
            randomDir = glm::normalize(randomDir);

            float surfaceHeight = Icosphere::getPlanetSurfaceHeight(randomDir, planetRadius, seed, freq, octaves, EARTH) - 0.5f;
            float normHeight = surfaceHeight / planetRadius;

            // WARUNEK OBSZARU: Sprawdzamy czy wylosowana wysokość mieści się w zdefiniowanym biome
            if (normHeight < minNormHeight || normHeight > maxNormHeight) {
                continue; // Pomijamy to miejsce, szukamy dalej
            }

            GameObject newObj = templateObj; // Kopiujemy szablon graficzny
            newObj.pos = planetCenter + (randomDir * surfaceHeight);

            // Losowe skalowanie
            float randomScale = 0.6f + (dist(rng) + 1.0f) * 0.4f;
            newObj.scale = glm::vec3(randomScale);

            // Orientacja pionowa do kuli planety
            glm::vec3 localUp = glm::vec3(0.0f, 1.0f, 0.0f);
            glm::vec3 rotationAxis = glm::cross(localUp, randomDir);
            glm::mat4 localRotation = glm::mat4(1.0f);

            if (glm::length(rotationAxis) > 0.001f) {
                rotationAxis = glm::normalize(rotationAxis);
                float angle = acos(glm::clamp(glm::dot(localUp, randomDir), -1.0f, 1.0f));
                localRotation = glm::rotate(glm::mat4(1.0f), angle, rotationAxis);
            }

            // Losowy obrót wokół własnej osi Y (żeby obiekty nie stały identycznie)
            std::uniform_real_distribution<float> rotDist(0.0f, glm::two_pi<float>());
            newObj.rotM = glm::rotate(localRotation, rotDist(rng), glm::vec3(0.0f, 1.0f, 0.0f));

            staticObjects.push_back(newObj);
        }
    }

    // Uniwersalny generator obiektów żywych (Entity/Fih) w danym przedziale wysokości
    template<typename T>
    void spawnLivingEntities(int count, glm::vec3 planetCenter, float planetRadius,
        T& templateEntity, float minNormHeight, float maxNormHeight,
        int seed, float freq, int octaves)
    {
        std::mt19937 rng(seed + 999); // Inny seed, żeby ryby nie stały dokładnie w drzewach
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

        for (int i = 0; i < count; i++) {
            glm::vec3 randomDir;
            do {
                randomDir = glm::vec3(dist(rng), dist(rng), dist(rng));
            } while (glm::length(randomDir) < 0.1f);
            randomDir = glm::normalize(randomDir);

            float surfaceHeight = Icosphere::getPlanetSurfaceHeight(randomDir, planetRadius, seed, freq, octaves, EARTH);
            float normHeight = surfaceHeight / planetRadius;

            // WARUNEK OBSZARU dla istot żywych
            if (normHeight < minNormHeight || normHeight > maxNormHeight) {
                continue;
            }

            // Alokujemy dynamicznie obiekt przekazanego typu (polimorfizm w C++)
            T* realEntity = new T(templateEntity);
            realEntity->originalPos = planetCenter + (randomDir * surfaceHeight);
            realEntity->pos = realEntity->originalPos;

            float randomScale = 0.6f + (dist(rng) + 1.0f) * 0.4f;
            realEntity->scale = glm::vec3(randomScale);

            // Rejestrujemy w handlerze logiki i renderu
            EntityHandler::add(realEntity);
        }
    }

    // Rysowanie obiektów statycznych (Ziemia obraca je razem ze sobą)
    void drawStatic(glm::mat4 view, glm::mat4 projection, float planetRotationAngle) {
        glm::mat4 planetRotMat = glm::rotate(glm::mat4(1.0f), planetRotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));

        for (auto& obj : staticObjects) {
            glm::vec3 originalPos = obj.pos;
            obj.pos = glm::vec3(planetRotMat * glm::vec4(originalPos, 1.0f));

            glm::mat4 originalRot = obj.rotM;
            obj.rotM = planetRotMat * originalRot;

            obj.draw(view, projection);

            obj.pos = originalPos;
            obj.rotM = originalRot;
        }
    }

    void clearStatic() {
        staticObjects.clear();
    }
};