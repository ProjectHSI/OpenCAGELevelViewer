#version 430 core
layout (location = 0) in vec3 vPos;
layout (location = 1) in vec4 vCol;
layout (location = 2) in uint iId;
layout (location = 3) in uint iIsRenderable;
layout (location = 4) in mat4 iMat;
layout (location = 8) in vec4 iModelCol;
layout (location = 9) in vec4 iColOffset;

layout (location = 0) out vec4 colour;
layout (location = 1) flat out uint instanceId;
layout (location = 2) flat out uint isRenderable;

const float PI = 3.1415926535897932384626433832795;
const float PI_180 = PI / 180;

//uniform mat4 model;
uniform int vertexColourMode;

uniform mat4 view;
uniform mat4 projection;

uint hash32(uint x)
{
    x ^= x >> 17;
    x *= 0xed5ad4bbU;
    x ^= x >> 11;
    x *= 0xac4c1b51U;
    x ^= x >> 15;
    x *= 0x31848babU;
    x ^= x >> 14;
    return x;
}

uint truncate(uint x) {
	uint w = x & 0xFFU;
	
	x >>= 8;
	x ^= w;
	x ^= w << 8;
	x ^= w << 16;

	return x;
}

void main()
{
	//mat4 model = mat4(1.0);

	/*
	float iRotXRad = iRot.x * PI_180;
	mat4 worldMatrixXRot  = mat4(1.0);

	worldMatrixXRot[1][1] = cos(iRotXRad);
	worldMatrixXRot[1][2] = -sin(iRotXRad);
	worldMatrixXRot[2][1] = sin(iRotXRad);
	worldMatrixXRot[2][2] = cos(iRotXRad);

	float iRotYRad = iRot.x * PI_180;
	mat4 worldMatrixYRot  = mat4(1.0);

	worldMatrixYRot[0][0] = cos(iRotYRad);
	worldMatrixYRot[0][2] = sin(iRotYRad);
	worldMatrixYRot[2][0] = -sin(iRotYRad);
	worldMatrixYRot[2][2] = cos(iRotYRad);

	float iRotZRad = iRot.x * PI_180;
	mat4 worldMatrixZRot  = mat4(1.0);

	worldMatrixZRot[0][0] = cos(iRotZRad);
	worldMatrixZRot[0][1] = -sin(iRotZRad);
	worldMatrixZRot[1][0] = sin(iRotZRad);
	worldMatrixZRot[1][1] = cos(iRotZRad);

	mat4 worldMatrixRot = worldMatrixXRot * worldMatrixYRot * worldMatrixZRot;


	mat4 worldMatrixTrans = mat4(1.0);
	worldMatrixTrans[3][0] = iPos.x;
	worldMatrixTrans[3][1] = iPos.y;
	worldMatrixTrans[3][2] = iPos.z;


	mat4 worldMatrix = worldMatrixTrans * worldMatrixRot;
	*/
	//mat4 worldMatrix = mat4(1.0);
	
	vec4 coreColour;

	switch (vertexColourMode) {
		case 0:
			coreColour = iModelCol;
			break;
		case 1:
			coreColour = vCol;
			break;
		case 2:
			{
				//uint instanceColourUint = truncate(hash32(iId));
				//coreColour = vec4(((instanceColourUint >> 16) & uint(0xFF)) / 255.0f, ((instanceColourUint >> 8) & uint(0xFF)) / 255.0f, ((instanceColourUint >> 0) & uint(0xFF)) / 255.0f, 1.0);

				coreColour = vec4(iId / 4294967295.0f, 0.0, 0.0, 1.0);
			}
			break;
		case 3:
			{
				const float positionDiv = 32;
				coreColour = vec4((iMat[3][0] / positionDiv + 1) / 2 + 1, iMat[3][1] / positionDiv + 0.5, iMat[3][2] / positionDiv + 0.5, 1.0);
			}
			break;
	}

	colour = coreColour * iColOffset;
	instanceId = iId;
	isRenderable = iIsRenderable;
	
	//if (coreColour == 0) {
		//coreColour = vec4(1.0, 0.0, 0.0, 1.0);
	//} else {
		//coreColour = vec4(0.0, 1.0, 0.0, 1.0);
	//}
	//colour = vec4(iPos.x, iPos.y, iPos.z, 1.0);

	gl_Position = projection * view * iMat * vec4(vPos, 1.0);
}