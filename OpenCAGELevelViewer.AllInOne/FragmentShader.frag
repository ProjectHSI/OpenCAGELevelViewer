#version 430 core
layout(location = 0) in vec4 colour;
layout(location = 1) flat in uint instanceId;
layout(location = 2) flat in uint isRenderable;
//uniform int vertexColourMode;
uniform bool ignoreColW;
layout(location = 0) out vec4 FragColor;
layout(location = 1) out uint InstanceID;

void main()
{
    if (isRenderable == 0)
        discard;

    FragColor = colour;
    InstanceID = instanceId;

    /*if (vertexColourMode == 0){
        FragColor = vec4(1.0, 0.0, 0.0, 1.0);
        return;
    }*/

    if (ignoreColW) {
        FragColor.w = 1.0;
    } else {
        //if (FragColor.w != 0 && FragColor.w != 1) {
            //FragColor = vec4(1.0, 0.0, 1.0, 1.0);
            //FragColor = vec4((gl_FragCoord.x + 1) / 2, (gl_FragCoord.y + 1) / 2, 0.0, 1.0);
            //FragColor = vec4(int((gl_FragCoord.x + gl_FragCoord.y) * 100) % 2 == 0 ? 1.0 : 0.0, 0.0, int((gl_FragCoord.x + gl_FragCoord.y) * 100) % 2 == 0 ? 1.0 : 0.0, 1.0);
        //}
    }
} 