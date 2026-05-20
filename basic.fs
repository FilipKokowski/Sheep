#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;

uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 objectColor;
uniform vec3 viewPos;

void main() {
    float ambientStrength = 0.15;
    vec3 ambient = ambientStrength * lightColor;
    
    vec3 norm = normalize(Normal);
    vec3 lightN = normalize(lightDir);
    float diff = max(dot(norm, lightN), 0.0);
    vec3 diffuse = diff * lightColor;
    
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightN + viewDir);
    
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
    float specularStrength = 0.5;
    vec3 specular = specularStrength * spec * lightColor;
    
    vec3 lightingResult = (ambient + diffuse + specular) * objectColor;
    
    FragColor = vec4(pow(lightingResult, vec3(1.0 / 2.2)), 1.0);
}