#include "pch.h"

#include "3DView.h"
#include <imgui.h>

#include "ContentManager.h"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <optional>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <exception>
#include <map>
#include <variant>

#include <filesystem>

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

glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

//glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
//glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

static Assimp::Importer import;

//static std::array<glm::mat4, 3> axisArrowMatricies = {glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f))}

unsigned int OpenCAGELevelViewer::_3DView::getFbo(void) {
	return _3dViewRenderFboTextureColorbuffer;
}

static void GL_APIENTRY glDebugOutput(GLenum source,
							GLenum type,
							GLuint id,
							GLenum severity,
							GLsizei length,
							const GLchar *message,
							const void *userParam) {
	// ignore non-significant error/warning codes
	if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

	std::cout << "---------------" << std::endl;
	std::cout << "Debug message (" << id << "): " << message << std::endl;

#pragma warning(disable: 4062)
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
#pragma warning(default: 4062)
	std::cout << std::endl;
}

float yaw = -90.0f;
float pitch = 0.0f;
float OpenCAGELevelViewer::_3DView::fov = 45.0f;
float OpenCAGELevelViewer::_3DView::mouseSensitivity = 1.0f;

void OpenCAGELevelViewer::_3DView::updateCamera(signed char x, signed char y, signed char z, signed char roll, int32_t mouseX, int32_t mouseY, float scrollY, bool isShiftPressed, bool isCtrlPressed, float deltaTime) {
	// TODO: Use isCtrlPressed to create some sort of accelerating camera, kind of like Optifine/Zoomify/Whatever (Minecraft)'s Cinematic Camera.
	
	yaw += mouseX * 0.1 * mouseSensitivity;
	pitch += mouseY * -0.1 * mouseSensitivity;

	if (pitch > 89.0f) pitch = 89.0f;
	else if (pitch < -89.0f) pitch = -89.0f;

	{
		glm::vec3 front;
		front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		front.y = sin(glm::radians(pitch));
		front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		cameraFront = glm::normalize(front);
	}

	cameraPos += ((deltaTime * 5.0f) * cameraFront * static_cast< float >(z) * static_cast< float >(isShiftPressed + 1));
	cameraPos += ((deltaTime * 5.0f) * cameraUp * static_cast< float >(-y) * static_cast< float >(isShiftPressed + 1));
	cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * (deltaTime * 5.0f) * static_cast< float >(x) * static_cast< float >(isShiftPressed + 1);

	fov += -scrollY;

	if (fov < 30.0f) fov = 30.0f;
	else if (fov > 120.0f) fov = 120.0f;
	//cameraPos += glm::normalize(glm::cross(cameraFront, cameraRight)) * (deltaTime * 5.0f) * static_cast< float >(y);
}

//ImVec2 previousWindowSize;
//glm::mat4 projection = glm::perspective(glm::radians(45.0f), ( float ) SCR_WIDTH / ( float ) SCR_HEIGHT, 0.1f, 100.0f);
//
//static void updateSizeSpecificVariablesWhenNeeded(ImVec2 windowSize) {
//	if (windowSize != previousWindowSize) {
//
//	}
//}

#pragma region Shader Classes
class ShaderPart {
private:
	GLuint _shader;
public:
	ShaderPart(std::string shaderPath, GLenum shaderType) {
		std::ifstream shaderStream(shaderPath);
		std::stringstream shader;
		shader << shaderStream.rdbuf();
		std::string shaderString = shader.str();
		const char *shaderCstring = shaderString.c_str();
		shaderStream.close();

		_shader = glCreateShader(shaderType);
		glShaderSource(_shader, 1, (&shaderCstring), NULL);
		glCompileShader(_shader);

		{
			int success;
			glGetShaderiv(_shader, GL_COMPILE_STATUS, &success);
			if (!success) {
				int length = 0;
				glGetShaderInfoLog(_shader, 0, &length, nullptr);

				std::string infoLog(length, '\0');
				glGetShaderInfoLog(_shader, length, nullptr, infoLog.data());

				throw std::exception(infoLog.data());

				//std::cout << infoLog << std::endl;
			}
		}
	}

	ShaderPart(const ShaderPart &) = delete;
	void operator=(const ShaderPart &) = delete;

	~ShaderPart() {
		//glDeleteShader(_shader);
	}

	GLuint getShader() const { return _shader; }
};

static std::filesystem::path getApplicationPath() {
	return std::filesystem::path(std::string(SDL_GetBasePath()));
}

