#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in float Height;

uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform float planetRadius; 
uniform int planetType;

void main() {
    float normHeight = Height / planetRadius;
    vec3 terrainColor = vec3(0.5);

    if (planetType == 0) {
        vec3 colorOcean   = vec3(0.05, 0.18, 0.40);
        vec3 colorShallow = vec3(0.10, 0.32, 0.60);
        vec3 colorBeach   = vec3(0.82, 0.76, 0.58);
        vec3 colorGrass   = vec3(0.22, 0.45, 0.22);
        vec3 colorRock    = vec3(0.40, 0.38, 0.36);
        vec3 colorSnow    = vec3(0.98, 0.98, 0.98);

        terrainColor = colorOcean;
        float wShallow = smoothstep(0.94, 0.97, normHeight);
        terrainColor = mix(terrainColor, colorShallow, wShallow);
        float wBeach = smoothstep(0.990, 1.000, normHeight);
        terrainColor = mix(terrainColor, colorBeach, wBeach);
        float wGrass = smoothstep(1.002, 1.012, normHeight);
        terrainColor = mix(terrainColor, colorGrass, wGrass);
        float wRock = smoothstep(1.040, 1.070, normHeight);
        terrainColor = mix(terrainColor, colorRock, wRock);
        float wSnow = smoothstep(1.090, 1.120, normHeight);
        terrainColor = mix(terrainColor, colorSnow, wSnow);
    } 
    else if (planetType == 1) {
        vec3 colorLowMaria = vec3(0.13, 0.13, 0.14); 
        vec3 colorHighlands = vec3(0.38, 0.38, 0.39); 
        vec3 colorCraters   = vec3(0.58, 0.58, 0.60); 

        float wHigh = smoothstep(0.98, 1.01, normHeight);
        terrainColor = mix(colorLowMaria, colorHighlands, wHigh);
        float wCrater = smoothstep(1.03, 1.07, normHeight);
        terrainColor = mix(terrainColor, colorCraters, wCrater);
    } 
    else if (planetType == 2) {
        vec3 colorLowPlains = vec3(0.35, 0.12, 0.04); 
        vec3 colorDesert    = vec3(0.68, 0.26, 0.10); 
        vec3 colorDunes     = vec3(0.80, 0.42, 0.22); 
        vec3 colorPolarIce  = vec3(0.92, 0.94, 0.97); 

        float wDes = smoothstep(0.95, 1.00, normHeight);
        terrainColor = mix(colorLowPlains, colorDesert, wDes);
        float wDune = smoothstep(1.03, 1.08, normHeight);
        terrainColor = mix(terrainColor, colorDunes, wDune);
        float wIce = smoothstep(1.09, 1.11, normHeight);
        terrainColor = mix(terrainColor, colorPolarIce, wIce);
    }
    else if (planetType == 3) {
        vec3 colorDeepPit  = vec3(0.18, 0.16, 0.15);
        vec3 colorDustyReg = vec3(0.32, 0.30, 0.28);
        vec3 colorHighEdge = vec3(0.48, 0.46, 0.44);

        float wReg = smoothstep(0.88, 0.96, normHeight);
        terrainColor = mix(colorDeepPit, colorDustyReg, wReg);
        float wEdge = smoothstep(1.02, 1.10, normHeight);
        terrainColor = mix(terrainColor, colorHighEdge, wEdge);
    }

    float ambientStrength = 0.14;
    vec3 ambient = ambientStrength * lightColor;
    
    vec3 norm = normalize(Normal);
    vec3 lightN = normalize(lightDir);
    float diff = max(dot(norm, lightN), 0.0);
    vec3 diffuse = diff * lightColor;
    
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightN + viewDir);
    
    float shininess = 8.0;
    float specStrength = 0.01;
    
    if (planetType == 0) {
        shininess = (normHeight < 1.0) ? 64.0 : 8.0;
        specStrength = (normHeight < 1.0) ? 0.6 : 0.01;
    }
    
    float spec = pow(max(dot(norm, halfwayDir), 0.0), shininess);
    vec3 specular = specStrength * spec * lightColor;
    
    vec3 lightingResult = (ambient + diffuse + specular) * terrainColor;
    
    FragColor = vec4(pow(lightingResult, vec3(1.0 / 2.2)), 1.0);
}