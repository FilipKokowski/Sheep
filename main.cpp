#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "Icosphere.h"
#include "ModelLoader.h"
#include "GameObject.h"
#include "ParticleSystem.h"

glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 55.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float lastX = 640.0f, lastY = 360.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

int currentSeed = 2137;
float currentFrequency = 1.5f;
int currentOctaves = 8;

float verticalVelocity = 0.0f;
const float GRAVITY_CONSTANT = 9.81f;
const float JETPACK_FORCE = 15.0f;

float planetRotationAngle = 0;
float planetRotationSpeed = .005f;

enum Filters {
    NONE,
    VHS,
    NEGATIVE,
    MONOCHROME,
    SKETCH
};

int activeFilterType = Filters::VHS;
int TOTAL_FILTERS = 5;

struct CameraData {
    glm::vec3& pos;
    glm::vec3& front;
    glm::vec3& up;
    glm::vec3& earthPos;
};

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    CameraData* data = static_cast<CameraData*>(glfwGetWindowUserPointer(window));
    if (!data) return;

    if (firstMouse) {
        lastX = (float)xpos; lastY = (float)ypos; firstMouse = false; return;
    }
    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;
    lastX = (float)xpos; lastY = (float)ypos;

    if (std::abs(xoffset) > 100.0f || std::abs(yoffset) > 100.0f) return;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    glm::mat4 yawRotation = glm::rotate(glm::mat4(1.0f), glm::radians(-xoffset), data->up);
    data->front = glm::vec3(yawRotation * glm::vec4(data->front, 0.0f));

    glm::vec3 right = glm::normalize(glm::cross(data->front, data->up));
    glm::mat4 pitchRotation = glm::rotate(glm::mat4(1.0f), glm::radians(yoffset), right);
    glm::vec3 newFront = glm::vec3(pitchRotation * glm::vec4(data->front, 0.0f));

    float dotVal = glm::dot(glm::normalize(newFront), data->up);
    float angleToUp = glm::degrees(acos(glm::clamp(dotVal, -1.0f, 1.0f)));

    if (angleToUp > 5.0f && angleToUp < 175.0f) {
        data->front = newFront;
    }

    data->front = glm::normalize(data->front);
}

std::string loadShaderSource(const char* filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cout << "Blad: Nie mozna otworzyc pliku shadera: " << filepath << std::endl;
        return "";
    }
    std::stringstream stream; stream << file.rdbuf(); return stream.str();
}

unsigned int compileShader(unsigned int type, const std::string& source) {
    unsigned int id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    int success;
    char infoLog[512];
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(id, 512, nullptr, infoLog);
        std::cout << "BŁĄD KOMPILACJI SHADERA (" << (type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT") << "):\n" << infoLog << std::endl;
    }

    return id;
}

unsigned int createShaderProgramFromSources(const std::string& vertexSource, const std::string& fragmentSource) {
    unsigned int vs = compileShader(GL_VERTEX_SHADER, vertexSource);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    unsigned int program = glCreateProgram();
    glAttachShader(program, vs); glAttachShader(program, fs); glLinkProgram(program);

    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cout << "BŁĄD LINKOWANIA PROGRAMU SHADERA:\n" << infoLog << std::endl;
    }

    glDeleteShader(vs); glDeleteShader(fs);
    return program;
}