static GLuint createShaderProgram(std::string vertexShaderPath, std::string fragmentShaderPath) {
	std::filesystem::path vertexShaderStdPath = getApplicationPath() / vertexShaderPath;
	std::filesystem::path fragmentShaderStdPath = getApplicationPath() / fragmentShaderPath;

	GLuint vertexShader;
	{
		std::ifstream vertexShaderStream(vertexShaderStdPath);
		std::stringstream vertexShaderStringStream;
		vertexShaderStringStream << vertexShaderStream.rdbuf();
		std::string vertexShaderString = vertexShaderStringStream.str();
		const char *vertexShaderCstring = vertexShaderString.c_str();
		vertexShaderStream.close();

		vertexShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertexShader, 1, (&vertexShaderCstring), NULL);
		glCompileShader(vertexShader);

		{
			int success;
			glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
			if (!success) {
				int length = 0;
				glGetShaderInfoLog(vertexShader, 0, &length, nullptr);

				std::string infoLog(length, '\0');
				glGetShaderInfoLog(vertexShader, length, nullptr, infoLog.data());

				throw std::exception(infoLog.data());

				//std::cout << infoLog << std::endl;
			}
		}
	}

	GLuint fragmentShader;
	{
		std::ifstream fragmentShaderStream(fragmentShaderStdPath);
		std::stringstream fragmentShaderStringStream;
		fragmentShaderStringStream << fragmentShaderStream.rdbuf();
		std::string fragmentShaderString = fragmentShaderStringStream.str();
		const char *fragmentShaderCstring = fragmentShaderString.c_str();
		fragmentShaderStream.close();

		fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragmentShader, 1, (&fragmentShaderCstring), NULL);
		glCompileShader(fragmentShader);

		{
			int success;
			glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
			if (!success) {
				int length = 0;
				glGetShaderInfoLog(fragmentShader, 0, &length, nullptr);

				std::string infoLog(length, '\0');
				glGetShaderInfoLog(fragmentShader, length, nullptr, infoLog.data());

				throw std::exception(infoLog.data());

				//std::cout << infoLog << std::endl;
			}
		}
	}

	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	{
		int success;
		glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
		if (!success) {
			int length = 0;
			glGetShaderInfoLog(shaderProgram, 0, &length, nullptr);

			std::string infoLog(length, '\0');
			glGetShaderInfoLog(shaderProgram, length, nullptr, infoLog.data());

			throw std::exception(infoLog.data());

			//std::cout << infoLog << std::endl;
		}
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}

//class ShaderProgram {
//private:
//	GLuint _program = 0;
//	bool isInitalised = false;
//public:
//	ShaderProgram(std::string vertexShaderPath, std::string fragmentShaderPath) {
//		GLuint vertexShader;
//		{
//			std::ifstream vertexShaderStream(vertexShaderPath);
//			std::stringstream vertexShaderStringStream;
//			vertexShaderStringStream << vertexShaderStream.rdbuf();
//			std::string vertexShaderString = vertexShaderStringStream.str();
//			const char *vertexShaderCstring = vertexShaderString.c_str();
//			vertexShaderStream.close();
//
//			vertexShader = glCreateShader(GL_VERTEX_SHADER);
//			glShaderSource(vertexShader, 1, (&vertexShaderCstring), NULL);
//			glCompileShader(vertexShader);
//
//			{
//				int success;
//				glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
//				if (!success) {
//					int length = 0;
//					glGetShaderInfoLog(vertexShader, 0, &length, nullptr);
//
//					std::string infoLog(length, '\0');
//					glGetShaderInfoLog(vertexShader, length, nullptr, infoLog.data());
//
//					throw std::exception(infoLog.data());
//
//					//std::cout << infoLog << std::endl;
//				}
//			}
//		}
//
//		GLuint fragmentShader;
//		{
//			std::ifstream fragmentShaderStream(fragmentShaderPath);
//			std::stringstream fragmentShaderStringStream;
//			fragmentShaderStringStream << fragmentShaderStream.rdbuf();
//			std::string fragmentShaderString = fragmentShaderStringStream.str();
//			const char *fragmentShaderCstring = fragmentShaderString.c_str();
//			fragmentShaderStream.close();
//
//			fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
//			glShaderSource(fragmentShader, 1, (&fragmentShaderCstring), NULL);
//			glCompileShader(fragmentShader);
//
//			{
//				int success;
//				glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
//				if (!success) {
//					int length = 0;
//					glGetShaderInfoLog(fragmentShader, 0, &length, nullptr);
//
//					std::string infoLog(length, '\0');
//					glGetShaderInfoLog(fragmentShader, length, nullptr, infoLog.data());
//
//					throw std::exception(infoLog.data());
//
//					//std::cout << infoLog << std::endl;
//				}
//			}
//		}
//
//		_program = glCreateProgram();
//		glAttachShader(_program, vertexShader);
//		glAttachShader(_program, fragmentShader);
//		glLinkProgram(_program);
//
//		{
//			int success;
//			glGetProgramiv(_program, GL_LINK_STATUS, &success);
//			if (!success) {
//				int length = 0;
//				glGetShaderInfoLog(_program, 0, &length, nullptr);
//
//				std::string infoLog(length, '\0');
//				glGetShaderInfoLog(_program, length, nullptr, infoLog.data());
//
//				throw std::exception(infoLog.data());
//
//				//std::cout << infoLog << std::endl;
//			}
//		}
//
//		glDeleteShader(vertexShader);
//		glDeleteShader(fragmentShader);
//
//		isInitalised = true;
//	}
//
//	ShaderProgram() { }
//
//	ShaderProgram(const ShaderProgram &) = delete;
//	//void operator=(const ShaderProgram &) = delete;
//
//	~ShaderProgram() {
//		glDeleteProgram(_program);
//	}
//
//	GLuint getProgram() { assert(isInitalised); return _program; }
//};
#pragma endregion

