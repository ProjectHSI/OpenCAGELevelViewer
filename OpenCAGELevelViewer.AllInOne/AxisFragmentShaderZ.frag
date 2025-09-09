#version 330 core
layout(location = 0) out vec4 FragColor;
layout(location = 1) out uint RedBufferOutput;

void main()
{
    FragColor = vec4(0.0f, 0.0f, 1.0f, 0.2f);
    RedBufferOutput = 4294967295u;
} 