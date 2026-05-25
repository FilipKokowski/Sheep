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
        vec2 tex_offset = 1.0 / vec2(textureSize(screenTexture, 0));
        
        float s00 = dot(texture(screenTexture, uv + vec2(-tex_offset.x, -tex_offset.y)).rgb, vec3(0.299, 0.587, 0.114));
        float s10 = dot(texture(screenTexture, uv + vec2( 0.0,          -tex_offset.y)).rgb, vec3(0.299, 0.587, 0.114));
        float s20 = dot(texture(screenTexture, uv + vec2( tex_offset.x, -tex_offset.y)).rgb, vec3(0.299, 0.587, 0.114));
        
        float s01 = dot(texture(screenTexture, uv + vec2(-tex_offset.x,  0.0)).rgb, vec3(0.299, 0.587, 0.114));
        float s21 = dot(texture(screenTexture, uv + vec2( tex_offset.x,  0.0)).rgb, vec3(0.299, 0.587, 0.114));
        
        float s02 = dot(texture(screenTexture, uv + vec2(-tex_offset.x,  tex_offset.y)).rgb, vec3(0.299, 0.587, 0.114));
        float s12 = dot(texture(screenTexture, uv + vec2( 0.0,           tex_offset.y)).rgb, vec3(0.299, 0.587, 0.114));
        float s22 = dot(texture(screenTexture, uv + vec2( tex_offset.x,  tex_offset.y)).rgb, vec3(0.299, 0.587, 0.114));
        
        float gx = -s00 - 2.0 * s01 - s02 + s20 + 2.0 * s21 + s22;
        float gy = -s00 - 2.0 * s10 - s20 + s02 + 2.0 * s12 + s22;
        
        float edge = length(vec2(gx, gy));
        
        float sketchLine = 1.0 - smoothstep(0.05, 0.25, edge);
        
        float originalGray = dot(color, vec3(0.299, 0.587, 0.114));
        
        vec3 paperColor = vec3(0.95, 0.95, 0.90);
       
        vec3 coloredBase = mix(paperColor * (originalGray * 0.7 + 0.3), color, 0.5);
        
        vec3 finalSketch = mix(vec3(0.1), coloredBase, sketchLine);
        
        FragColor = vec4(finalSketch, 1.0);
    }

}