GLuint baseProgram;
GLuint axisXProgram;
GLuint axisYProgram;
GLuint axisZProgram;

void OpenCAGELevelViewer::_3DView::Initalise(void) {
	//int data;
	//glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &data);
	//std::cout << data << std::endl;

	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

	glDebugMessageCallback(glDebugOutput, nullptr);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

	//glEnable(GL_CULL_FACE);

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
	{
		std::cout << std::filesystem::current_path() << std::endl;

		baseProgram = createShaderProgram("VertexShader.vert", "FragmentShader.frag");
		axisXProgram = createShaderProgram("AxisVertexShader.vert", "AxisFragmentShaderX.frag");
		axisYProgram = createShaderProgram("AxisVertexShader.vert", "AxisFragmentShaderY.frag");
		axisZProgram = createShaderProgram("AxisVertexShader.vert", "AxisFragmentShaderZ.frag");
	}
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
	const aiScene *scene = import.ReadFile((getApplicationPath() / "AxisArrow.obj").string(), aiProcess_Triangulate);

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

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

#if 0
#pragma region Scene Management
struct ParsedModel {
	GLuint vertexArrayObject = 0;

	GLuint vertexBufferObject = 0;
	GLuint elementBufferObject = 0;
	size_t elementCount = 0;

	GLuint shaderProgram = 0;

	//glm::mat4 modelTransform;
};

//std::map < OpenCAGELevelViewer::ContentManager::UnmanagedComposite *, ExtendedComposite > sceneComposites;
//std::map < OpenCAGELevelViewer::ContentManager::UnmanagedModelReference *, ExtendedModelReference > sceneModelReferences;

//std::map<U>

struct ExtendedComposite {
	OpenCAGELevelViewer::ContentManager::UnmanagedComposite *baseUnmanagedComposite;
	glm::mat4 absoluteTransform;
};

struct ExtendedModelReference : public OpenCAGELevelViewer::ContentManager::UnmanagedModelReference {
	OpenCAGELevelViewer::ContentManager::UnmanagedModelReference *baseUnmanagedModelReference;
	glm::mat4 absoluteTransform;
};

//struct ExtendedModelReference {
	//glm::mat4 absolute
//};

std::map<unsigned long long, ParsedModel> parsedCs2Models;

std::optional < OpenCAGELevelViewer::_3DView::UsuableUnmanagedObjects > _3dViewSelectedUnmanagedObject;
#endif
void OpenCAGELevelViewer::_3DView::UpdateScene(/*std::optional < OpenCAGELevelViewer::_3DView::UsuableUnmanagedObjects > unmanagedObject, */std::atomic_flag &isDone) {
#if 0
	for (auto &sceneModel : parsedCs2Models) {
		glDeleteBuffers(2, &sceneModel.second.vertexBufferObject); // ebo is sure to come after this, so we can do this
		glDeleteVertexArrays(1, &sceneModel.second.vertexArrayObject);
	}

	parsedCs2Models.clear();

	//rootSceneComposite = SceneComposite(); //delete everything

	_3dViewSelectedUnmanagedObject.reset();

	std::map<unsigned long long, ParsedModel> newParsedCs2Models;

	std::function<ParsedModel(OpenCAGELevelViewer::ContentManager::UnmanagedModelReference::ModelStorage &, const glm::mat4 &)> parseModel = [](OpenCAGELevelViewer::ContentManager::UnmanagedModelReference::ModelStorage &modelStorage, const glm::mat4 &currentAbsoluteTransform) {
		ParsedModel parsedModel;

		//while (true) {
			//auto glError = glGetError();

			//if (glError == GL_NO_ERROR) {
			//	break;
			//}
		//}

		//GLuint vao = 0;
		glGenVertexArrays(1, &parsedModel.vertexArrayObject);

		//auto aew = glGetError();

		//parsedModel.vertexArrayObject = vao;

		//if (parsedModel.vertexArrayObject == -1) {
			//auto a = glGetError();

			//__debugbreak();
		//}

		glGenBuffers(1, &parsedModel.vertexBufferObject);
		glGenBuffers(1, &parsedModel.elementBufferObject);

		glBindVertexArray(parsedModel.vertexArrayObject);

		glBindBuffer(GL_ARRAY_BUFFER, parsedModel.vertexBufferObject);
		glBufferData(GL_ARRAY_BUFFER, modelStorage.vertices.size() * sizeof(OpenCAGELevelViewer::ContentManager::Vector3<float>), modelStorage.vertices.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, parsedModel.elementBufferObject);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, modelStorage.indices.size() * sizeof(uint16_t), modelStorage.indices.data(), GL_STATIC_DRAW);
		parsedModel.elementCount = modelStorage.indices.size();

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(OpenCAGELevelViewer::ContentManager::Vector3<float>), ( void * ) 0);
		glEnableVertexAttribArray(0);

		glBindVertexArray(0);

		parsedModel.shaderProgram = baseProgram;

		//parsedModel.modelTransform = currentAbsoluteTransform;

		return parsedModel;
	};

	//std::function<void(OpenCAGELevelViewer::ContentManager::UnmanagedComposite &, const glm::mat4 &)> parseComposite;
	//
	//parseComposite = [&parseModel, &parseComposite, &newParsedCs2Models](OpenCAGELevelViewer::ContentManager::UnmanagedComposite &unmanagedComposite, const glm::mat4 &currentAbsoluteTransform) {
	//	//glm::mat4 absoluteTransformForComposite 

	//	for (auto &unmanagedComposite : unmanagedComposite.unmanagedCompositeChildren) {
	//		parseComposite(unmanagedComposite);
	//	}

	//	for (auto &unmanagedEntity : unmanagedComposite.unmanagedEntityChildren) {
	//		if (std::holds_alternative< OpenCAGELevelViewer::ContentManager::UnmanagedModelReference >(unmanagedEntity)) {
	//			auto unmanagedModelReference = std::get< OpenCAGELevelViewer::ContentManager::UnmanagedModelReference >(unmanagedEntity);

	//			if (unmanagedModelReference.model.has_value() && !newParsedCs2Models.contains(unmanagedModelReference.model->id)) {
	//				newParsedCs2Models[unmanagedModelReference.model->id] = parseModel(*unmanagedModelReference.model);
	//			}

	//			//if (unmanagedModelReference.model->id)
	//		}
	//	}
	//	};

	//if (unmanagedObject.has_value()) {
	//	if (std::holds_alternative< OpenCAGELevelViewer::ContentManager::UnmanagedModelReference >(*unmanagedObject)) {
	//		parsedCs2Models[std::get< OpenCAGELevelViewer::ContentManager::UnmanagedModelReference >(*unmanagedObject).model->id] = parseModel(*std::get< OpenCAGELevelViewer::ContentManager::UnmanagedModelReference >(*unmanagedObject).model, glm::mat4(1.0f));
	//	} else {
	//		parseComposite(std::get< OpenCAGELevelViewer::ContentManager::UnmanagedComposite >(*unmanagedObject), glm::mat4(1.0f));
	//	}
	//}

	//parsedCs2Models = newParsedCs2Models;

	//_3dViewSelectedUnmanagedObject = unmanagedObject;
	////_3dViewSelectedUnmanagedObject.value() = unmanagedObject.value();

	//isDone.test_and_set();

	//std::function<void(SceneComposite)> processSceneComposite;

	//processSceneComposite = [processSceneComposite](SceneComposite sceneComposite) {
	//	for (auto &sceneObject : sceneComposite.sceneObjects) {
	//		glDeleteBuffers(2, &sceneObject.sceneModel.vertexBufferObject); // ebo is sure to come after this
	//		glDeleteVertexArrays(1, &sceneObject.sceneModel.vertexArrayObject);
	//	}
	//	for (auto &sceneComposite : sceneComposite.sceneComposites) {
	//		processSceneComposite(sceneComposite);
	//	}
	//};

	//for (auto &sceneComposite : rootSceneComposite.sceneComposites) {
	//	sceneComposite.
	//};
