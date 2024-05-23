#version 460

layout(location = 0) in vec2 vertexPosition;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 vertexUV;

out vec4 fragPosition;
out vec4 fragColor;
out vec2 fragUV;

uniform float time;


void main() {
    fragPosition = vec4(vertexPosition, 0.0, 1.0);
    // fragColor = color + vec4(sin(time) * 0.5, cos(time) * 0.5, 0.0, 0.0);
    fragColor = color;
    fragUV = vec2(vertexUV.x, 1.0 - vertexUV.y);
    gl_Position = fragPosition;
}