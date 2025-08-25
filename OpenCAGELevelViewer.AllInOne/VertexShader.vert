#version 330 core
layout (location = 0) in vec3 vPos;
layout (location = 1) in vec4 vCol;
layout (location = 2) in uint iId;
layout (location = 3) in vec3 iPos;
layout (location = 4) in vec3 iRot;
layout (location = 5) in vec4 iModelCol;
layout (location = 6) in vec4 iColOffset;

out vec4 colour;

const float PI = 3.1415926535897932384626433832795;
const float PI_180 = PI / 180;

//uniform mat4 model;
uniform int vertexColourMode;

uniform mat4 view;
uniform mat4 projection;

void main()
{
	//mat4 model = mat4(1.0);

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
	//mat4 worldMatrix = mat4(1.0);
	
	vec4 coreColour;

	switch (vertexColourMode) {
		case 0:
			coreColour = iModelCol;
			break;
		case 1:
			coreColour = vCol;
			break;
	}

	colour = coreColour * iColOffset;
	
	//if (coreColour == 0) {
		//coreColour = vec4(1.0, 0.0, 0.0, 1.0);
	//} else {
		//coreColour = vec4(0.0, 1.0, 0.0, 1.0);
	//}
	//colour = vec4(iPos.x, iPos.y, iPos.z, 1.0);

	gl_Position = projection * view * worldMatrix * vec4(vPos, 1.0);
}