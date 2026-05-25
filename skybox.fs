#version 330 core
precision highp float;

out vec4 FragColor;
in vec3 ViewDir;

uniform float time;
uniform vec3 lightDir;
uniform vec3 sunColor;

float hash(vec3 p) {
    p = fract(p * vec3(0.1031, 0.1030, 0.0973));
    p += dot(p, p.zyx + 31.45);
    return fract((p.x + p.y) * p.z);
}

float noise3D(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    
    vec3 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);
    
    float s000 = hash(i + vec3(0.0, 0.0, 0.0));
    float s100 = hash(i + vec3(1.0, 0.0, 0.0));
    float s010 = hash(i + vec3(0.0, 1.0, 0.0));
    float s110 = hash(i + vec3(1.0, 1.0, 0.0));
    float s001 = hash(i + vec3(0.0, 0.0, 1.0));
    float s101 = hash(i + vec3(1.0, 0.0, 1.0));
    float s011 = hash(i + vec3(0.0, 1.0, 1.0));
    float s111 = hash(i + vec3(1.0, 1.0, 1.0));
    
    float x1 = mix(s000, s100, u.x);
    float x2 = mix(s010, s110, u.x);
    float x3 = mix(s001, s101, u.x);
    float x4 = mix(s011, s111, u.x);
    
    float y1 = mix(x1, x2, u.y);
    float y2 = mix(x3, x4, u.y);
    
    return mix(y1, y2, u.z);
}

float fbm(vec3 p) {
    float value = 0.0;
    float amplitude = 0.5;
    
    vec3 shift = vec3(0.115, 0.123, 0.111); 
    
    for (int i = 0; i < 4; ++i) {
        value += amplitude * noise3D(p);
        
        p = p.yzx * 2.01 + shift;
        amplitude *= 0.5;
    }
    return value;
}

void main() {
    vec3 viewDir = normalize(ViewDir);
    vec3 lightN = normalize(lightDir);
    
    float sunDot = max(dot(viewDir, lightN), 0.0);
    float sunDisc = smoothstep(0.9992, 0.9995, sunDot);
    float sunGlow = pow(sunDot, 256.0) * 1.2 + pow(sunDot, 32.0) * 0.2;
    vec3 sunElement = (sunDisc + sunGlow) * sunColor;
    
    vec3 starGrid = floor(viewDir * 300.0);
    float starWeight = hash(starGrid);
    float starIntensity = step(0.994, starWeight); 
    vec3 stars = vec3(starIntensity * hash(starGrid + vec3(2.0)));
    
    stars *= (1.0 - step(0.98, sunDot));
    
    vec3 noisePos = viewDir * 3.5 + vec3(sin(time * 0.01), time * 0.005, cos(time * 0.01));
    float n = fbm(noisePos);
    vec3 nebulaColor1 = vec3(0.18, 0.04, 0.35); 
    vec3 nebulaColor2 = vec3(0.02, 0.15, 0.25); 
    vec3 nebula = mix(nebulaColor1, nebulaColor2, n) * pow(n, 2.2) * 0.5;
    
    vec3 spaceVacuum = vec3(0.002, 0.002, 0.006);
    
    vec3 finalColor = stars + nebula + spaceVacuum + sunElement;
    
    FragColor = vec4(pow(finalColor, vec3(1.0 / 2.2)), 1.0);
}