#version 330 core
out vec4 FragColor;

flat in int axisPlaneID;
flat in uint enabledAxisPlanesForFrag;
in vec2 uv;

const float checkboardEpsilon = 0.1 / 2;
const vec3 checkerboardBorderColour = vec3(0.0f, 0.0f, 0.0f);

void main()
{
	//if (enabledAxisPlanesForFrag >> axisPlaneID - 1 == 0U) {
		//discard;
		//return;
	//}

	
	vec3 checkerboardFiller;

	switch (axisPlaneID) {
		case 1:
			checkerboardFiller = vec3(uv.x, uv.y, 0.0);
			break;
		case 2:
			checkerboardFiller = vec3(uv.x, 0.0, uv.y);
			break;
		case 3:
			checkerboardFiller = vec3(0.0, uv.x, uv.y);
			break;
	}

	float squareDistance = (max(uv.x, 0.5f) - min(uv.x, 0.5f)) - (max(uv.y, 0.5f) - min(uv.y, 0.5f));
	bool isInCheckboardBorder = mod(squareDistance, 1.0) <= checkboardEpsilon || 1 - mod(squareDistance, 1.0) >= checkboardEpsilon;

	vec3 colour = isInCheckboardBorder ? checkerboardBorderColour : checkerboardFiller;

	//FragColor = vec4(colour.xyz, 1.0f);

	FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
}