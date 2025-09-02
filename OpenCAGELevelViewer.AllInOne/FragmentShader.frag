#version 330 core
in vec4 colour;
//uniform int vertexColourMode;
uniform bool ignoreColW;
out vec4 FragColor;

void main()
{
    FragColor = colour;

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