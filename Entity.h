#pragma once
#include "GameObject.h"
#define GLFW_INCLUDE_NONE 
#include <GLFW/glfw3.h>

class Entity : public GameObject {
public:
    glm::vec3 originalPos = glm::vec3(0.0f);
    bool isSpawned = false;
    glm::mat4 localRotM = glm::mat4(1.0f);

    bool isFirstPerson = false;

    virtual void handleControls(GLFWwindow* window, float deltaTime) {}

    virtual void update(float planetRotationAngle) {
        if (!isSpawned) {
            originalPos = pos;
            isSpawned = true;
        }

        glm::mat4 planetRotMat = glm::rotate(glm::mat4(1.0f), planetRotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
        pos = glm::vec3(planetRotMat * glm::vec4(originalPos, 1.0f));
        rotM = planetRotMat * localRotM;
    }

    bool operator==(const Entity& e) const {
        return this->getUUID() == e.getUUID();
    }
};