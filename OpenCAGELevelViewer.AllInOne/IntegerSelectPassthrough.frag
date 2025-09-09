#version 330 core
layout(location = 0) out vec4 FragColor;

in vec2 fVUv;

uniform usampler2D integerFramebufferTexture;

void main()
{
    FragColor = vec4(texture(integerFramebufferTexture, fVUv).r / 4294967295.0f, 0.0f, 0.0f, 1.0f);
} 