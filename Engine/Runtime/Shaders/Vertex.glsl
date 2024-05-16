#version 460

layout(location = 0) in vec2 vertexPosition;
layout(location = 1) in vec4 color;

out vec4 fragColor;

uniform float time;

void main() {
    fragColor = color + vec4(sin(time) * 0.5, cos(time) * 0.5, 0.0, 0.0);
    gl_Position = vec4(vertexPosition, 0.0, 1.0);
}