#endif
}
#if 0
#pragma endregion

static void renderUnmanagedModelStorage(OpenCAGELevelViewer::ContentManager::UnmanagedModelReference::ModelStorage modelStorage) {
	
}

static void renderUnmanagedModelReference(OpenCAGELevelViewer::ContentManager::UnmanagedModelReference unmanangedModelReference) {

}
#endif

struct MRBufferAllocation {
	size_t modelId;
	size_t size;
	size_t offset;
};

std::vector < MRBufferAllocation > MRVboAllocations {};
std::vector < MRBufferAllocation > MREboAllocations {};
std::vector < MRBufferAllocation > MRIboAllocations {};
std::vector < MRBufferAllocation > MRIndirectBufferAllocations {};

GLuint MRVertexBuffer = 0;
GLuint MRElementBuffer = 0;
GLuint MRInstanceBuffer = 0;
GLuint MRIndirectBuffer = 0;
GLuint MRVertexArray = 0;

std::atomic_flag CMLoaded {};
static void regenerateMRBuffers() {
	if (MRVertexBuffer == 0)
		glGenBuffers(1, &MRVertexBuffer);
	if (MRElementBuffer == 0)
		glGenBuffers(1, &MRElementBuffer);
	if (MRInstanceBuffer == 0)
		glGenBuffers(1, &MRInstanceBuffer);
	if (MRIndirectBuffer == 0)
		glGenBuffers(1, &MRIndirectBuffer);
	if (MRVertexArray == 0)
		glGenVertexArrays(1, &MRVertexArray);
	
	std::cout << "OpenGL Objects:" << std::endl;
	std::cout << MRVertexBuffer << std::endl;
	std::cout << MRElementBuffer << std::endl;
	std::cout << MRInstanceBuffer << std::endl;
	std::cout << MRIndirectBuffer << std::endl;
	std::cout << MRVertexArray << std::endl;
}

