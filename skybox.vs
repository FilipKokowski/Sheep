#version 330 core

uniform mat4 invViewProj;
out vec3 ViewDir;

void main() {
    float x = -1.0 + float((gl_VertexID & 1) << 2);
    float y = -1.0 + float((gl_VertexID & 2) << 1);
    
    gl_Position = vec4(x, y, 1.0, 1.0);
    
    vec4 unprojected = invViewProj * vec4(x, y, 1.0, 1.0);
    ViewDir = unprojected.xyz / unprojected.w;
}