unsigned int createShaderProgram(const char* vertexPath, const char* fragmentPath) {
    std::string vertexSource = loadShaderSource(vertexPath);
    std::string fragmentSource = loadShaderSource(fragmentPath);
    return createShaderProgramFromSources(vertexSource, fragmentSource);
}

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_RED_BITS, 8);
    glfwWindowHint(GLFW_GREEN_BITS, 8);
    glfwWindowHint(GLFW_BLUE_BITS, 8);
    glfwWindowHint(GLFW_ALPHA_BITS, 8);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);

    int windowWidth = 1280;
    int windowHeight = 720;
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Custom Space Sandbox", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    if (glewInit() != GLEW_OK) return -1;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    unsigned int terrainShader = createShaderProgram("terrain.vs", "terrain.fs");
    unsigned int skyboxShader = createShaderProgram("skybox.vs", "skybox.fs");
    unsigned int modelShader = createShaderProgram("model.vs", "model.fs");
    unsigned int shadowShader = createShaderProgram("shadow.vs", "shadow.fs");

    std::string vhsVertexSrc = R"(
        #version 330 core
        out vec2 TexCoords;
        void main() {
            float x = -1.0 + float((gl_VertexID & 1) << 2);
            float y = -1.0 + float((gl_VertexID & 2) << 1);
            gl_Position = vec4(x, y, 0.0, 1.0);
            TexCoords = vec2(x * 0.5 + 0.5, y * 0.5 + 0.5);
        }
    )";
    std::string vhsFragmentSrc = loadShaderSource("vhs.fs");
    unsigned int vhsShader = createShaderProgramFromSources(vhsVertexSrc, vhsFragmentSrc);

    unsigned int dummyVAO;
    glGenVertexArrays(1, &dummyVAO);
    glm::vec3 sunColor = glm::vec3(1.0f, 0.96f, 0.88f);

    unsigned int screenTexture;
    glGenTextures(1, &screenTexture);
    glBindTexture(GL_TEXTURE_2D, screenTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, windowWidth, windowHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);

    bool plusPressed = false;
    bool minusPressed = false;

    bool fPressed = false;
    bool flashlightOn = false;

    Icosphere earth;
    glm::vec3 earthPos(0.0f, 0.0f, 0.0f);
    float earthRadius = 50.0f;
    glm::vec3 lastRegenEarth(0.0f);

    ParticleSystem trees;
    trees.generateTrees(500, earthPos, earthRadius, createShaderProgram("model.vs", "model.fs"), currentSeed, currentFrequency, currentOctaves);

    earth.generateAsync(earthRadius, currentSeed, currentFrequency, currentOctaves, cameraPos, EARTH);
    lastRegenEarth = cameraPos;

    float G = GRAVITY_CONSTANT;

    CameraData camData = { cameraPos, cameraFront, cameraUp, earthPos };
    glfwSetWindowUserPointer(window, &camData);

    const unsigned int shadowRes = 4096;
    unsigned int depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);

    unsigned int depthMap;
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowRes, shadowRes, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        float frameRotationFrame = planetRotationSpeed * deltaTime;
        planetRotationAngle += frameRotationFrame;
        if (planetRotationAngle > glm::two_pi<float>()) {
            planetRotationAngle -= glm::two_pi<float>();
        }

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);

        if (glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) {
            if (!plusPressed) { activeFilterType = (activeFilterType + 1) % TOTAL_FILTERS; plusPressed = true; }
        }
        else { plusPressed = false; }

        if (glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS) {
            if (!minusPressed) { activeFilterType = (activeFilterType - 1 + TOTAL_FILTERS) % TOTAL_FILTERS; minusPressed = true; }
        }
        else { minusPressed = false; }

        if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
            if (!fPressed) {
                flashlightOn = !flashlightOn;
                fPressed = true;
            }
        }
        else { fPressed = false; }

        float distToPlanet = glm::distance(cameraPos, earthPos);
        float atmosphereRadius = earthRadius * 1.5f;
        bool inAtmosphere = distToPlanet < atmosphereRadius;

        if (inAtmosphere) {
            glm::mat4 dynamicPlanetRot = glm::rotate(glm::mat4(1.0f), frameRotationFrame, glm::vec3(0.0f, 1.0f, 0.0f));
            cameraPos = earthPos + glm::vec3(dynamicPlanetRot * glm::vec4(cameraPos - earthPos, 1.0f));
            cameraFront = glm::normalize(glm::vec3(dynamicPlanetRot * glm::vec4(cameraFront, 0.0f)));
            cameraUp = glm::normalize(glm::vec3(dynamicPlanetRot * glm::vec4(cameraUp, 0.0f)));

            glm::vec3 planetNormal = glm::normalize(cameraPos - earthPos);

            glm::vec3 moveRight = glm::normalize(glm::cross(cameraFront, cameraUp));
            glm::vec3 moveForward = glm::normalize(glm::cross(cameraUp, moveRight));

            float cameraSpeed = 5.0f * deltaTime;
            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) cameraSpeed *= 5;

            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPos += cameraSpeed * moveForward;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPos -= cameraSpeed * moveForward;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cameraPos -= cameraSpeed * moveRight;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cameraPos += cameraSpeed * moveRight;

            planetNormal = glm::normalize(cameraPos - earthPos);
            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) verticalVelocity += JETPACK_FORCE * deltaTime;
            verticalVelocity -= G * deltaTime;
            cameraPos += planetNormal * verticalVelocity * deltaTime;

            glm::mat4 invPlanetRot = glm::rotate(glm::mat4(1.0f), -planetRotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
            glm::vec3 rotatedPosVec = glm::vec3(invPlanetRot * glm::vec4(cameraPos - earthPos, 1.0f));

            float surfaceLevel = Icosphere::getPlanetSurfaceHeight(rotatedPosVec, earthRadius, currentSeed, currentFrequency, currentOctaves, EARTH);

            if (glm::distance(cameraPos, earthPos) <= surfaceLevel + 1.8f) {
                verticalVelocity = 0;
                cameraPos = earthPos + glm::normalize(cameraPos - earthPos) * (surfaceLevel + 1.8f);
            }

            planetNormal = glm::normalize(cameraPos - earthPos);
            glm::vec3 newUp = planetNormal;
            glm::vec3 axis = glm::cross(cameraUp, newUp);
            float sinAngle = glm::length(axis);

            if (sinAngle > 0.0000001f) {
                axis = glm::normalize(axis);
                float cosAngle = glm::clamp(glm::dot(cameraUp, newUp), -1.0f, 1.0f);
                float angle = atan2(sinAngle, cosAngle);

                glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), angle, axis);
                cameraFront = glm::normalize(glm::vec3(rotation * glm::vec4(cameraFront, 0.0f)));
            }
            else if (glm::dot(cameraUp, newUp) < -0.99f) {
                axis = glm::normalize(glm::cross(cameraFront, cameraUp));
                glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), axis);
                cameraFront = glm::normalize(glm::vec3(rotation * glm::vec4(cameraFront, 0.0f)));
            }
            cameraUp = newUp;

        }
        else {
            glm::vec3 moveForward = cameraFront;
            glm::vec3 moveRight = glm::normalize(glm::cross(cameraFront, cameraUp));
            verticalVelocity = 0.0f;

            float cameraSpeed = 5.0f * deltaTime;
            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) cameraSpeed *= 5;

            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPos += cameraSpeed * moveForward;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPos -= cameraSpeed * moveForward;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cameraPos -= cameraSpeed * moveRight;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cameraPos += cameraSpeed * moveRight;

            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) cameraPos += cameraUp * (JETPACK_FORCE * deltaTime);
            if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) cameraPos -= cameraUp * (JETPACK_FORCE * deltaTime);

            if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
                glm::mat4 roll = glm::rotate(glm::mat4(1.0f), glm::radians(-60.0f * deltaTime), cameraFront);
                cameraUp = glm::normalize(glm::vec3(roll * glm::vec4(cameraUp, 0.0f)));
            }
            if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
                glm::mat4 roll = glm::rotate(glm::mat4(1.0f), glm::radians(60.0f * deltaTime), cameraFront);
                cameraUp = glm::normalize(glm::vec3(roll * glm::vec4(cameraUp, 0.0f)));
            }
        }

        glm::mat4 invPlanetRot = glm::rotate(glm::mat4(1.0f), -planetRotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::vec3 rotatedCamPos = glm::vec3(invPlanetRot * glm::vec4(cameraPos - earthPos, 1.0f));

        if (glm::distance(cameraPos, lastRegenEarth) > (earthRadius * 0.03f)) {
            if (earth.generateAsync(earthRadius, currentSeed, currentFrequency, currentOctaves, rotatedCamPos, EARTH)) {
                lastRegenEarth = cameraPos;
            }
        }
        earth.updateGLBuffers();

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)windowWidth / (float)windowHeight, 0.05f, 1000.0f);

        glm::vec3 lightDir = glm::normalize(glm::vec3(1.0f, 0.2f, 1.0f));
        glm::vec3 lightPos = earthPos + (lightDir * 150.0f);

        float near_plane = 1.0f, far_plane = 300.0f;
        glm::mat4 lightProjection = glm::ortho(-120.0f, 120.0f, -120.0f, 120.0f, near_plane, far_plane);
        glm::mat4 lightView = glm::lookAt(lightPos, earthPos, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 lightSpaceMatrix = lightProjection * lightView;

        glm::mat4 modelEarth = glm::translate(glm::mat4(1.0f), earthPos);
        modelEarth = glm::rotate(modelEarth, planetRotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));

        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        glViewport(0, 0, shadowRes, shadowRes);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);

        glUseProgram(shadowShader);
        glUniformMatrix4fv(glGetUniformLocation(shadowShader, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

        glUniformMatrix4fv(glGetUniformLocation(shadowShader, "model"), 1, GL_FALSE, glm::value_ptr(modelEarth));
        earth.draw();

        for (auto& tree : trees.trees) {
            tree.setShader(shadowShader);
        }
        trees.draw(view, projection, planetRotationAngle);
        for (auto& tree : trees.trees) {
            tree.setShader(modelShader);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glViewport(0, 0, windowWidth, windowHeight);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

        glDisable(GL_CULL_FACE); glDisable(GL_DEPTH_TEST);
        glUseProgram(skyboxShader);
        glm::mat4 skyboxView = glm::mat4(glm::mat3(view));
        glm::mat4 invViewProj = glm::inverse(projection * skyboxView);
        glUniformMatrix4fv(glGetUniformLocation(skyboxShader, "invViewProj"), 1, GL_FALSE, glm::value_ptr(invViewProj));
        glUniform1f(glGetUniformLocation(skyboxShader, "time"), currentFrame);
        glUniform3fv(glGetUniformLocation(skyboxShader, "lightDir"), 1, glm::value_ptr(lightDir));
        glUniform3fv(glGetUniformLocation(skyboxShader, "sunColor"), 1, glm::value_ptr(sunColor));
        glBindVertexArray(dummyVAO); glDrawArrays(GL_TRIANGLES, 0, 3);
        glEnable(GL_DEPTH_TEST); glEnable(GL_CULL_FACE);

        float cutOff = flashlightOn ? glm::cos(glm::radians(12.5f)) : 3.0f;
        float outerCutOff = flashlightOn ? glm::cos(glm::radians(17.5f)) : 2.0f;

        glUseProgram(terrainShader);
        glUniform3fv(glGetUniformLocation(terrainShader, "flashLightPos"), 1, glm::value_ptr(cameraPos));
        glUniform3fv(glGetUniformLocation(terrainShader, "flashLightDir"), 1, glm::value_ptr(cameraFront));
        glUniform1f(glGetUniformLocation(terrainShader, "flashLightCutOff"), cutOff);
        glUniform1f(glGetUniformLocation(terrainShader, "flashLightOuterCutOff"), outerCutOff);

        glUniformMatrix4fv(glGetUniformLocation(terrainShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(terrainShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(glGetUniformLocation(terrainShader, "lightDir"), 1, glm::value_ptr(lightDir));
        glUniform3fv(glGetUniformLocation(terrainShader, "lightColor"), 1, glm::value_ptr(sunColor));
        glUniform3fv(glGetUniformLocation(terrainShader, "viewPos"), 1, glm::value_ptr(cameraPos));
        glUniformMatrix4fv(glGetUniformLocation(terrainShader, "model"), 1, GL_FALSE, glm::value_ptr(modelEarth));
        glUniform1f(glGetUniformLocation(terrainShader, "planetRadius"), earthRadius);
        glUniform1i(glGetUniformLocation(terrainShader, "planetType"), EARTH);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glUniform1i(glGetUniformLocation(terrainShader, "shadowMap"), 1);
        glUniformMatrix4fv(glGetUniformLocation(terrainShader, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

        earth.draw();

        glUseProgram(modelShader);
        glUniform3fv(glGetUniformLocation(modelShader, "flashLightPos"), 1, glm::value_ptr(cameraPos));
        glUniform3fv(glGetUniformLocation(modelShader, "flashLightDir"), 1, glm::value_ptr(cameraFront));
        glUniform1f(glGetUniformLocation(modelShader, "flashLightCutOff"), cutOff);
        glUniform1f(glGetUniformLocation(modelShader, "flashLightOuterCutOff"), outerCutOff);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glUniform1i(glGetUniformLocation(modelShader, "shadowMap"), 1);
        glUniformMatrix4fv(glGetUniformLocation(modelShader, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

        trees.draw(view, projection, planetRotationAngle);

        glBindTexture(GL_TEXTURE_2D, screenTexture);
        glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, windowWidth, windowHeight, 0);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        glUseProgram(vhsShader);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, screenTexture);
        glUniform1i(glGetUniformLocation(vhsShader, "screenTexture"), 0);
        glUniform1f(glGetUniformLocation(vhsShader, "time"), currentFrame);
        glUniform1i(glGetUniformLocation(vhsShader, "filterType"), activeFilterType);

        glBindVertexArray(dummyVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window); glfwPollEvents();
    }

    glDeleteTextures(1, &screenTexture);
    glDeleteVertexArrays(1, &dummyVAO);
    glDeleteProgram(terrainShader); glDeleteProgram(skyboxShader); glDeleteProgram(vhsShader);
    glDeleteProgram(shadowShader);
    glfwTerminate(); return 0;
}