#include <string_view>
#include <glbinding/gl/types.h>
#pragma once

namespace OpenCAGELevelViewer {
	namespace _3DView {
		namespace Shader {
			class Shader {
			public:
				Shader(std::string_view vertexShader, std::string_view fragmentShader);
				~Shader();

				gl::GLuint getShaderProgram();
			};
		}
	}
}