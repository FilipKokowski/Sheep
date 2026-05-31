#pragma once

#include "Entity.h"

class MovementSystem {
public:

    static void applyMovement(Entity* entity, float planetRotationAngle) {
        if (!entity) return;

        if (!entity->isSpawned) {
            entity->originalPos = entity->pos;
            entity->isSpawned = true;
        }

        glm::mat4 planetRotMat = glm::rotate(glm::mat4(1.0f), planetRotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));

        entity->pos = glm::vec3(planetRotMat * glm::vec4(entity->originalPos, 1.0f));

        entity->rotM = planetRotMat * entity->localRotM;
    }
};