#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in float Height;
in vec4 FragPosLightSpace;

uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform float planetRadius; 
uniform int planetType;

uniform vec3 flashLightPos;
uniform vec3 flashLightDir;
uniform float flashLightCutOff;
uniform float flashLightOuterCutOff;

uniform sampler2D shadowMap;

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if(projCoords.z > 1.0)
        return 0.0;
        
    float currentDepth = projCoords.z;
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);
    
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);

    vec2 noiseSeed = vec2(FragPos.x + FragPos.y, FragPos.z - FragPos.x) * 50.0;
    float noise = fract(sin(dot(noiseSeed, vec2(12.9898, 78.233))) * 43758.5453);

    float angle = noise * 6.2831853;
    float s = sin(angle);
    float c = cos(angle);
    mat2 rot = mat2(c, -s, s, c);

    vec2 poissonDisk[16] = vec2[](
       vec2( -0.94201624, -0.39906216 ), vec2( 0.94558609, -0.76890725 ),
       vec2( -0.094184101, -0.92938870 ), vec2( 0.34495938, 0.29387760 ),
       vec2( -0.91588581, 0.45771432 ), vec2( -0.81544232, -0.87912464 ),
       vec2( -0.38277543, 0.27676845 ), vec2( 0.97484398, 0.75648379 ),
       vec2( 0.44323325, -0.97511554 ), vec2( 0.53742981, -0.47373420 ),
       vec2( -0.26496911, -0.41893023 ), vec2( 0.79197514, 0.19090188 ),
       vec2( -0.24188840, 0.99706507 ), vec2( -0.81409955, 0.91437590 ),
       vec2( 0.19984126, 0.78641367 ), vec2( 0.14383161, -0.14100790 )
    );

    float blurRadius = 3.5; 

    for(int i = 0; i < 16; i++) {
        vec2 offset = rot * poissonDisk[i] * texelSize * blurRadius;
        float pcfDepth = texture(shadowMap, projCoords.xy + offset).r;
        
        shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
    }
    
    shadow /= 16.0;

    return shadow;
}

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

    float ambientStrength = 0.015;
    vec3 ambient = ambientStrength * vec3(0.2, 0.3, 0.8);
    
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
    
    float shadow = ShadowCalculation(FragPosLightSpace, norm, lightN);
    
    vec3 flashLightColor = vec3(1.0, 0.95, 0.8); 
    
    vec3 lightDirFlash = normalize(flashLightPos - FragPos);
    
    float theta = dot(lightDirFlash, normalize(-flashLightDir)); 
    
    float epsilon   = flashLightCutOff - flashLightOuterCutOff;
    float intensity = clamp((theta - flashLightOuterCutOff) / epsilon, 0.0, 1.0);
    
    float distance    = length(flashLightPos - FragPos);

    float constant    = 1.0;
    float linear      = 0.045;
    float quadratic   = 0.0075;
    float attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance));    
    
    float diffFlash = max(dot(norm, lightDirFlash), 0.0);
    vec3 flashDiffuse = diffFlash * flashLightColor * intensity * attenuation;

    vec3 lightingResult = (ambient + (1.0 - shadow) * (diffuse + specular) + flashDiffuse) * terrainColor;

    FragColor = vec4(pow(lightingResult, vec3(1.0 / 2.2)), 1.0);
}