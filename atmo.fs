#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;

uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 atmoColor;
uniform float atmoThickness;
uniform vec3 viewPos;

void main() {
    vec3 norm = normalize(Normal);
    vec3 viewD = normalize(viewPos - FragPos);
    vec3 lightN = normalize(lightDir);
    
    float fresnel = 1.0 - max(dot(norm, viewD), 0.0);
    float alpha = pow(fresnel, atmoThickness);
    
    float diff = max(dot(norm, lightN), 0.0);
    
    float cosTheta = dot(viewD, -lightN);
    float rayleighPhase = 0.75 * (1.0 + cosTheta * cosTheta);
    float miePhase = 1.5 * ((1.0 - 0.95 * 0.95) / pow(1.0 + 0.95 * 0.95 - 2.0 * 0.95 * cosTheta, 1.5));
    float scattering = mix(rayleighPhase, miePhase, 0.4);

    float sunsetFactor = smoothstep(0.25, 0.0, dot(norm, lightN)) * smoothstep(-0.1, 0.15, dot(norm, lightN));
    vec3 sunsetColor = vec3(1.0, 0.35, 0.08)
    vec3 dynamicAtmoColor = mix(atmoColor, sunsetColor, sunsetFactor * 0.85);
    
    vec3 finalColor = dynamicAtmoColor * lightColor * (diff * 0.6 + 0.15) + (lightColor * scattering * alpha * 0.35);
    
    vec3 nightAtmo = vec3(0.01, 0.03, 0.08) * alpha * 0.2;
    finalColor = mix(nightAtmo, finalColor, smoothstep(-0.15, 0.15, dot(norm, lightN)));
    
    float finalAlpha = alpha * (smoothstep(-0.3, 0.5, dot(norm, lightN)) * 0.75 + 0.25);
    
    FragColor = vec4(pow(finalColor, vec3(1.0 / 2.2)), finalAlpha);
}