static void destroyMRBuffers() {
	if (MRVertexBuffer != 0) {
		glDeleteBuffers(1, &MRVertexBuffer);
		MRVertexBuffer = 0;
	}
	if (MRElementBuffer != 0) {
		glDeleteBuffers(1, &MRElementBuffer);
		MRElementBuffer = 0;
	}
	if (MRInstanceBuffer != 0) {
		glDeleteBuffers(1, &MRInstanceBuffer);
		MRInstanceBuffer = 0;
	}
	if (MRIndirectBuffer != 0) {
		glDeleteBuffers(1, &MRIndirectBuffer);
		MRIndirectBuffer = 0;
	}
	if (MRVertexArray != 0) {
		glDeleteVertexArrays(1, &MRVertexArray);
		MRVertexArray = 0;
	}

	MRVboAllocations.clear();
	MREboAllocations.clear();
	MRIboAllocations.clear();
	MRIndirectBufferAllocations.clear();
}

static void allocateMRVEBO() {
	std::lock_guard cmLock(OpenCAGELevelViewer::AllInOne::ContentManager::cmMutex);

	std::vector < OpenCAGELevelViewer::AllInOne::ContentManager::CMVertex > workingVbo;
	std::vector < uint32_t > workingEbo;
	std::vector < MRBufferAllocation > workingVboAllocations;
	std::vector < MRBufferAllocation > workingEboAllocations;

	for (const auto &model : OpenCAGELevelViewer::AllInOne::ContentManager::models) {
		{
			MRBufferAllocation vboAllocation {
				model.second.modelId,
				model.second.vertices.size(),
				(!workingVboAllocations.empty()) ? (workingVboAllocations[workingVboAllocations.size() - 1].offset + workingVboAllocations[workingVboAllocations.size() - 1].size) : 0
			};

			workingVboAllocations.push_back(vboAllocation);

			MRBufferAllocation eboAllocation {
				model.second.modelId,
				model.second.elements.size(),
				(!workingEboAllocations.empty()) ? (workingEboAllocations[workingEboAllocations.size() - 1].offset + workingEboAllocations[workingEboAllocations.size() - 1].size) : 0
			};

			workingEboAllocations.push_back(eboAllocation);
		}

		{
			workingVbo.insert(workingVbo.end(), model.second.vertices.begin(), model.second.vertices.end());
			for (const auto index : model.second.elements) {
				workingEbo.push_back(((!workingEboAllocations.empty()) ? (workingEboAllocations[workingEboAllocations.size() - 1].offset) : 0) + index);
			}
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, MRVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, workingVbo.size() * sizeof(decltype(workingVbo)::value_type), workingVbo.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, MRElementBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, workingEbo.size() * sizeof(decltype(workingEbo)::value_type), workingEbo.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	MRVboAllocations = workingVboAllocations;
	MREboAllocations = workingEboAllocations;

	//OpenCAGELevelViewer::AllInOne::ContentManager::
}

//int test = GL_UNSIGNED_INT64_ARB;

static void fillInMRInstanceBuffer() {
	std::lock_guard cmLock(OpenCAGELevelViewer::AllInOne::ContentManager::cmMutex);

	std::vector < OpenCAGELevelViewer::AllInOne::ContentManager::ModelReferenceGL > workingInstanceBuffer;
	std::vector < MRBufferAllocation > workingInstanceBufferAllocations;

	for (const auto &modelReferences : OpenCAGELevelViewer::AllInOne::ContentManager::modelReferences) {
		if (modelReferences.second.empty())
			continue;

		workingInstanceBuffer.insert(workingInstanceBuffer.end(), modelReferences.second.begin(), modelReferences.second.end());

		MRBufferAllocation instanceBufferAllocation {
			modelReferences.first,
			modelReferences.second.size(),
			(!workingInstanceBufferAllocations.empty()) ? workingInstanceBufferAllocations[workingInstanceBufferAllocations.size() - 1].offset + workingInstanceBufferAllocations[workingInstanceBufferAllocations.size() - 1].size : 0};
		workingInstanceBufferAllocations.push_back(instanceBufferAllocation);
	}

	glBindBuffer(GL_ARRAY_BUFFER, MRInstanceBuffer);
	glBufferData(GL_ARRAY_BUFFER, workingInstanceBuffer.size() * sizeof(decltype(workingInstanceBuffer)::value_type), workingInstanceBuffer.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	MRIboAllocations = workingInstanceBufferAllocations;
}

// taken straight from the opengl documentation.
struct DrawElementsIndirectCommand {
	GLuint   count;
	GLuint   instanceCount;
	GLuint   firstIndex;
	GLint    baseVertex;
	GLuint   baseInstance;
};

static void fillInMRIndirectBuffer() {
	std::lock_guard cmLock(OpenCAGELevelViewer::AllInOne::ContentManager::cmMutex);
	
	std::vector < DrawElementsIndirectCommand > workingIndirectBuffer {};
	std::vector < MRBufferAllocation > workingIndirectBufferAllocations {};

	for (size_t i = 0; i < MRIboAllocations.size(); i++) {
		DrawElementsIndirectCommand deic {};

		deic.count = MREboAllocations[i].size;
		deic.instanceCount = MRIboAllocations[i].size;
		deic.firstIndex = MREboAllocations[i].offset;
		deic.baseVertex = 0; // We already account for this.
		deic.baseInstance = MRIboAllocations[i].offset;

		workingIndirectBuffer.push_back(deic);


		{
			MRBufferAllocation indirectBufferAllocation {
				MRIboAllocations[i].modelId,
				1,
				i
			};

			workingIndirectBufferAllocations.push_back(indirectBufferAllocation);
		}
	}

	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, MRIndirectBuffer);
	glBufferData(GL_DRAW_INDIRECT_BUFFER, workingIndirectBuffer.size() * sizeof(decltype(workingIndirectBuffer)::value_type), workingIndirectBuffer.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

	MRIndirectBufferAllocations = workingIndirectBufferAllocations;
}

GLuint64 test = 5;
GLenum test2 = GL_UNSIGNED_INT64_ARB;

static void makeMRVAO() {
	glBindVertexArray(MRVertexArray);

	glBindVertexBuffer(0, MRVertexBuffer, 0, sizeof(OpenCAGELevelViewer::AllInOne::ContentManager::CMVertex));
	glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, offsetof(OpenCAGELevelViewer::AllInOne::ContentManager::CMVertex, pos));
	glVertexAttribFormat(1, 4, GL_UNSIGNED_BYTE, GL_FALSE, offsetof(OpenCAGELevelViewer::AllInOne::ContentManager::CMVertex, col));
	glVertexAttribBinding(0, 0);
	glVertexAttribBinding(1, 0);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	size_t test = offsetof(OpenCAGELevelViewer::AllInOne::ContentManager::ModelReferenceGL, worldPosition);

	std::cout << "instance id offset " << offsetof(OpenCAGELevelViewer::AllInOne::ContentManager::ModelReferenceGL, instanceId) << std::endl;
	std::cout << "world pos offset " << offsetof(OpenCAGELevelViewer::AllInOne::ContentManager::ModelReferenceGL, worldPosition) << std::endl;
	std::cout << "world rot offset " << offsetof(OpenCAGELevelViewer::AllInOne::ContentManager::ModelReferenceGL, worldRotation) << std::endl;
	std::cout << "col offset offset " << offsetof(OpenCAGELevelViewer::AllInOne::ContentManager::ModelReferenceGL, colOffset) << std::endl;

	glBindVertexBuffer(1, MRInstanceBuffer, 0, sizeof(OpenCAGELevelViewer::AllInOne::ContentManager::ModelReferenceGL));
	glVertexAttribFormat(2, 1, GL_UNSIGNED_INT, GL_FALSE, offsetof(OpenCAGELevelViewer::AllInOne::ContentManager::ModelReferenceGL, instanceId));
	glVertexAttribFormat(3, 3, GL_FLOAT, GL_FALSE, offsetof(OpenCAGELevelViewer::AllInOne::ContentManager::ModelReferenceGL, worldPosition));
	glVertexAttribFormat(4, 3, GL_FLOAT, GL_FALSE, offsetof(OpenCAGELevelViewer::AllInOne::ContentManager::ModelReferenceGL, worldRotation));
	glVertexAttribFormat(5, 4, GL_FLOAT, GL_FALSE, offsetof(OpenCAGELevelViewer::AllInOne::ContentManager::ModelReferenceGL, colOffset));
	glVertexAttribBinding(2, 1);
	glVertexAttribBinding(3, 1);
	glVertexAttribBinding(4, 1);
	glVertexAttribBinding(5, 1);
	glVertexBindingDivisor(1, 1);
	/*glVertexAttribDivisor(2, 1);
	glVertexAttribDivisor(3, 1);
	glVertexAttribDivisor(4, 1);
	glVertexAttribDivisor(5, 1);*/
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);
	glEnableVertexAttribArray(4);
	glEnableVertexAttribArray(5);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, MRElementBuffer);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, MRIndirectBuffer);

	glBindVertexArray(0);
}

