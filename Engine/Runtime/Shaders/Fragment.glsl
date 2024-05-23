#version 460

in vec4 fragPosition;
in vec4 fragColor;
in vec2 fragUV;

out vec4 color;

uniform sampler2D sampler;

void main() {
    color = texture(sampler, fragUV) * fragColor;
}