#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

out vec2 TexCoords;
out vec4 FragPosLightSpace;
out vec3 FragPos;

void main() {
    TexCoords = aTexCoords;

    FragPos = vec3(model * vec4(aPos, 1.0)); 
    
    FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
    gl_Position = projection * view * vec4(FragPos, 1.0);
}