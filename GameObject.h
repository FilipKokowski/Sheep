#pragma once

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <GL/glew.h>

#include <string>

#include "ModelLoader.h"
#include <glm/gtc/type_ptr.hpp>

class GameObject {
public:

    glm::vec3 
        pos = glm::vec3(0.0f, 0.0f, 0.0f), 
        rot = glm::vec3(0.0f, 0.0f, 0.0f), 
        scale = glm::vec3(1.0f, 1.0f, 1.0f);

    glm::mat4 rotM = glm::mat4(1.0f);
    
    std::vector<glm::vec3> vertices = {};
    unsigned int tex = 0;
    unsigned int VAO = 0;

    unsigned int modelShader = 0;

    GameObject() {}

    glm::mat4 getModelMatrix() {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, pos);

        model = model * rotM;

        model = glm::scale(model, scale);
        return model;
    }

    bool setModel(const char* model, const char* tex = "") {
        std::vector< glm::vec2 > uvs;
        std::vector< glm::vec3 > normals;
        if (!ModelLoader::loadOBJ(model, tex, vertices, uvs, normals, this->tex))
            return false;

        unsigned int modelVBO, modelEBO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &modelVBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, modelVBO);

        GLsizeiptr vertSize = vertices.size() * sizeof(glm::vec3);
        GLsizeiptr uvSize = uvs.size() * sizeof(glm::vec2);
        glBufferData(GL_ARRAY_BUFFER, vertSize + uvSize, nullptr, GL_STATIC_DRAW);

        glBufferSubData(GL_ARRAY_BUFFER, 0, vertSize, &vertices[0]);
        glBufferSubData(GL_ARRAY_BUFFER, vertSize, uvSize, &uvs[0]);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)vertSize);
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);

        return true;
    }

    void setShader(unsigned int shader) {
        this->modelShader = shader;
    }

    void update() {

    }

    void draw(glm::mat4 view, glm::mat4 projection) {
       // glUseProgram(modelShader);

        glUniformMatrix4fv(glGetUniformLocation(modelShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(modelShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glm::mat4 modelMat = getModelMatrix();

        glUniformMatrix4fv(glGetUniformLocation(modelShader, "model"), 1, GL_FALSE, glm::value_ptr(modelMat));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        glUniform1i(glGetUniformLocation(modelShader, "tex"), 0);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vertices.size());
        glBindVertexArray(0);
    }

};