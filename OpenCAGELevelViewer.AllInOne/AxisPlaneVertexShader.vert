#version 330 core
layout (location = 0) in vec2 vFaux; // do not use, garbage
//layout (location = 0) in vec2 vUv;
//layout (location = 1) in vec3 vCol;
//uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float axisPlaneSize;
uniform uint enabledAxisPlanes; // bit field

flat out int axisPlaneID;
flat out uint enabledAxisPlanesForFrag;
out vec2 uv;

float axisPlaneSizeHalf = axisPlaneSize / 2;

const vec2[4] uvs = vec2[4](vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(1.0, 1.0), vec2(0.0, 1.0));

void main()
{
	vec4[4] corners = vec4[4](vec4(1.0), vec4(1.0), vec4(1.0), vec4(1.0));

	// 1 = XY, 2 = XZ, 3 = YZ
	switch (gl_InstanceID) {
		case 1:
			// XY
			corners = vec4[4](
				vec4(-axisPlaneSizeHalf, axisPlaneSizeHalf, 0, 0),
				vec4(axisPlaneSizeHalf, axisPlaneSizeHalf, 0, 0),
				vec4(axisPlaneSizeHalf, -axisPlaneSizeHalf, 0, 0),
				vec4(-axisPlaneSizeHalf, -axisPlaneSizeHalf, 0, 0)
			);
			break;
		case 2:
			// XZ
			corners = vec4[4](
				vec4(-axisPlaneSizeHalf, 0, axisPlaneSizeHalf, 0),
				vec4(axisPlaneSizeHalf, 0, axisPlaneSizeHalf, 0),
				vec4(axisPlaneSizeHalf, 0, -axisPlaneSizeHalf, 0),
				vec4(-axisPlaneSizeHalf, 0, -axisPlaneSizeHalf, 0)
			);
			break;
		case 3:
			// YZ
			corners = vec4[4](
				vec4(0, -axisPlaneSizeHalf, axisPlaneSizeHalf, 0),
				vec4(0, axisPlaneSizeHalf, axisPlaneSizeHalf, 0),
				vec4(0, axisPlaneSizeHalf, -axisPlaneSizeHalf, 0),
				vec4(0, -axisPlaneSizeHalf, -axisPlaneSizeHalf, 0)
			);
			break;
	}

	uv = uvs[gl_VertexID];
	axisPlaneID = gl_InstanceID;
	enabledAxisPlanesForFrag = enabledAxisPlanes;
	gl_Position = corners[gl_VertexID];
	//gl_Position = vec4(corners[gl_VertexID].xy, 0.0, 0.0);
}