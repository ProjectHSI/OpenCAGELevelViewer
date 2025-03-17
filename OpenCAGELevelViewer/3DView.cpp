#include "3DView.h"
#include <imgui.h>

#include <glbinding/CallbackMask.h>
#include <glbinding/FunctionCall.h>
#include <glbinding/glbinding.h>
#include <glbinding/Version.h>

#include <glbinding/getProcAddress.h>
#include <glbinding/gl/gl.h>

#include <ContentManager.h>
#include <cstdint>
#include <fstream>
#include <glbinding/gl/gl.h>
#include <iostream>
#include <optional>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace gl;

float testTriangleVertices[] = {
	-0.5f, -0.5f, 0.0f,
	 0.5f, -0.5f, 0.0f,
	 0.0f,  0.5f, 0.0f
};

float testTriangleIndices[] = {
	0, 1, 2
};

//const char *vertexShader = "#version 330 core\n"
//    "layout (location = 0) in vec3 aPos;\n"
//    "void main()\n"
//    "{\n"
//    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
//	"}";
//
//const char *fragmentShader = "#version 330 core\n"
//    "out vec4 FragColor;\n"
//    "void main()\n"
//    "{\n"
//    "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
//	"}";

unsigned int _3dViewRenderFbo;
unsigned int _3dViewRenderFboTextureColorbuffer;
unsigned int _3dViewRenderRbo;

unsigned int _3dViewOutputFbo;
unsigned int _3dViewOutputFboTextureColorbuffer;

//unsigned int _3dViewTestTriangleVbo;
//unsigned int _3dViewTestTriangleEbo;
//unsigned int _3dViewTestTriangleVao;
GLuint _3dViewAxisArrowVao;
GLuint _3dViewAxisArrowVbo;
GLuint _3dViewAxisArrowEbo;
GLuint _3dViewAxisArrowIndicesCount;
unsigned int _3dViewVertexShader;
unsigned int _3dViewFragmentShader;
unsigned int _3dViewShaderProgram;

glm::vec3 cameraPos = glm::vec3(5.0f, 0.0f, 50.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

static Assimp::Importer import;

//static std::array<glm::mat4, 3> axisArrowMatricies = {glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f))}

unsigned int OpenCAGELevelViewer::_3DView::getFbo(void) {
	return _3dViewRenderFboTextureColorbuffer;
}

static void glDebugOutput(GLenum source,
							GLenum type,
							unsigned int id,
							GLenum severity,
							GLsizei length,
							const char *message,
							const void *userParam) {
	// ignore non-significant error/warning codes
	if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

	std::cout << "---------------" << std::endl;
	std::cout << "Debug message (" << id << "): " << message << std::endl;

	switch (source) {
		case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
		case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
		case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
	} std::cout << std::endl;

	switch (type) {
		case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
		case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
		case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
		case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
		case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
		case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
		case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
	} std::cout << std::endl;

	switch (severity) {
		case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
		case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
		case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
		case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
	} std::cout << std::endl;
	std::cout << std::endl;
}

void OpenCAGELevelViewer::_3DView::updateCamera(int32_t x, int32_t y) {

}

void OpenCAGELevelViewer::_3DView::updateCamera(signed char x, signed char y, signed char z, signed char roll) {
	cameraPos += (5.0f * cameraFront * static_cast< float >(x));
	cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * 5.0f * static_cast< float >(y);
}

//ImVec2 previousWindowSize;
//glm::mat4 projection = glm::perspective(glm::radians(45.0f), ( float ) SCR_WIDTH / ( float ) SCR_HEIGHT, 0.1f, 100.0f);
//
//static void updateSizeSpecificVariablesWhenNeeded(ImVec2 windowSize) {
//	if (windowSize != previousWindowSize) {
//
//	}
//}

void OpenCAGELevelViewer::_3DView::Initalise(void) {
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

	glDebugMessageCallback(glDebugOutput, nullptr);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

#pragma region Framebuffer Initalisation
	// OpenGL requires us to pass a resolution for the render buffer and texture.
	// We use 800x800 here, but it'll be overriden during rendering anyway.

	glGenFramebuffers(1, &_3dViewRenderFbo);
	glGenFramebuffers(1, &_3dViewOutputFbo);
	glBindFramebuffer(GL_FRAMEBUFFER, _3dViewRenderFbo);

	glGenTextures(1, &_3dViewRenderFboTextureColorbuffer);
	glBindTexture(GL_TEXTURE_2D, _3dViewRenderFboTextureColorbuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 800, 800, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _3dViewRenderFboTextureColorbuffer, 0);

	glGenRenderbuffers(1, &_3dViewRenderRbo);
	glBindRenderbuffer(GL_RENDERBUFFER, _3dViewRenderRbo);
	//glRenderbufferStorageMultisample(GL_RENDERBUFFER, 1, GL_DEPTH24_STENCIL8, 800, 800); // use a single renderbuffer object for both a depth AND stencil buffer.
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 800, 800);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _3dViewRenderRbo);

	/*glBindFramebuffer(GL_FRAMEBUFFER, _3dViewOutputFbo);
	glGenTextures(1, &_3dViewOutputFboTextureColorbuffer);
	glBindTexture(GL_TEXTURE_2D, _3dViewOutputFboTextureColorbuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 800, 800, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _3dViewOutputFboTextureColorbuffer, 0);*/
