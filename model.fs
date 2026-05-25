#version 330 core
in vec2 TexCoords;
in vec4 FragPosLightSpace;
in vec3 FragPos;

out vec4 FragColor;

uniform sampler2D tex;
uniform sampler2D shadowMap;

uniform vec3 flashLightPos;       
uniform vec3 flashLightDir;       
uniform float flashLightCutOff;      
uniform float flashLightOuterCutOff; 

float ShadowCalculation(vec4 fragPosLightSpace) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if(projCoords.z > 1.0) return 0.0;
        
    float currentDepth = projCoords.z;
    float bias = 0.002;
    
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
    vec3 texColor = texture(tex, TexCoords).rgb;
    float shadow = ShadowCalculation(FragPosLightSpace);
    
    vec3 ambient = texColor * vec3(0.2, 0.3, 0.8) * 0.015; 
    vec3 sunLight = texColor * (1.0 - shadow); 
    
    vec3 baseLighting = ambient + sunLight;

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
    
    vec3 flashDiffuse = flashLightColor * intensity * attenuation * texColor;

    vec3 finalColor = baseLighting + flashDiffuse;
    
    FragColor = vec4(finalColor, 1.0);
}