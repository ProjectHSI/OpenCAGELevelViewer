#version 330 core
layout (location = 0) in vec3 vPos;
layout (location = 1) in vec4 vCol;
layout (location = 2) in uint iId;
layout (location = 3) in vec3 iPos;
layout (location = 4) in vec3 iRot;
layout (location = 5) in vec4 iColOffset;

out vec4 colour;

const float PI = 3.1415926535897932384626433832795;
const float PI_180 = PI / 180;

//uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	mat4 model = mat4(1.0);

	float iRotXRad = iRot.x * PI_180;
	mat4 worldMatrixXRot  = mat4x4(1, 0,             0,              0,
								   0, cos(iRotXRad), -sin(iRotXRad), 0,
								   0, sin(iRotXRad), cos(iRotXRad),  0,
								   0, 0,             0,              1);

	float iRotYRad = iRot.x * PI_180;
	mat4 worldMatrixYRot  = mat4x4(cos(iRotYRad),  0, sin(iRotYRad), 0,
								   0,              1, 0,             0,
								   -sin(iRotYRad), 0, cos(iRotYRad), 0,
								   0,              0, 0,             1);

	float iRotZRad = iRot.x * PI_180;
	mat4 worldMatrixZRot  = mat4x4(-sin(iRotZRad), -sin(iRotZRad), 0, 0,
								   cos(iRotZRad),  cos(iRotZRad),  0, 0,
								   0,              0,              1, 0,
								   0,              0,              0, 1);

	mat4 worldMatrixRot = worldMatrixXRot * worldMatrixYRot * worldMatrixZRot;

	mat4 worldMatrixTrans = mat4x4(1, 0, 0, iPos.x,
								   0, 1, 0, iPos.y,
								   0, 0, 1, iPos.z,
								   0, 0, 0, 1     );

	mat4 worldMatrix = worldMatrixTrans * worldMatrixRot;
	//mat4 worldMatrix = mat4(1.0);
	
	colour = vCol * iColOffset;

	gl_Position = projection * view * worldMatrix * vec4(vPos, 1.0);
}