#version 330 core
layout (location = 0) in vec2 vPos;
layout (location = 1) in vec2 vUv;

out vec2 fVUv;

void main()
{
    gl_Position = vec4(vPos, 0.0, 1.0);
    fVUv = vUv;
}