static void regenerateMRVAO() {
	std::lock_guard cmLock(OpenCAGELevelViewer::AllInOne::ContentManager::cmMutex);
	OpenCAGELevelViewer::AllInOne::ContentManager::CMStatusEnum cmStatus = OpenCAGELevelViewer::AllInOne::ContentManager::cmStatus.load().currentStatus;

	glBindVertexArray(0);

	switch (cmStatus) {
		case OpenCAGELevelViewer::AllInOne::ContentManager::CMStatusEnum::LOADING:
			{
				destroyMRBuffers();

				break;
			}
		case OpenCAGELevelViewer::AllInOne::ContentManager::CMStatusEnum::DIRTY:
			{
				destroyMRBuffers();
				regenerateMRBuffers();
				 
				allocateMRVEBO();
				fillInMRInstanceBuffer();
				fillInMRIndirectBuffer();
				makeMRVAO();

				OpenCAGELevelViewer::AllInOne::ContentManager::cmStatus.store({OpenCAGELevelViewer::AllInOne::ContentManager::CMStatusEnum::READY});

				break;
			}
		case OpenCAGELevelViewer::AllInOne::ContentManager::CMStatusEnum::READY:
			break;
	}
}

void OpenCAGELevelViewer::_3DView::Render(ImVec2 windowSize/*, std::optional<std::variant<OpenCAGELevelViewer::ContentManager::UnmanagedModelReference, OpenCAGELevelViewer::ContentManager::UnmanagedComposite>> unmanagedModelReference*//*, int msaaSamples*/) {
	regenerateMRVAO();

#pragma region Rendering Start
	glBindFramebuffer(GL_FRAMEBUFFER, _3dViewRenderFbo);

	// update the texture size to correspond to the window
	//glBindTexture(GL_TEXTURE_2D, _3dViewRenderFboTextureColorbuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, static_cast< GLsizei >(windowSize.x), static_cast< GLsizei >(windowSize.y), 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	//glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, pow(2, msaaSamples), GL_RGB, static_cast< GLsizei >(windowSize.x), static_cast< GLsizei >(windowSize.y), GL_TRUE);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, static_cast< GLsizei >(windowSize.x), static_cast< GLsizei >(windowSize.y));
	//glRenderbufferStorageMultisample(GL_RENDERBUFFER, 1, GL_DEPTH24_STENCIL8, static_cast< GLsizei >(windowSize.x), static_cast< GLsizei >(windowSize.y));

	//glBindFramebuffer(GL_FRAMEBUFFER, _3dViewOutputFbo);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, static_cast< GLsizei >(windowSize.x), static_cast< GLsizei >(windowSize.y), 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

	//glBindFramebuffer(GL_FRAMEBUFFER, _3dViewRenderFbo);

	//glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//glPolygonMode(GL_FRONT, GL_FILL);
	//glPolygonMode(GL_BACK, GL_LINE);

	glViewport(0, 0, static_cast< GLsizei >(windowSize.x), static_cast< GLsizei >(windowSize.y));
	//glViewport(0, 0, 1600, 1200);

	glClearColor(0.2f, 0.3f, 0.3f, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (fov < 30.0f) fov = 30.0f;
	else if (fov > 120.0f) fov = 120.0f;

	glm::mat4 projection = glm::perspective(glm::radians(fov), ( float ) windowSize.x / ( float ) windowSize.y, 0.1f, 1000.0f);
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

#if 0
#pragma region Scene Drawing
	auto &localParsedCs2Models = parsedCs2Models;

	std::function<void(const OpenCAGELevelViewer::ContentManager::UnmanagedModelReference &, const glm::mat4 &)> renderModelReference;

	renderModelReference = [&localParsedCs2Models, &projection, &view](const OpenCAGELevelViewer::ContentManager::UnmanagedModelReference &unmanagedModelReference, const glm::mat4 &currentAbsoluteTransform) {
		if (!unmanagedModelReference.model.has_value())
			return;

		glm::mat4 absoluteTransformForEntity = glm::translate(currentAbsoluteTransform, glm::vec3(unmanagedModelReference.transform.position.x, unmanagedModelReference.transform.position.y, unmanagedModelReference.transform.position.z));
		absoluteTransformForEntity = glm::rotate(absoluteTransformForEntity, glm::radians(static_cast< float >(unmanagedModelReference.transform.rotation.x)), glm::vec3(1.0f, 0.0f, 0.0f));
		absoluteTransformForEntity = glm::rotate(absoluteTransformForEntity, glm::radians(static_cast< float >(unmanagedModelReference.transform.rotation.y)), glm::vec3(0.0f, 1.0f, 0.0f));
		absoluteTransformForEntity = glm::rotate(absoluteTransformForEntity, glm::radians(static_cast< float >(unmanagedModelReference.transform.rotation.z)), glm::vec3(0.0f, 0.0f, 1.0f));

		glBindVertexArray(localParsedCs2Models[unmanagedModelReference.model->id].vertexArrayObject);

		glUseProgram(localParsedCs2Models[unmanagedModelReference.model->id].shaderProgram);
		glUniformMatrix4fv(glGetUniformLocation(axisYProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(axisYProgram, "view"), 1, GL_FALSE, &view[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(axisYProgram, "model"), 1, GL_FALSE, &absoluteTransformForEntity[0][0]);

		glDrawElements(GL_TRIANGLES, localParsedCs2Models[unmanagedModelReference.model->id].elementCount, GL_UNSIGNED_INT, 0);
		};

	std::function<void(const OpenCAGELevelViewer::ContentManager::UnmanagedComposite &, const glm::mat4 &)> renderComposite;
	 
	renderComposite = [&localParsedCs2Models, &renderModelReference, &renderComposite, &projection, &view](const OpenCAGELevelViewer::ContentManager::UnmanagedComposite &unmanagedComposite, const glm::mat4 &currentAbsoluteTransform) {
		glm::mat4 absoluteTransformForComposite = glm::translate(currentAbsoluteTransform, glm::vec3(unmanagedComposite.transform.position.x, unmanagedComposite.transform.position.y, unmanagedComposite.transform.position.z));
		absoluteTransformForComposite = glm::rotate(absoluteTransformForComposite, glm::radians(static_cast< float >(unmanagedComposite.transform.rotation.x)), glm::vec3(1.0f, 0.0f, 0.0f));
		absoluteTransformForComposite = glm::rotate(absoluteTransformForComposite, glm::radians(static_cast< float >(unmanagedComposite.transform.rotation.y)), glm::vec3(0.0f, 1.0f, 0.0f));
		absoluteTransformForComposite = glm::rotate(absoluteTransformForComposite, glm::radians(static_cast< float >(unmanagedComposite.transform.rotation.z)), glm::vec3(0.0f, 0.0f, 1.0f));

		for (auto &unmanagedEntity : unmanagedComposite.unmanagedEntityChildren) {
			if (std::holds_alternative<OpenCAGELevelViewer::ContentManager::UnmanagedModelReference>(unmanagedEntity)) {
				renderModelReference(std::get<OpenCAGELevelViewer::ContentManager::UnmanagedModelReference>(unmanagedEntity), absoluteTransformForComposite);
			}
		}

		for (auto &unmanagedComposite : unmanagedComposite.unmanagedCompositeChildren) {
			renderComposite(unmanagedComposite, absoluteTransformForComposite);
		}
		};

	if (_3dViewSelectedUnmanagedObject.has_value()) {
		//auto _3dViewSelectedUnmanagedObjectLocal = _3dViewSelectedUnama

		if (std::holds_alternative < OpenCAGELevelViewer::ContentManager::UnmanagedComposite >(_3dViewSelectedUnmanagedObject.value())) {
			renderComposite(std::get< OpenCAGELevelViewer::ContentManager::UnmanagedComposite >(_3dViewSelectedUnmanagedObject.value()), glm::mat4(1.0f));
		} else {
			renderModelReference(std::get< OpenCAGELevelViewer::ContentManager::UnmanagedModelReference >(_3dViewSelectedUnmanagedObject.value()), glm::mat4(1.0f));
		}
	}
#pragma endregion
#endif

#pragma region Drawing Axis
	//glDisable(GL_DEPTH_TEST);

	{
		glBindVertexArray(_3dViewAxisArrowVao);
		glm::mat4 modelMatrix(1.0f);
		modelMatrix = glm::scale(modelMatrix, glm::vec3(0.3f));

		glUseProgram(axisXProgram);
		glUniformMatrix4fv(glGetUniformLocation(axisXProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(axisXProgram, "view"), 1, GL_FALSE, &view[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(axisXProgram, "model"), 1, GL_FALSE, &modelMatrix[0][0]);

		glDrawElements(GL_TRIANGLES, _3dViewAxisArrowIndicesCount, GL_UNSIGNED_INT, 0);

		modelMatrix = glm::rotate(modelMatrix, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, -1.0f));

		glUseProgram(axisYProgram);
		glUniformMatrix4fv(glGetUniformLocation(axisYProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(axisYProgram, "view"), 1, GL_FALSE, &view[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(axisYProgram, "model"), 1, GL_FALSE, &modelMatrix[0][0]);

		glDrawElements(GL_TRIANGLES, _3dViewAxisArrowIndicesCount, GL_UNSIGNED_INT, 0);

		modelMatrix = glm::rotate(modelMatrix, glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f));

		glUseProgram(axisZProgram);
		glUniformMatrix4fv(glGetUniformLocation(axisYProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(axisYProgram, "view"), 1, GL_FALSE, &view[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(axisYProgram, "model"), 1, GL_FALSE, &modelMatrix[0][0]);

		glDrawElements(GL_TRIANGLES, _3dViewAxisArrowIndicesCount, GL_UNSIGNED_INT, 0);
	}

	//glEnable(GL_DEPTH_TEST);

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

	if (MRVertexArray != 0) {
		glUseProgram(baseProgram);
		glUniformMatrix4fv(glGetUniformLocation(baseProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(baseProgram, "view"), 1, GL_FALSE, &view[0][0]);

		glBindVertexArray(MRVertexArray);
		glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, MRIndirectBufferAllocations.size(), 0);
		glBindVertexArray(0);
	}

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