#pragma endregion

#pragma region Shader Setup
	std::ifstream vertexShaderStream("VertexShader.vert");
	std::stringstream vertexShader;
	vertexShader << vertexShaderStream.rdbuf();
	std::string vertexShaderString = vertexShader.str();
	const char *vertexShaderCstring = vertexShaderString.c_str();

	_3dViewVertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(_3dViewVertexShader, 1, (&vertexShaderCstring), NULL);
	glCompileShader(_3dViewVertexShader);

	{
		int success;
		glGetShaderiv(_3dViewVertexShader, GL_COMPILE_STATUS, &success);
		if (!success) {
			int length = 0;
			glGetShaderInfoLog(_3dViewVertexShader, 0, &length, nullptr);

			std::string infoLog(length, '\0');
			glGetShaderInfoLog(_3dViewVertexShader, length, nullptr, infoLog.data());

			std::cout << infoLog << std::endl;
		}
	}

	std::ifstream fragmentShaderStream("FragmentShader.frag");
	std::stringstream fragmentShader;
	fragmentShader << fragmentShaderStream.rdbuf();
	std::string fragmentShaderString = fragmentShader.str();
	const char *fragmentShaderCstring = fragmentShaderString.c_str();

	_3dViewFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(_3dViewFragmentShader, 1, &fragmentShaderCstring, NULL);
	glCompileShader(_3dViewFragmentShader);

	{
		int success;
		glGetShaderiv(_3dViewFragmentShader, GL_COMPILE_STATUS, &success);
		if (!success) {
			int length = 0;
			glGetShaderInfoLog(_3dViewFragmentShader, 0, &length, nullptr);

			std::string infoLog(length, '\0');
			glGetShaderInfoLog(_3dViewFragmentShader, length, nullptr, infoLog.data());

			std::cout << infoLog << std::endl;
		}
	}

	_3dViewShaderProgram = glCreateProgram();
	glAttachShader(_3dViewShaderProgram, _3dViewVertexShader);
	glAttachShader(_3dViewShaderProgram, _3dViewFragmentShader);
	glLinkProgram(_3dViewShaderProgram);

	{
		int success;
		glGetProgramiv(_3dViewShaderProgram, GL_LINK_STATUS, &success);
		if (!success) {
			int length = 0;
			glGetShaderInfoLog(_3dViewShaderProgram, 0, &length, nullptr);

			std::string infoLog(length, '\0');
			glGetShaderInfoLog(_3dViewShaderProgram, length, nullptr, infoLog.data());

			std::cout << infoLog << std::endl;
		}
	}

	glDeleteShader(_3dViewVertexShader);
	glDeleteShader(_3dViewFragmentShader);
#pragma endregion

#pragma region Triangle Test Setup
	/*glGenVertexArrays(1, &_3dViewTestTriangleVao);
	glGenBuffers(1, &_3dViewTestTriangleVbo);
	glGenBuffers(1, &_3dViewTestTriangleEbo);

	glBindVertexArray(_3dViewTestTriangleVao);
	glBindBuffer(GL_ARRAY_BUFFER, _3dViewTestTriangleVbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(testTriangleVertices), testTriangleVertices, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _3dViewTestTriangleEbo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(testTriangleIndices), testTriangleIndices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), ( void * ) 0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);*/
#pragma endregion

