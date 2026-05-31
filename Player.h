#pragma once
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Entity.h"
#include "Icosphere.h"

class Player : public Entity {
private:
    GLFWwindow* savedWindow = nullptr;
    float savedDeltaTime = 0.0f;
    float lastPlanetAngle = 0.0f;
    bool firstUpdate = true;

public:
    Player() {
        isFirstPerson = true;
    }

    float speed = 5.0f;
    float verticalVelocity = 0.0f;
    const float GRAVITY_CONSTANT = 9.81f;
    const float JETPACK_FORCE = 15.0f;
    float playerRadius = 50.0f;

    void handleControls(GLFWwindow* window, float deltaTime) override {
        savedWindow = window;
        savedDeltaTime = deltaTime;
    }

    void update(float planetRotationAngle) override {
        if (!savedWindow) return;
        if (glm::length(originalPos) < 0.1f) return;

        float deltaAngle = 0.0f;
        if (!firstUpdate) {
            deltaAngle = planetRotationAngle - lastPlanetAngle;

            if (deltaAngle < -glm::pi<float>()) deltaAngle += glm::two_pi<float>();
            if (deltaAngle > glm::pi<float>()) deltaAngle -= glm::two_pi<float>();
        }
        lastPlanetAngle = planetRotationAngle;
        firstUpdate = false;

        struct CameraData {
            glm::vec3& pos;
            glm::vec3& front;
            glm::vec3& up;
            glm::vec3& earthPos;
        };
        CameraData* cam = static_cast<CameraData*>(glfwGetWindowUserPointer(savedWindow));
        if (!cam) return;

        float currentRadius = glm::length(originalPos);
        float atmosphereRadius = playerRadius * 1.5f;
        bool inAtmosphere = currentRadius < atmosphereRadius;

        float currentSpeed = speed;
        if (glfwGetKey(savedWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            currentSpeed *= 5.0f;
        }

        glm::mat4 planetRotMat = glm::rotate(glm::mat4(1.0f), planetRotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 invPlanetRot = glm::inverse(planetRotMat);

        if (inAtmosphere) {
            if (deltaAngle != 0.0f) {
                glm::mat4 planetTickRot = glm::rotate(glm::mat4(1.0f), deltaAngle, glm::vec3(0.0f, 1.0f, 0.0f));
                cam->front = glm::normalize(glm::vec3(planetTickRot * glm::vec4(cam->front, 0.0f)));
                cam->up = glm::normalize(glm::vec3(planetTickRot * glm::vec4(cam->up, 0.0f)));
            }

            glm::vec3 localFront = glm::normalize(glm::vec3(invPlanetRot * glm::vec4(cam->front, 0.0f)));
            glm::vec3 localUp = glm::normalize(originalPos);

            glm::vec3 forward = glm::normalize(localFront - glm::dot(localFront, localUp) * localUp);
            glm::vec3 right = glm::normalize(glm::cross(forward, localUp));

            glm::vec3 moveDir(0.0f);
            bool moved = false;

            if (glfwGetKey(savedWindow, GLFW_KEY_W) == GLFW_PRESS) { moveDir += forward; moved = true; }
            if (glfwGetKey(savedWindow, GLFW_KEY_S) == GLFW_PRESS) { moveDir -= forward; moved = true; }
            if (glfwGetKey(savedWindow, GLFW_KEY_A) == GLFW_PRESS) { moveDir -= right;   moved = true; }
            if (glfwGetKey(savedWindow, GLFW_KEY_D) == GLFW_PRESS) { moveDir += right;   moved = true; }

            if (moved && glm::length(moveDir) > 0.001f) {
                moveDir = glm::normalize(moveDir);
                glm::vec3 rotationAxis = glm::normalize(glm::cross(localUp, moveDir));
                float angle = (currentSpeed * savedDeltaTime) / currentRadius;

                glm::mat4 walkRotation = glm::rotate(glm::mat4(1.0f), angle, rotationAxis);
                originalPos = glm::vec3(walkRotation * glm::vec4(originalPos, 1.0f));

                glm::mat4 globalWalkRotation = planetRotMat * walkRotation * invPlanetRot;
                cam->front = glm::normalize(glm::vec3(globalWalkRotation * glm::vec4(cam->front, 0.0f)));
                cam->up = glm::normalize(glm::vec3(globalWalkRotation * glm::vec4(cam->up, 0.0f)));
            }

            localUp = glm::normalize(originalPos);
            if (glfwGetKey(savedWindow, GLFW_KEY_SPACE) == GLFW_PRESS) {
                float forceMultiplier = (glfwGetKey(savedWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ? 2.0f : 1.0f;
                verticalVelocity += JETPACK_FORCE * forceMultiplier * savedDeltaTime;
            }

            verticalVelocity -= GRAVITY_CONSTANT * savedDeltaTime;
            originalPos += localUp * verticalVelocity * savedDeltaTime;

            float surfaceLevel = Icosphere::getPlanetSurfaceHeight(originalPos, playerRadius, 2137, 1.5f, 8, EARTH);
            if (glm::length(originalPos) <= surfaceLevel) {
                verticalVelocity = 0.0f;
                originalPos = glm::normalize(originalPos) * surfaceLevel;
            }

            localUp = glm::normalize(originalPos);
            glm::vec3 baseUp = glm::vec3(0.0f, 1.0f, 0.0f);
            glm::vec3 alignAxis = glm::cross(baseUp, localUp);
            if (glm::length(alignAxis) > 0.001f) {
                alignAxis = glm::normalize(alignAxis);
                float alignAngle = acos(glm::clamp(glm::dot(baseUp, localUp), -1.0f, 1.0f));
                glm::mat4 alignment = glm::rotate(glm::mat4(1.0f), alignAngle, alignAxis);

                float faceAngle = atan2(-localFront.z, localFront.x);
                localRotM = glm::rotate(alignment, faceAngle, glm::vec3(0.0f, 1.0f, 0.0f));
            }
        }
        else {
            verticalVelocity = 0.0f;
            glm::vec3 spaceMove(0.0f);
            glm::vec3 right = glm::normalize(glm::cross(cam->front, cam->up));

            if (glfwGetKey(savedWindow, GLFW_KEY_W) == GLFW_PRESS) spaceMove += cam->front;
            if (glfwGetKey(savedWindow, GLFW_KEY_S) == GLFW_PRESS) spaceMove -= cam->front;
            if (glfwGetKey(savedWindow, GLFW_KEY_A) == GLFW_PRESS) spaceMove -= right;
            if (glfwGetKey(savedWindow, GLFW_KEY_D) == GLFW_PRESS) spaceMove += right;
            if (glfwGetKey(savedWindow, GLFW_KEY_SPACE) == GLFW_PRESS) spaceMove += cam->up;
            if (glfwGetKey(savedWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) spaceMove -= cam->up;

            if (glm::length(spaceMove) > 0.001f) {
                glm::vec3 worldPos = glm::vec3(planetRotMat * glm::vec4(originalPos, 1.0f));
                worldPos += glm::normalize(spaceMove) * currentSpeed * savedDeltaTime;
                originalPos = glm::vec3(invPlanetRot * glm::vec4(worldPos, 1.0f));
            }
        }

        Entity::update(planetRotationAngle);
    }

    void updateCamera(glm::vec3& camPos, glm::vec3& camFront, glm::vec3& camUp) {
        if (glm::length(pos) < 0.1f) return;
        glm::vec3 playerUp = glm::normalize(pos);

        float eyeHeight = 1.8f;
        camPos = pos + (playerUp * eyeHeight);

        if (glm::length(originalPos) < playerRadius * 1.5f) {
            camUp = playerUp;
        }
    }
};