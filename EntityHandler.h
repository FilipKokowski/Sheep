#pragma once
#include <vector>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "Entity.h"

class EntityHandler {
    static std::vector<Entity*> entities;
public:
    static void add(Entity* entity) { entities.push_back(entity); }
    static void remove(Entity* entity) {
        for (std::vector<Entity*>::iterator it = entities.begin(); it != entities.end(); it++) {
            if (*it == entity) {
                entities.erase(it); return;
            }
        }
    }
    static void updateLogic(GLFWwindow* window, float planetRotationAngle, float deltaTime) {
        for (Entity* entity : entities) {
            if (entity) {
                entity->handleControls(window, deltaTime);
                entity->update(planetRotationAngle);
            }
        }
    }

    static void draw(glm::mat4 view, glm::mat4 projection, unsigned int shader, bool isShadowPass) {
        for (Entity* entity : entities) {
            if (entity) {
                if (!isShadowPass && entity->isFirstPerson) {
                    continue;
                }

                entity->setShader(shader);
                entity->draw(view, projection);
            }
        }
    }

    static std::vector<Entity*> get() { return entities; }
};