#pragma region Setup Axis Arrow
	const aiScene *scene = import.ReadFile("AxisArrow.obj", aiProcess_Triangulate);

#pragma region Fragment Shaders
	/*std::ifstream vertexShaderStream("VertexShader.vert");
	std::stringstream vertexShader;
	vertexShader << vertexShaderStream.rdbuf();
	std::string vertexShaderString = vertexShader.str();
	const char *vertexShaderCstring = vertexShaderString.c_str();

	_3dViewVertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(_3dViewVertexShader, 1, (&vertexShaderCstring), NULL);
	glCompileShader(_3dViewVertexShader);

	{
		int success;
		glGetShaderiv(_3dViewVertexShader, GL_COMPILE_STATUS, &success);
		if (!success) {
			int length = 0;
			glGetShaderInfoLog(_3dViewVertexShader, 0, &length, nullptr);

			std::string infoLog(length, '\0');
			glGetShaderInfoLog(_3dViewVertexShader, length, nullptr, infoLog.data());

			std::cout << infoLog << std::endl;
		}
	}*/
#pragma endregion

#pragma region OpenGL Buffers
	glGenVertexArrays(1, &_3dViewAxisArrowVao);
	glGenBuffers(1, &_3dViewAxisArrowVbo);
	glGenBuffers(1, &_3dViewAxisArrowEbo);

	glBindVertexArray(_3dViewAxisArrowVao);
	
	glBindBuffer(GL_ARRAY_BUFFER, _3dViewAxisArrowVbo);
	//auto x = scene->mMeshes[0]->mVertices[0].x;
	glBufferData(GL_ARRAY_BUFFER, scene->mMeshes[0]->mNumVertices * sizeof(ai_real) * 3, scene->mMeshes[0]->mVertices, GL_STATIC_DRAW);

	std::vector<unsigned int> axisArrowIndices;

	for (size_t i = 0; i < scene->mMeshes[0]->mNumFaces; i++) {
		for (size_t v = 0; v < scene->mMeshes[0]->mFaces[i].mNumIndices; v++) {
			axisArrowIndices.push_back(scene->mMeshes[0]->mFaces[i].mIndices[v]);
		}
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _3dViewAxisArrowEbo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, axisArrowIndices.size() * sizeof(unsigned int), axisArrowIndices.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), ( void * ) 0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);

	_3dViewAxisArrowIndicesCount = axisArrowIndices.size();
	//glBufferData(GL_ARRAY_BUFFER, sizeof())
#pragma endregion
#pragma endregion

	glEnable(GL_DEPTH_TEST);
}

#pragma region Scene Management
struct SceneObject {
	GLuint vertexArrayObject;

	GLuint vertexBufferObject;
	GLuint elementBufferObject;

	GLuint shader;
	OpenCAGELevelViewer::ContentManager::Transform absoluteTransform; // This shouldn't have any parent - this should be the absolute transform of the object.
};

void OpenCAGELevelViewer::_3DView::UpdateScene(std::optional<std::variant<OpenCAGELevelViewer::ContentManager::UnmanagedModelReference, OpenCAGELevelViewer::ContentManager::UnmanagedComposite>> unmanagedObject) {

}
#pragma endregion

static void renderUnmanagedModelStorage(OpenCAGELevelViewer::ContentManager::UnmanagedModelReference::ModelStorage modelStorage) {

}

static void renderUnmanagedModelReference(OpenCAGELevelViewer::ContentManager::UnmanagedModelReference unmanangedModelReference) {

}

