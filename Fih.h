#pragma once
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <random>
#include "Entity.h"
#include "Icosphere.h"

class Fih : public Entity {
public:
    float speed = 1.0f;
    float verticalVelocity = 0.0f;
    const float GRAVITY_CONSTANT = 9.81f;
    float playerRadius = 50.0f;

    float aiTimer = 0.0f;
    glm::vec3 currentMoveDir = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 targetMoveDir = glm::vec3(1.0f, 0.0f, 0.0f);

    float physicsTimer = 0.0f;
    const float PHYSICS_TICK_RATE = 0.02f;
    float cachedSurfaceLevel = -1.0f;

    void handleControls(GLFWwindow* window, float deltaTime) override {
        if (glm::length(originalPos) < 0.1f) return;

        physicsTimer += deltaTime;
        bool shouldUpdatePhysics = (physicsTimer >= PHYSICS_TICK_RATE) || (cachedSurfaceLevel < 0.0f);

        float currentRadius = glm::length(originalPos);
        glm::vec3 localUp = glm::normalize(originalPos);

        aiTimer -= deltaTime;
        if (aiTimer <= 0.0f) {
            static std::mt19937 rng(std::random_device{}());
            std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

            glm::vec3 randomVec(dist(rng), dist(rng), dist(rng));
            if (glm::length(randomVec) > 0.01f) {
                targetMoveDir = glm::normalize(randomVec - glm::dot(randomVec, localUp) * localUp);
            }
            aiTimer = 2.0f + (dist(rng) + 1.0f) * 2.0f;
        }

        currentMoveDir = glm::normalize(glm::mix(currentMoveDir, targetMoveDir, deltaTime * 2.0f));
        currentMoveDir = glm::normalize(currentMoveDir - glm::dot(currentMoveDir, localUp) * localUp);

        glm::vec3 rotationAxis = glm::normalize(glm::cross(localUp, currentMoveDir));
        float angle = (speed * deltaTime) / currentRadius;
        glm::mat4 walkRotation = glm::rotate(glm::mat4(1.0f), angle, rotationAxis);

        originalPos = glm::vec3(walkRotation * glm::vec4(originalPos, 1.0f));

        localUp = glm::normalize(originalPos);
        verticalVelocity -= GRAVITY_CONSTANT * deltaTime;
        originalPos += localUp * verticalVelocity * deltaTime;

        if (shouldUpdatePhysics) {
            physicsTimer = 0.0f;

            cachedSurfaceLevel = Icosphere::getPlanetSurfaceHeight(originalPos, playerRadius, 2137, 1.5f, 8, EARTH);
        }

        if (glm::length(originalPos) <= cachedSurfaceLevel) {
            verticalVelocity = 0.0f;
            originalPos = localUp * cachedSurfaceLevel;
        }

        localUp = glm::normalize(originalPos);
        glm::vec3 baseUp = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 alignAxis = glm::cross(baseUp, localUp);

        glm::mat4 alignment = glm::mat4(1.0f);
        if (glm::length(alignAxis) > 0.001f) {
            alignAxis = glm::normalize(alignAxis);
            float alignAngle = acos(glm::clamp(glm::dot(baseUp, localUp), -1.0f, 1.0f));
            alignment = glm::rotate(glm::mat4(1.0f), alignAngle, alignAxis);
        }

        float faceAngle = atan2(-currentMoveDir.z, currentMoveDir.x);

        localRotM = glm::rotate(alignment, faceAngle, glm::vec3(0.0f, 1.0f, 0.0f));
    }
};