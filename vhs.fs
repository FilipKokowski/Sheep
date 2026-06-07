#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform float time;
uniform int filterType;

void main() {
    vec2 uv = TexCoords;
    vec3 color = texture(screenTexture, uv).rgb;

    if (filterType == 0) {
        FragColor = vec4(color, 1.0);
    }
    else if (filterType == 1) {
        float shift = sin(time * 10.0 + uv.y * 20.0) * 0.002;
        float r = texture(screenTexture, uv + vec2(shift, 0.0)).r;
        float g = texture(screenTexture, uv).g;
        float b = texture(screenTexture, uv - vec2(shift, 0.0)).b;

        vec3 vhsColor = vec3(r, g, b);
        float scanline = sin(uv.y * 800.0) * 0.04;

        vhsColor -= scanline;
        FragColor = vec4(vhsColor, 1.0);
    }
    else if (filterType == 2) {
        FragColor = vec4(1.0 - color, 1.0);
    }
    else if (filterType == 3) {
        float gray = dot(color, vec3(0.299, 0.587, 0.114));
        FragColor = vec4(vec3(gray), 1.0);
    }
    else if (filterType == 4) {
        float pixelSize = 4; 
    
        vec2 screenRes = textureSize(screenTexture, 0);
        vec2 pixelCoords = uv * screenRes;
          
        vec2 customPixelCoords = floor(pixelCoords / pixelSize) * pixelSize;
        vec2 pixelUV = customPixelCoords / screenRes;
    
        vec3 pixelColor = texture(screenTexture, pixelUV).rgb;
    
        FragColor = vec4(pixelColor, 1.0);
    }
}