void OpenCAGELevelViewer::_3DView::Render(ImVec2 windowSize, std::optional<std::variant<OpenCAGELevelViewer::ContentManager::UnmanagedModelReference, OpenCAGELevelViewer::ContentManager::UnmanagedComposite>> unmanagedModelReference/*, int msaaSamples*/) {
#pragma region Rendering Start
	glBindFramebuffer(GL_FRAMEBUFFER, _3dViewRenderFbo);

	// update the texture size to correspond to the window
	glBindTexture(GL_TEXTURE_2D, _3dViewRenderFboTextureColorbuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, static_cast< GLsizei >(windowSize.x), static_cast< GLsizei >(windowSize.y), 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	//glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, pow(2, msaaSamples), GL_RGB, static_cast< GLsizei >(windowSize.x), static_cast< GLsizei >(windowSize.y), GL_TRUE);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, static_cast< GLsizei >(windowSize.x), static_cast< GLsizei >(windowSize.y));
	//glRenderbufferStorageMultisample(GL_RENDERBUFFER, 1, GL_DEPTH24_STENCIL8, static_cast< GLsizei >(windowSize.x), static_cast< GLsizei >(windowSize.y));

	//glBindFramebuffer(GL_FRAMEBUFFER, _3dViewOutputFbo);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, static_cast< GLsizei >(windowSize.x), static_cast< GLsizei >(windowSize.y), 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

	glBindFramebuffer(GL_FRAMEBUFFER, _3dViewRenderFbo);

	//glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glViewport(0, 0, static_cast< GLsizei >(windowSize.x), static_cast< GLsizei >(windowSize.y));
	//glViewport(0, 0, 1600, 1200);

	glClearColor(0.2f, 0.3f, 0.3f, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 projection = glm::perspective(glm::radians(45.0f), ( float ) windowSize.x / ( float ) windowSize.y, 0.1f, 100.0f);
	glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
#pragma endregion

#pragma region Rendering
	//if (unmanagedModelReference.has_value()) {
	#pragma region Old
		//glBindVertexArray(_3dViewTestTriangleVao);

		////glDrawArrays(GL_TRIANGLES, 0, 3);
		//glBindBuffer(GL_ARRAY_BUFFER, _3dViewTestTriangleVbo);
		//glBufferData(GL_ARRAY_BUFFER, unmanagedModelReference->model->vertices.size() * sizeof(float), unmanagedModelReference->model->vertices.data(), GL_DYNAMIC_DRAW);

		//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _3dViewTestTriangleEbo);
		//glBufferData(GL_ELEMENT_ARRAY_BUFFER, unmanagedModelReference->model->indices.size() * sizeof(uint16_t), unmanagedModelReference->model->indices.data(), GL_STATIC_DRAW);

		//glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), ( void * ) 0);
		//glEnableVertexAttribArray(0);

		//glBindVertexArray(0);
		//glBindBuffer(GL_ARRAY_BUFFER, 0);
		//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		//glUseProgram(_3dViewShaderProgram);
		//glBindVertexArray(_3dViewTestTriangleVao);

		//glDrawElements(GL_TRIANGLES, unmanagedModelReference->model->indices.size(), GL_UNSIGNED_INT, 0);
	#pragma endregion

#pragma region Drawing Axis
	glBindVertexArray(_3dViewAxisArrowVao);

	glUseProgram(_3dViewShaderProgram);
	glUniformMatrix4fv(glGetUniformLocation(_3dViewShaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(_3dViewShaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(_3dViewShaderProgram, "model"), 1, GL_FALSE, &(glm::mat4(1.0f))[0][0]);

	glDrawElements(GL_TRIANGLES, _3dViewAxisArrowIndicesCount, GL_UNSIGNED_INT, 0);

	//glBindVertexArray(0);

	//glm::mat4 axisModel = glm::mat4(1.0f);
	//model = glm::rotate(model, glm::radians(0), glm::vec3(1.0f, 0.0f, 0.0f));
#pragma endregion
	//}
#pragma endregion

#pragma region Rendering End
	//glBindFramebuffer(GL_READ_FRAMEBUFFER, _3dViewRenderFbo);
	//glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _3dViewOutputFbo);
	//glBlitFramebuffer(0, 0, static_cast< GLsizei >(windowSize.x), static_cast< GLsizei >(windowSize.y), 0, 0, static_cast< GLsizei >(windowSize.x), static_cast< GLsizei >(windowSize.y), GL_COLOR_BUFFER_BIT, GL_NEAREST);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#pragma endregion
}

void OpenCAGELevelViewer::_3DView::Quit(void) {
	glDeleteProgram(_3dViewShaderProgram);
	//glDeleteVertexArrays(1, &_3dViewTestTriangleVao);
	//glDeleteBuffers(1, &_3dViewTestTriangleVbo);

	glDeleteFramebuffers(1, &_3dViewRenderFbo);
	glDeleteRenderbuffers(1, &_3dViewRenderRbo);
	glDeleteTextures(1, &_3dViewRenderFboTextureColorbuffer);

	glDeleteFramebuffers(1, &_3dViewOutputFbo);
	glDeleteTextures(1, &_3dViewOutputFboTextureColorbuffer);
}
