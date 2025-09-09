//#include "OpenCAGELevelViewer.h"
#include "pch.h"

#include "handoff.h"

#include "Configuration.h"
#include "ContentManager.h"
#include "WebsocketThread.h"
#include <chrono>
#include <functional>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <iostream>
#include <SDL3/SDL.h>
#include <string>
#include <thread>
#include <variant>

#include "3DView.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <map>
#include <unordered_map>
#include <vector>

using namespace gl;
using namespace std::chrono_literals;

SDL_Window *sdlWindow;
SDL_Renderer *sdlRenderer;

void fatalError(std::string baseErrorMessage) {
	std::string errorMessage = baseErrorMessage + "\n\n" + "OpenCAGE C++ Level Viewer will now exit.";

	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "OpenCAGE C++ Level Viewer", errorMessage.data(), NULL);

	exit(1);
}

void fatalSdlError(std::string baseErrorMessage) {
	//std::string errorMessage = baseErrorMessage + "\n\;

	fatalError(baseErrorMessage + "\n\n" + SDL_GetError());
}

static bool isDemoWindowOpen = true;

struct _3DViewConfiguration {
	int msaaSamples;
	bool useLods;
};

typedef std::chrono::steady_clock SteadyClock;
typedef std::chrono::time_point < SteadyClock > SteadyClockTimePoint;
SteadyClockTimePoint timeSinceConnectDialogOpen = SteadyClockTimePoint();

OpenCAGELevelViewer::AllInOne::Handoff::SceneRenderingStrategy OpenCAGELevelViewer::AllInOne::Handoff::currentSceneRenderingStrategy = OpenCAGELevelViewer::AllInOne::Handoff::SceneRenderingStrategy::INDIRECT_BATCHING;


std::string OpenCAGELevelViewer::AllInOne::Handoff::gameRoot = "";


static std::atomic_flag commandsEditorDependent {};

#pragma managed(push, on)
static void forceCoreClr() {

}
#pragma managed(pop)

#pragma managed(push, off)
static bool modifiedProfile = false;

static void setGameRootFolder(void *userData, const char *const *filelist, int filter) {
	if (filelist[0] != nullptr) {
		OpenCAGELevelViewer::AllInOne::ContentManager::setGameRoot(std::string(filelist[0]));
		modifiedProfile = true;
	}
}
#pragma managed(pop)

static std::hash < std::string > stringHashFunction {};

#pragma region Commands Editor

static bool commandsContentWindowOpen = true;

typedef struct CommandsContent;

struct CommandsContentType {
	bool isFolder;
	std::string name;

	bool operator==(const CommandsContentType &other) const = default;
	bool operator<(const CommandsContentType &other) const {
		return (name == other.name) ?
			isFolder == other.isFolder ? false : isFolder :
			name < other.name;
	}
	bool operator>(const CommandsContentType &other) const {
		return (name == other.name) ?
			isFolder == other.isFolder ? false : !isFolder :
			name > other.name;
	}

	//bool operator==(const CommandsContentType &other) const {
		//return isFolder == other.isFolder && name == other.name;
	//}
};

template<>
struct std::hash<CommandsContentType> {
	std::size_t operator()(const CommandsContentType &commandsContentType) const noexcept {
		std::size_t h1 = std::hash<bool> {}(commandsContentType.isFolder);
		std::size_t h2 = std::hash<std::string> {}(commandsContentType.name);
		return OpenCAGELevelViewer::AllInOne::combineHash(h1, h2);
	}
};

using CommandsContentChildren = std::map < CommandsContentType, CommandsContent >;

struct CommandsContent {
	std::string name {};
	uint32_t shortGuid = 0xFFFFFFFF;
	CommandsContentChildren children {};
};

#pragma managed(push, on)
//const std::string *compositeStringContainsSearchTerm

ref class CompositeNameSearchTermPredicate {
private:
	System::String ^searchTerm;
public:
	CompositeNameSearchTermPredicate(System::String ^_searchTerm) {
		searchTerm = _searchTerm;
	}

	bool Predicate(System::String ^composite) {
		return composite->ToLower()->Contains(searchTerm->ToLower());
	}
};

static bool fillInCommandsContentChildren(CommandsContentChildren &commandsContentChildren, const std::string &searchTerm) {
	commandsContentChildren.clear();

	std::lock_guard cmLock(OpenCAGELevelViewer::AllInOne::ContentManager::cmMutex);

	// Regenerate commandsContent.

	if (OpenCAGELevelViewer::AllInOne::ContentManager::levelContentInstance.operator OpenCAGELevelViewer::AllInOne::ContentManager::LevelContent ^() == nullptr)
		return false;

	if (OpenCAGELevelViewer::AllInOne::ContentManager::levelContentInstance->combinedHash != OpenCAGELevelViewer::AllInOne::combineHash(OpenCAGELevelViewer::AllInOne::ContentManager::getGameRootHash(), OpenCAGELevelViewer::AllInOne::ContentManager::getLevelHash()))
		return false;

	CATHODE::Commands ^localCommandsHandle = OpenCAGELevelViewer::AllInOne::ContentManager::levelContentInstance->Commands;

	if (localCommandsHandle == nullptr)
		return false;

	CompositeNameSearchTermPredicate ^compositeNameSearchTermPredicate = gcnew CompositeNameSearchTermPredicate(gcnew System::String(searchTerm.data()));
	System::Func < System::String ^, bool > ^compositeNameSearchTermPredicateInstance = gcnew System::Func < System::String ^, bool >(compositeNameSearchTermPredicate, &CompositeNameSearchTermPredicate::Predicate);

	for each(System::String ^ compositeName in
			 System::Linq::ParallelEnumerable::Where(
			 System::Linq::ParallelEnumerable::AsParallel < System::String ^ >(
			 localCommandsHandle->GetCompositeNames()
	), compositeNameSearchTermPredicateInstance)
	) {
		CommandsContentChildren *currentCommandsContentChildren = &commandsContentChildren;

		System::String ^displayCompositeName = compositeName;

		// get rid of P:\\ composites and replace them only with the composite name
		if (displayCompositeName->StartsWith("P:\\") || displayCompositeName->StartsWith("E:\\")) {
			displayCompositeName = displayCompositeName->Split('\\')[(displayCompositeName->Split('\\').Length) - 1];
		}

		array < System::String ^ > ^compositeNameComponents = displayCompositeName->Split('\\');

		for (size_t i = 0; i < compositeNameComponents->Length; i++) {
			CommandsContentType propsedCommandsContentType = {i + 1 != compositeNameComponents->Length, ConvertCliStringToCXXString(compositeNameComponents[i])};

			if (!currentCommandsContentChildren->contains(propsedCommandsContentType)) {
				CommandsContent newContent {};

				newContent.name = ConvertCliStringToCXXString(compositeNameComponents[i]);
				if (i + 1 == compositeNameComponents->Length) {
					if (localCommandsHandle->GetComposite(compositeName) == nullptr) {
						std::cout << "ERR! Composite " << MarshalCliString(compositeName) << " is null." << std::endl;
						__debugbreak();
					} else {
						newContent.shortGuid = localCommandsHandle->GetComposite(compositeName)->shortGUID.AsUInt32;
					}
				}

				currentCommandsContentChildren->operator [](propsedCommandsContentType) = newContent;
			}

			currentCommandsContentChildren = &(currentCommandsContentChildren->operator [](propsedCommandsContentType).children);
			//if (i + 1 == compositeNameComponents->Length)
		}
	}

	return true;
}
#pragma managed(pop)

#pragma managed(push, off)
static void renderCommandsContentChildren(const CommandsContentChildren &commandsContentChildren) {
	for (const auto &pair : commandsContentChildren) {
		const CommandsContentType &type = pair.first;
		const CommandsContent &content = pair.second;
		if (type.isFolder) {
			if (ImGui::TreeNodeEx(content.name.c_str(), ImGuiTreeNodeFlags_OpenOnArrow)) {
				renderCommandsContentChildren(content.children);
				ImGui::TreePop();
			}
		} else {
			ImGui::TreeNodeEx(content.name.c_str(), ImGuiTreeNodeFlags_Leaf | (content.shortGuid == OpenCAGELevelViewer::AllInOne::ContentManager::getComposite() ? ImGuiTreeNodeFlags_Selected : 0));
			if (content.shortGuid != OpenCAGELevelViewer::AllInOne::ContentManager::getComposite() && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
				OpenCAGELevelViewer::AllInOne::ContentManager::setComposite(content.shortGuid);
			}
			ImGui::TreePop();
		}
	}
}

bool commandsContentRerenderNeeded = false;

//static int searchCallback(ImGuiInputTextCallbackData *data) {
//	switch (data->EventFlag) {
//		case ImGuiInputTextFlags_CallbackEdit
//	}
//}

static void renderCommandsContentWindow() {
	if (commandsEditorDependent.test())
		return;

	if (commandsContentWindowOpen) {
		if (ImGui::Begin("Composites List", &commandsContentWindowOpen)) {
			static CommandsContentChildren commandsContent {};

			static std::string compositeSearch(255, '\0');
			if (ImGui::InputTextWithHint("Search", "Enter the name of a composite here", compositeSearch.data(), compositeSearch.size()))
				commandsContentRerenderNeeded = true;

			//{
			static size_t combinedHash = 0;
			//static bool commandsContentNeedsUpdate = true;

			if (combinedHash != OpenCAGELevelViewer::AllInOne::combineHash(OpenCAGELevelViewer::AllInOne::ContentManager::getGameRootHash(), OpenCAGELevelViewer::AllInOne::ContentManager::getLevelHash()))
				commandsContentRerenderNeeded = true;
			//}

			if (commandsContentRerenderNeeded)
				if (fillInCommandsContentChildren(commandsContent, compositeSearch)) {
					combinedHash = OpenCAGELevelViewer::AllInOne::combineHash(OpenCAGELevelViewer::AllInOne::ContentManager::getGameRootHash(), OpenCAGELevelViewer::AllInOne::ContentManager::getLevelHash());
					commandsContentRerenderNeeded = false;
				}

			renderCommandsContentChildren(commandsContent);
		}
		ImGui::End();
	}
}
#pragma managed(pop)
#pragma endregion

#pragma managed(push, on)
static void debugTestDelegate(const int64_t selectedInstanceId) {
	auto compositeTest = OpenCAGELevelViewer::AllInOne::ContentManager::compositesById.operator->()[static_cast < uint32_t >(selectedInstanceId)];
	auto debug = OpenCAGELevelViewer::AllInOne::ContentManager::entityDataRoot.operator->();

	std::cout << compositeTest->Count << std::endl;
}
#pragma managed(pop)

#pragma managed(push, off)
int handoff(char **argv, int argc) {
	// force c++/cli to initalise here.
	// if c++/cli isn't initalised, debugging becomes very strange.
	forceCoreClr();

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMEPAD) < 0) {
		fatalSdlError("SDL3 was unable to initalize one or more of its subsystems.");
	};

	OpenCAGELevelViewer::AllInOne::Configuration::load();

	sdlWindow = SDL_CreateWindow("OpenCAGE Level Viewer", 1600, 1200, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

	if (!sdlWindow) {
		fatalSdlError("SDL3 was unable to create a window.");
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	//SDL_GL_SetAttribute()
	/*

	sdlRenderer = SDL_CreateRenderer(sdlWindow, -1, 0);

	if (!sdlRenderer) {
		fatalSdlError("SDL2 was unable to create a renderer.");
	}*/

	SDL_GLContext gl_context = SDL_GL_CreateContext(sdlWindow);

	/*if (SDL_GL_GetCurrentContext()) {
		fatalSdlError("An OpenGL context wasn't created when the renderer was made.");
	}*/

	int gotMajorVersion = 0;
	int gotMinorVersion = 0;
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &gotMajorVersion);
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &gotMinorVersion);

	//__debugbreak();

	if ((gotMajorVersion != 4 || gotMinorVersion != 3) &&  SDL_GL_ExtensionSupported("ARB_gpu_shader_int64")) {
		fatalError("Your system does not support the needed OpenGL version and/or extensions.\nCheck that you have the latest graphics drivers for your card installed, and that your card is correctly configured.");
	}

	SDL_GL_MakeCurrent(sdlWindow, gl_context);
	SDL_GL_SetSwapInterval(OpenCAGELevelViewer::AllInOne::Configuration::configuration.vsync);

	glbinding::initialize(reinterpret_cast< glbinding::ProcAddress(*)(const char *) >(SDL_GL_GetProcAddress), true);
	//glbinding::initialize(SDL_GL_GetProcAddress);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	//io.ConfigFlags |= ImGuiConfigFlags_

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplSDL3_InitForOpenGL(sdlWindow, SDL_GL_GetCurrentContext());
	ImGui_ImplOpenGL3_Init();

	OpenCAGELevelViewer::WebsocketThread::keepThreadActive.test_and_set();
	OpenCAGELevelViewer::WebsocketThread::willConnect.clear();

	std::atomic_flag suspendFlag;
	suspendFlag.test_and_set();

	std::thread contentManagerThread([&suspendFlag]() -> void { OpenCAGELevelViewer::AllInOne::ContentManager::threadMain(suspendFlag); });
	std::thread websocketThread(OpenCAGELevelViewer::WebsocketThread::main);

	bool userRequestedExit = false;

	//try {
		//bool output = test();

		//std::cout << output << std::endl;
	//} catch (...) {
		//std::exception_ptr exception = std::current_exception();

		//exception.
	//}

	/*ImGuiID dockspaceId = ImGui::GetID("OpenCAGELevelViewer Dockspace");*/

	OpenCAGELevelViewer::_3DView::Initalise();

	static bool _3dViewLockInput = false;

	auto lastFrameTime = std::chrono::duration_cast< std::chrono::microseconds >(std::chrono::system_clock::now().time_since_epoch()).count();    // This is technically the last last frame time
	auto currentFrameTime = lastFrameTime; // For delta time calculations

	while (!userRequestedExit) {
		static float dX = 0;
		static float dY = 0;
		static float dZ = 0;
		static float speed = 1;
		static bool smooth = false;
		//static bool isUpPressed = false;
		//static bool isDownPressed = false;

		//static bool isShiftPressed = false;
		//static bool isCtrlPressed = false;

		bool was3dViewSelectRequested = false;
		//static bool was3dViewSelectRequestedLastFrame = false;

		//was3dViewSelectRequestedLastFrame = was3dViewSelectRequested;

		//if (was3dViewSelectRequested) {
			//std::this_thread::sleep_for(16ms);
			//was3dViewSelectRequested = false;
		//}

		float dYaw = 0;
		float dPitch = 0;

		float dFov = 0;

		
		SDL_Event event;

		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL3_ProcessEvent(&event);

			switch (event.type) {
				case SDL_EVENT_QUIT:
					userRequestedExit = true;
					break;
				case SDL_EVENT_MOUSE_MOTION:
					if (_3dViewLockInput) {
						dYaw += event.motion.xrel;
						dPitch += event.motion.yrel;
					}
					break;
				case SDL_EVENT_MOUSE_WHEEL:
					if (_3dViewLockInput) {
						dFov += event.wheel.y;
					}
					break;
				case SDL_EVENT_KEY_DOWN:
					if (_3dViewLockInput) {
						switch (event.key.key) {
							case SDLK_RCTRL:
								{
									_3dViewLockInput = false;

									ImGuiIO &io = ImGui::GetIO(); ( void ) io;
									io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
									io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
									io.ConfigFlags &= (0xFFFFFFFF ^ ImGuiConfigFlags_NoMouse);

									SDL_SetWindowRelativeMouseMode(sdlWindow, false);
								}
								break;

							case SDLK_E:
								dY += 1;
								break;
							case SDLK_Q:
								dY -= 1;
								break;

							case SDLK_W:
								dZ += 1;
								break;
							case SDLK_S:
								dZ -= 1;
								break;

							case SDLK_D:
								dX += 1;
								break;
							case SDLK_A:
								dZ -= 1;
								break;

							case SDLK_LSHIFT:
								speed = 2;
								break;
							case SDLK_LCTRL:
								smooth = true;
								break;

							case SDLK_RALT:
								// panic button
								dX = 0;
								dY = 0;
								dZ = 0;
								speed = 1;
								smooth = false;
								break;
						}
					}
					break;
				case SDL_EVENT_KEY_UP:
					if (_3dViewLockInput) {
						switch (event.key.key) {
							case SDLK_E:
								dY -= 1;
								break;
							case SDLK_Q:
								dY += 1;
								break;

							case SDLK_W:
								dZ -= 1;
								break;
							case SDLK_S:
								dZ += 1;
								break;

							case SDLK_D:
								dX -= 1;
								break;
							case SDLK_A:
								dZ += 1;
								break;

							case SDLK_LSHIFT:
								speed = 1;
								break;
							case SDLK_LCTRL:
								smooth = false;
								break;
						}
					}
					break;
				case SDL_EVENT_MOUSE_BUTTON_DOWN:
					if (_3dViewLockInput) {
						if (event.button.button == SDL_BUTTON_LEFT) {
							was3dViewSelectRequested = true;
						}
					}

				case SDL_EVENT_GAMEPAD_AXIS_MOTION:
					switch (event.gaxis.axis) {
						case SDL_GamepadAxis::SDL_GAMEPAD_AXIS_LEFTX:
							if (event.gaxis.value < 0) {
								// negative
								dX = event.gaxis.value / static_cast < float >(std::abs(SDL_JOYSTICK_AXIS_MIN));
							} else if (event.gaxis.value > 0) {
								// positive
								dX = event.gaxis.value / static_cast < float >(SDL_JOYSTICK_AXIS_MAX);
							} else {
								// exactly 0
								dX = 0;
							}
							break;
						case SDL_GamepadAxis::SDL_GAMEPAD_AXIS_LEFTY:
							if (event.gaxis.value < 0) {
								// negative
								dZ = event.gaxis.value / static_cast < float >(std::abs(SDL_JOYSTICK_AXIS_MIN));
							} else if (event.gaxis.value > 0) {
								// positive
								dZ = event.gaxis.value / static_cast < float >(SDL_JOYSTICK_AXIS_MAX);
							} else {
								// exactly 0
								dZ = 0;
							}
							break;

						case SDL_GamepadAxis::SDL_GAMEPAD_AXIS_RIGHTX:
							if (event.gaxis.value < 0) {
								// negative
								dYaw = event.gaxis.value / static_cast < float >(std::abs(SDL_JOYSTICK_AXIS_MIN));
							} else if (event.gaxis.value > 0) {
								// positive
								dYaw = event.gaxis.value / static_cast < float >(SDL_JOYSTICK_AXIS_MAX);
							} else {
								// exactly 0
								dYaw = 0;
							}
							break;

						case SDL_GamepadAxis::SDL_GAMEPAD_AXIS_RIGHTY:
							if (event.gaxis.value < 0) {
								// negative
								dPitch = event.gaxis.value / static_cast < float >(std::abs(SDL_JOYSTICK_AXIS_MIN));
							} else if (event.gaxis.value > 0) {
								// positive
								dPitch = event.gaxis.value / static_cast < float >(SDL_JOYSTICK_AXIS_MAX);
							} else {
								// exactly 0
								dPitch = 0;
							}
							break;

						case SDL_GamepadAxis::SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:
							if (event.gaxis.value >= 30000)
								was3dViewSelectRequested = true;
							break;

						case SDL_GamepadAxis::SDL_GAMEPAD_AXIS_LEFT_TRIGGER:
							if (event.gaxis.value == 0)
								speed = 1;
							else
								speed = event.gaxis.value / static_cast < float >(SDL_JOYSTICK_AXIS_MAX);
					}
					break;
			}

			if (event.type == SDL_EVENT_QUIT)
				userRequestedExit = true;
				
		}

		OpenCAGELevelViewer::_3DView::updateCamera(dX, dY, dZ, 0, dYaw, dPitch, dFov, speed, smooth, static_cast< float >(currentFrameTime - lastFrameTime) / std::chrono::microseconds::period::den);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();

		if (commandsEditorDependent.test()) {
			if (!OpenCAGELevelViewer::WebsocketThread::willConnect.test())
				OpenCAGELevelViewer::WebsocketThread::willConnect.test_and_set();
		} else {
			if (OpenCAGELevelViewer::WebsocketThread::willConnect.test())
				OpenCAGELevelViewer::WebsocketThread::willConnect.clear();
		}

		//#if 0

			//ImGui::ShowDemoWindow(&isDemoWindowOpen);

		//auto contentManagerState = OpenCAGELevelViewer::ContentManager::getContentManagerContext();

		/*ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), 0);*/

	#pragma region ImGui Start
		static bool openAboutMenu = false;
		static bool isSettingsMenuOpen = false;
		static bool isDebugMenuOpen = true;

		static int profileToModify = -1;

		{
			static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
			ImGuiWindowFlags window_flags = /*ImGuiWindowFlags_MenuBar | */ImGuiWindowFlags_NoDocking;

			const ImGuiViewport *viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

			bool dockspaceOpen = true;
			ImGui::Begin("Dockspace", &dockspaceOpen, window_flags);

			ImGuiID dockspace_id = ImGui::GetID("Dockspace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

			ImGui::PopStyleVar(2);

			ImGui::End();

			if (ImGui::BeginMainMenuBar()) {
				if (commandsEditorDependent.test()) {
					ImGui::BeginDisabled();
				}

				if (ImGui::BeginMenu("File")) {
					if (ImGui::BeginMenu("Profile")) {
						for (size_t i = 0; i < OpenCAGELevelViewer::AllInOne::Configuration::profileLength; i++) {
							ImGui::BeginDisabled(!modifiedProfile && !OpenCAGELevelViewer::AllInOne::Configuration::configuration.profile[i].exists);

							if (ImGui::MenuItem((std::string("Profile ") + std::to_string(i + 1)).data(), nullptr, nullptr)) {
								if (modifiedProfile) {
									profileToModify = i;
								} else
									OpenCAGELevelViewer::AllInOne::ContentManager::setGameRoot(OpenCAGELevelViewer::AllInOne::Configuration::configuration.profile[i].gamePath);
							}

							ImGui::EndDisabled();
						}

						ImGui::EndMenu();
					}

					if (ImGui::MenuItem("Game Root"))
						SDL_ShowOpenFolderDialog(setGameRootFolder, nullptr, sdlWindow, nullptr, false);

					if (ImGui::BeginMenu("Level")) {
						static std::vector < std::string > levels {};
						static size_t gameRootHash = 0;

						if (gameRootHash != OpenCAGELevelViewer::AllInOne::ContentManager::getGameRootHash()) {
							levels = OpenCAGELevelViewer::AllInOne::ContentManager::getAllLevels();
							gameRootHash = OpenCAGELevelViewer::AllInOne::ContentManager::getGameRootHash();
						}

						//static std::string currentLevel = OpenCAGELevelViewer::AllInOne::ContentManager::getLevel();

						for (const std::string &level : levels) {
							if (ImGui::MenuItem(level.data(), nullptr, stringHashFunction(level) == OpenCAGELevelViewer::AllInOne::ContentManager::getLevelHash())) {
								OpenCAGELevelViewer::AllInOne::ContentManager::setLevel(level);
							}
						}

						ImGui::EndMenu();
					}
					ImGui::EndMenu();
				}
				if (commandsEditorDependent.test()) {
					ImGui::EndDisabled();
				}

				if (ImGui::BeginMenu("Help")) {
					openAboutMenu = ImGui::MenuItem("About");
					ImGui::MenuItem("Settings", NULL, &isSettingsMenuOpen);

					ImGui::EndMenu();
				}

				ImGui::Separator();

				if (ImGui::BeginMenu("View")) {
					ImGui::MenuItem("Axis Arrows", nullptr, &OpenCAGELevelViewer::_3DView::axisArrows);

					ImGui::BeginDisabled(commandsEditorDependent.test());
					if (ImGui::BeginMenu("Commands Editor")) {
						ImGui::MenuItem("Commands Content", nullptr, &commandsContentWindowOpen);

						ImGui::EndMenu();
					}
					ImGui::EndDisabled();

					ImGui::EndMenu();
				}

				if (ImGui::MenuItem("Commands Editor Integration", nullptr, commandsEditorDependent.test())) {
					if (commandsEditorDependent.test())
						commandsEditorDependent.clear();
					else
						commandsEditorDependent.test_and_set();
				}

				ImGui::Separator();

				ImGui::MenuItem("Debug", nullptr, &isDebugMenuOpen);

				//static bool ceiFalseBool = false;

				//assert(ceiFalseBool == commandsEditorDependent.test());

				ImGui::EndMainMenuBar();
			}
		}

		if (openAboutMenu)
			ImGui::OpenPopup("About", 0);

		if (ImGui::BeginPopupModal("About", &openAboutMenu, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("OpenCAGE C++ Level Viewer");
			ImGui::Text("By ProjectHSI");
			ImGui::NewLine();
			ImGui::Text("Pre-Alpha");
			ImGui::NewLine();
			ImGui::Text("Made with CATHODELib by Matt Filer");
			ImGui::NewLine();
			ImGui::Text("Made with love for the OpenCAGE Community");

			ImGui::EndPopup();
		}

		if (profileToModify != -1)
			ImGui::OpenPopup("Save Profile?", ImGuiPopupFlags_NoReopen);

		if (ImGui::BeginPopupModal("Save Profile?")) {
			ImGui::Text("You have modified the current profile.\nDo you want to save it?\nIt will be saved to slot %i.", profileToModify + 1);

			if (ImGui::Button("Save")) {
				if (OpenCAGELevelViewer::AllInOne::ContentManager::getGameRoot().size() > OpenCAGELevelViewer::AllInOne::Configuration::gamePathLength)
					fatalError("If you're seeing this message, please file a bug report, your game root path is larger than the allowed value.");

				std::fill(OpenCAGELevelViewer::AllInOne::Configuration::configuration.profile[profileToModify].gamePath, OpenCAGELevelViewer::AllInOne::Configuration::configuration.profile[profileToModify].gamePath + OpenCAGELevelViewer::AllInOne::Configuration::gamePathLength, '\0');
				std::memcpy(OpenCAGELevelViewer::AllInOne::Configuration::configuration.profile[profileToModify].gamePath, OpenCAGELevelViewer::AllInOne::ContentManager::getGameRoot().data(), OpenCAGELevelViewer::AllInOne::ContentManager::getGameRoot().size() + 1);
				OpenCAGELevelViewer::AllInOne::Configuration::configuration.profile[profileToModify].exists = true;

				modifiedProfile = false;
				profileToModify = -1;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			ImGui::BeginDisabled(!OpenCAGELevelViewer::AllInOne::Configuration::configuration.profile[profileToModify].exists);
			if (ImGui::Button("Load")) {
				OpenCAGELevelViewer::AllInOne::ContentManager::setGameRoot(OpenCAGELevelViewer::AllInOne::Configuration::configuration.profile[profileToModify - 1].gamePath);

				modifiedProfile = false;
				profileToModify = -1;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndDisabled();
			ImGui::SameLine();
			if (ImGui::Button("Cancel")) {
				//modifiedProfile = false;
				profileToModify = -1;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		if (isSettingsMenuOpen) {
			if (ImGui::Begin("Settings", &isSettingsMenuOpen)) {
				//int swapInterval;
				//const char *swapIntervalString = "";

				SDL_GL_GetSwapInterval(&OpenCAGELevelViewer::AllInOne::Configuration::configuration.vsync);

				auto getSwapIntervalString = [](int swapInterval) -> const char * {
					switch (swapInterval) {
						case -1:
							return "Adaptive VSync";
						case 0:
							return "No Vertical Sync";
						case 1:
							return "Vertical Sync";
					}
					};

				if (ImGui::BeginCombo("VSync", getSwapIntervalString(OpenCAGELevelViewer::AllInOne::Configuration::configuration.vsync))) {
					for (int8_t i = -1; i <= 1; i++) {
						if (ImGui::Selectable(getSwapIntervalString(i), OpenCAGELevelViewer::AllInOne::Configuration::configuration.vsync == i))
							SDL_GL_SetSwapInterval(i);

						if (OpenCAGELevelViewer::AllInOne::Configuration::configuration.vsync == i)
							ImGui::SetItemDefaultFocus();
					}

					ImGui::EndCombo();
				}
			}
			ImGui::End();
		}

		if (isDebugMenuOpen) {
			if (ImGui::Begin("Debug", &isDebugMenuOpen, ImGuiWindowFlags_::ImGuiWindowFlags_AlwaysAutoResize)) {
				if (ImGui::BeginTabBar("DebugMenu")) {
					if (ImGui::BeginTabItem("ContentManager")) {
						ImGui::EndTabItem();
					}

					if (ImGui::BeginTabItem("OpenGL")) {
						static bool openglStatsDone = false;
						static int openglVersionMajor = 0;
						static int openglVersionMinor = 0;
						static int openglMaxVertexAttributes = 0;
						static int openglMaxTextureSize = 0;
						static int openglMaxTextureArrayLayers = 0;

						{
							if (!openglStatsDone) {
								openglVersionMajor = gotMajorVersion;
								openglVersionMinor = gotMinorVersion;
								glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &openglMaxVertexAttributes);
								glGetIntegerv(GL_MAX_TEXTURE_SIZE, &openglMaxTextureSize);
								glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &openglMaxTextureArrayLayers);
								openglStatsDone = true;
							}
						}

						ImGui::Text("OpenGL Version: %i.%i", openglVersionMajor, openglVersionMinor);
						ImGui::Text("Max Vertex Attributes: %i", openglMaxVertexAttributes);
						ImGui::Text("Max Texture Size: %i(^2)", openglMaxTextureSize);
						ImGui::Text("Max Texture Array Layers: %i", openglMaxTextureArrayLayers);

						//ImGui::Checkbox("");
						if (ImGui::BeginCombo("Scene Strategy", OpenCAGELevelViewer::AllInOne::Handoff::sceneRenderingStrategy[OpenCAGELevelViewer::AllInOne::Handoff::currentSceneRenderingStrategy].second.data())) {
							for (uint8_t i = 0; i < OpenCAGELevelViewer::AllInOne::Handoff::sceneRenderingStrategy.size(); i++) {
								if (ImGui::Selectable(OpenCAGELevelViewer::AllInOne::Handoff::sceneRenderingStrategy[i].second.data(), OpenCAGELevelViewer::AllInOne::Handoff::currentSceneRenderingStrategy == i))
									OpenCAGELevelViewer::AllInOne::Handoff::currentSceneRenderingStrategy = static_cast < OpenCAGELevelViewer::AllInOne::Handoff::SceneRenderingStrategy >(i);

								//switch (i) {
								//	case 0:
								//		ImGui::SetTooltip("Batches the entire (A:I) scene into a single draw call.\nThis is the fastest option, and should generally be kept onto this unless you're diagnosing a problem with batching.");
								//		break;
								//	case 1:
								//		ImGui::SetTooltip("Batches a-like models together.\nThis is the \"OK\" option.");
								//		break;
								//	case 2:
								//		ImGui::SetTooltip("Doesn't batch at all.\nNot recommended as it takes time to send data from the CPU to the GPU. The models don't need to be sent every time, but the data does.");
								//		break;
								//}

								if (OpenCAGELevelViewer::AllInOne::Handoff::currentSceneRenderingStrategy == i)
									ImGui::SetItemDefaultFocus();
							}
							
							ImGui::EndCombo();
						}
						
						ImGui::EndTabItem();
					}

					ImGui::EndTabBar();
				}
				//ImGui::Text("This is debug text.");
			}
			ImGui::End();
		}

		// Commands Content
		renderCommandsContentWindow();

		// Composite Tree
		{
			//if (contentManagerState->unmanagedComposite.has_value()) {
			static bool composite_tree_open = true;
			if (ImGui::Begin("Composite Tree", &composite_tree_open, ImGuiWindowFlags_HorizontalScrollbar)) {
				/*
				if (contentManagerState->unmanagedComposite.has_value()) {
					if (contentManagerState->unmanagedCompositeMutex.try_lock()) {
						std::lock_guard unmanagedCompositeLockGuard(contentManagerState->unmanagedCompositeMutex);

						contentManagerState->unmanagedCompositeMutex.unlock();

						std::function<void(OpenCAGELevelViewer::ContentManager::UnmanagedComposite &)> renderEntity;

						std::optional<OpenCAGELevelViewer::ContentManager::UnmanagedModelReference> _unmanagedModelReference = unmanagedModelReference;

						renderEntity = [&_unmanagedModelReference](OpenCAGELevelViewer::ContentManager::UnmanagedComposite &unmanagedComposite) {
							for (auto &newEntity : unmanagedComposite.unmanagedEntityChildren) {
								// Assign a type.
								if (std::holds_alternative<OpenCAGELevelViewer::ContentManager::UnmanagedModelReference>(newEntity)) {
									if (ImGui::TreeNodeEx(("Function Entity \"" + std::get< OpenCAGELevelViewer::ContentManager::UnmanagedModelReference >(newEntity).name + "\" (ModelReference)").c_str())) {
										//ImGui::CollapsingHeader()
										ImGui::InputDouble("Position X", &unmanagedComposite.transform.position.x);
										ImGui::InputDouble("Position Y", &unmanagedComposite.transform.position.y);
										ImGui::InputDouble("Position Z", &unmanagedComposite.transform.position.z);
										ImGui::InputDouble("Rotation X", &unmanagedComposite.transform.rotation.x);
										ImGui::InputDouble("Rotation Y", &unmanagedComposite.transform.rotation.y);
										ImGui::InputDouble("Rotation Z", &unmanagedComposite.transform.rotation.z);
										//if (ImGui::Button("Select")) {
											//_unmanagedModelReference = std::get< OpenCAGELevelViewer::ContentManager::UnmanagedModelReference >(newEntity);
											//OpenCAGELevelViewer::_3DView::UsuableUnmanagedObjects newEntityInVariant = std::get< OpenCAGELevelViewer::ContentManager::UnmanagedModelReference >(newEntity);
											//std::optional < OpenCAGELevelViewer::_3DView::UsuableUnmanagedObjects > newEntityInVariantOptional = newEntityInVariant;
										//}

										ImGui::TreePop();
									}
								}
							}
							};

						std::function<void(OpenCAGELevelViewer::ContentManager::UnmanagedComposite &)> renderComposite;

						renderComposite = [&renderEntity, &renderComposite](OpenCAGELevelViewer::ContentManager::UnmanagedComposite &unmanagedComposite) {
							if (ImGui::TreeNodeEx(("Composite \"" + unmanagedComposite.name + "\" (\"" + unmanagedComposite.instanceType + "\" @ " + unmanagedComposite.shortGuid + ")").c_str())) {
								//ImGui::CollapsingHeader()
								ImGui::InputDouble("Position X", &unmanagedComposite.transform.position.x);
								ImGui::InputDouble("Position Y", &unmanagedComposite.transform.position.y);
								ImGui::InputDouble("Position Z", &unmanagedComposite.transform.position.z);
								ImGui::InputDouble("Rotation X", &unmanagedComposite.transform.rotation.x);
								ImGui::InputDouble("Rotation Y", &unmanagedComposite.transform.rotation.y);
								ImGui::InputDouble("Rotation Z", &unmanagedComposite.transform.rotation.z);
								//ImGui::Button("Select");

								for (auto &newComposite : unmanagedComposite.unmanagedCompositeChildren) {
									renderComposite(newComposite);
								}

								renderEntity(unmanagedComposite);

								ImGui::TreePop();
							}
							};

						if (ImGui::TreeNodeEx(("Selected Composite (\"" + contentManagerState->unmanagedComposite->instanceType + "\" @ " + contentManagerState->unmanagedComposite->shortGuid + ")").c_str())) {
							for (auto &newComposite : contentManagerState->unmanagedComposite->unmanagedCompositeChildren) {
								renderComposite(newComposite);
							}

							renderEntity(contentManagerState->unmanagedComposite.value());

							ImGui::TreePop();
						}

						unmanagedModelReference = _unmanagedModelReference;
					}
				}
				*/
			}
			ImGui::End();
			//}
		}

		static bool wasReadyLastFrame = true;

		if (commandsEditorDependent.test() && !OpenCAGELevelViewer::WebsocketThread::ready.test()) {
			ImGui::OpenPopup("Connecting", ImGuiPopupFlags_::ImGuiPopupFlags_NoReopen);

			if (ImGui::BeginPopupModal("Connecting", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
				if (OpenCAGELevelViewer::WebsocketThread::connected.test()) {
					wasReadyLastFrame = true;
					ImGui::Text("Handshaking...");
				} else {
					if (wasReadyLastFrame) {
						wasReadyLastFrame = false;
						timeSinceConnectDialogOpen = SteadyClock::now();
					}

					ImGui::Text("Connecting to the commands editor...");

					//std::cout << (SteadyClock::now() - timeSinceConnectDialogOpen) << std::endl;

					if ((SteadyClock::now() - timeSinceConnectDialogOpen) / 5s >= 1) {
						ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
						ImGui::TextWrapped("Make sure that the Commands Editor is running and that \"Options\" -> \"Level Viewer\" -> \"Connect To Level Viewer\" is selected.");
						ImGui::PopStyleColor();
					}

					if (ImGui::Button("Disable Commands Editor Integration")) {
						commandsEditorDependent.clear();
						//OpenCAGELevelViewer::WebsocketThread::willConnect.clear();
					}
				}

				if (OpenCAGELevelViewer::WebsocketThread::ready.test()) {
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}
		} else {
			wasReadyLastFrame = false;

			/*
			if (contentManagerState->contentManagerState.contentManagerState != OpenCAGELevelViewer::ContentManager::ContentManagerStateEnum::READY) {
				ImGui::OpenPopup(contentManagerState->contentManagerState.contentManagerState == OpenCAGELevelViewer::ContentManager::ContentManagerStateEnum::NO_LEVEL ? "No Level Loaded" : "Loading...", ImGuiPopupFlags_::ImGuiPopupFlags_NoReopen);

				if (ImGui::BeginPopupModal("No Level Loaded", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
					ImGui::Text("No level loaded!");
					ImGui::TextWrapped("Make sure that the commands editor has a level loaded.");

					if (contentManagerState->contentManagerState.contentManagerState != OpenCAGELevelViewer::ContentManager::ContentManagerStateEnum::NO_LEVEL) {
						ImGui::CloseCurrentPopup();
					}

					ImGui::EndPopup();
				}

				if (ImGui::BeginPopupModal("Loading...", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
					ImGui::ProgressBar(contentManagerState->contentManagerState.progress == -1 ? -1.0f * ( float ) ImGui::GetTime() * 1 : contentManagerState->contentManagerState.progress, ImVec2(350.0f, 0.0f), contentManagerState->contentManagerState.progressText.data());
					//ImGui::Text("No level loaded!");
					//ImGui::TextWrapped("Make sure that the commands editor has a level loaded.");

					if (contentManagerState->contentManagerState.contentManagerState == OpenCAGELevelViewer::ContentManager::ContentManagerStateEnum::READY) {
						ImGui::CloseCurrentPopup();
					}

					ImGui::EndPopup();
				}
			} else {
				if (!openGlLoadingThreadIsDone.test()) {
					ImGui::OpenPopup("Buffering OpenGL", ImGuiPopupFlags_::ImGuiPopupFlags_NoReopen);
				}
				if (ImGui::BeginPopupModal("Buffering OpenGL", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
					ImGui::ProgressBar(-1.0f * ( float ) ImGui::GetTime() * 1, ImVec2(350.0f, 0.0f), "Buffering OpenGL...");

					if (openGlLoadingThreadIsDone.test()) {
						ImGui::CloseCurrentPopup();
					}

					ImGui::EndPopup();
				}
			}
			*/
		}

		//static int currentResolution[2] = {640, 480};
		// Do 2^msaaSamples to get the MSAA sample count. (1x, 2x, 4x, 8x)
		static int msaaSamples = 0;

		static _3DViewConfiguration _3dviewConfiguration {};

		{
			if (ImGui::Begin("3D View Configuration")) {
				ImGui::Text(_3dViewLockInput ? "! Press Right Control to Unlock UI !" : "");

				constexpr const char *msaaSamplesNames[] = {"1x (Off)", "2x", "4x", "8x"};
				const char *msaaSampleName = msaaSamplesNames[msaaSamples];

				ImGui::SliderFloat("FOV", &OpenCAGELevelViewer::_3DView::fov, 30, 120, "%.3f", 0);
				ImGui::SliderFloat("Mouse Sensitivity", &OpenCAGELevelViewer::_3DView::mouseSensitivity, 0.25, 10, "%.3f", ImGuiSliderFlags_Logarithmic);

				ImGui::BeginDisabled();
				ImGui::SliderInt("MSAA Antialiasing", &msaaSamples, 0, 3, msaaSampleName);
				ImGui::SetItemTooltip("Antialiasing has not yet been implemented.");
				ImGui::EndDisabled();

				ImGui::Checkbox("Use Model LODs", &_3dviewConfiguration.useLods);
				if (ImGui::BeginCombo("Vertex Colour Mode", OpenCAGELevelViewer::_3DView::vertexColourModeNames[OpenCAGELevelViewer::_3DView::vertexColourMode].second)) {
					for (const auto &vertexColourMode : OpenCAGELevelViewer::_3DView::vertexColourModeNames) {
						if (ImGui::Selectable(vertexColourMode.second, OpenCAGELevelViewer::_3DView::vertexColourMode == vertexColourMode.first)) {
							OpenCAGELevelViewer::_3DView::vertexColourMode = vertexColourMode.first;
						}
						if (OpenCAGELevelViewer::_3DView::vertexColourMode == vertexColourMode.first)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
				ImGui::Checkbox("Ignore Col W (Alpha)", &OpenCAGELevelViewer::_3DView::ignoreColW);
				//ImGui::SliderInt();
				//ImGui::

				/*static int currentSelection = 0;
				static float aspectRatio[2] = {4, 3};*/

				//ImGui::InputInt2("Resolution", currentResolution);

				//if (ImGui::BeginCombo("Resolution", "test", ImGuiComboFlags_WidthFitPreview)) {
				//	ImGui::Selectable("640x")
				//	//ImGui::Selectable("Test");

				//	//for (int n = 0; n < IM_ARRAYSIZE(items); n++) {
				//	//	const bool is_selected = (item_current_idx == n);
				//	//	if (ImGui::Selectable(items[n], is_selected))
				//	//		item_current_idx = n;

				//	//	// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
				//	//	if (is_selected)
				//	//		ImGui::SetItemDefaultFocus();
				//	//}
				//	ImGui::EndCombo();
				//}
			}

			ImGui::End();

			//#if 0
			//if (contentManagerState->newCompositeLoaded.test() && openGlLoadingThreadIsDone.test()) {
				//openGlLoadingThreadIsDone.clear();
				//contentManagerState->newCompositeLoaded.clear();

				//OpenCAGELevelViewer::_3DView::UsuableUnmanagedObjects newCompositeInVariant = contentManagerState->unmanagedComposite.value();
				//std::optional < OpenCAGELevelViewer::_3DView::UsuableUnmanagedObjects > newCompositeInVariantOptional = newCompositeInVariant;

				//std::atomic_flag &localThreadIsDone = openGlLoadingThreadIsDone;

				//std::thread _openGlLoadingThreadTemp = std::thread([newCompositeInVariantOptional, &localThreadIsDone]() { OpenCAGELevelViewer::_3DView::UpdateScene(newCompositeInVariantOptional, localThreadIsDone); });

				//openGlLoadingThread.swap(_openGlLoadingThreadTemp);

				//OpenCAGELevelViewer::_3DView::UpdateScene(newCompositeInVariantOptional, openGlLoadingThreadIsDone);

				//if (_openGlLoadingThreadTemp.joinable())
					//_openGlLoadingThreadTemp.detach();

				//contentManagerState->newCompositeLoaded.clear();
			//}
			//#endif
		}
	#pragma endregion

	#pragma region OpenGL
		{
			if (ImGui::Begin("3D View")) {
				//if (openGlLoadingThreadIsDone.test()) {
					if (ImGui::BeginChild("3DViewRender")) {
						ImVec2 wsize = ImGui::GetWindowSize();

						ImVec2 currentCursor = ImGui::GetCursorPos();

						//glBindTexture(GL_TEXTURE_2D, 0);

						ImVec2 clickPos = ImVec2(0.5f, 0.5f);
						if (ImGui::InvisibleButton("3DViewRenderPhantom", wsize, ImGuiButtonFlags_::ImGuiButtonFlags_MouseButtonLeft)) {
							if (ImGui::IsKeyDown(ImGuiKey::ImGuiMod_Ctrl)) {
								clickPos = (ImGui::GetMousePos() - ImGui::GetItemRectMin()) / ImGui::GetItemRectSize();
								was3dViewSelectRequested = true;
							} else {
								_3dViewLockInput = true;

								ImGuiIO &io = ImGui::GetIO(); ( void ) io;
								io.ConfigFlags &= (0xFFFFFFFF ^ ImGuiConfigFlags_NavEnableKeyboard);
								io.ConfigFlags &= (0xFFFFFFFF ^ ImGuiConfigFlags_NavEnableGamepad);
								io.ConfigFlags |= ImGuiConfigFlags_NoMouse;

								SDL_SetWindowRelativeMouseMode(sdlWindow, true);
							}
						}
						ImGui::SetCursorPos(currentCursor);
						ImGui::SetNextItemAllowOverlap();

						OpenCAGELevelViewer::_3DView::Render(wsize);

						if (was3dViewSelectRequested) {
							const int64_t selectedInstanceId = OpenCAGELevelViewer::_3DView::getUserSelectedInstanceId(wsize, clickPos);
							if (selectedInstanceId != -1) {
								debugTestDelegate(selectedInstanceId);
								//auto  debug.first.operator->() << std::endl;
							}
						}

						ImGui::Image(( ImTextureID ) OpenCAGELevelViewer::_3DView::getFbo(), wsize, ImVec2(0, 1), ImVec2(1, 0));
					}
					ImGui::EndChild();
				//}

				/*int w, h;
				SDL_GL_GetDrawableSize(sdlWindow, &w, &h);
				glViewport(0, 0, w, h);*/
			}

			ImGui::End();
		}
	#pragma endregion

		/*constexpr std::vector<int> resolutionWidths = {
			640,
			1440,
			1920,
			2880,
			3160
		}*/

		//OpenCAGELevelViewer::_3DView::Render(/*wsize*/);
	//#endif


	#pragma region ImGui End

	#pragma endregion

		ImGui::ShowDemoWindow();

		glViewport(0, 0, ( int ) io.DisplaySize.x, ( int ) io.DisplaySize.y);
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ImGui::Render();

		//SDL_RenderClear(sdlRenderer);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glFlush();

		SDL_GL_SwapWindow(sdlWindow);

		lastFrameTime = currentFrameTime;
		currentFrameTime = std::chrono::duration_cast< std::chrono::microseconds >(std::chrono::system_clock::now().time_since_epoch()).count();

		//std::cout << currentFrameTime - lastFrameTime << std::endl;
		//SDL_RenderPresent(sdlRenderer);
	}

	OpenCAGELevelViewer::_3DView::Quit();

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();

	//SDL_DestroyRenderer(sdlRenderer);
	SDL_GL_DestroyContext(gl_context);
	SDL_DestroyWindow(sdlWindow);

	OpenCAGELevelViewer::AllInOne::Configuration::save();

	SDL_Quit();

	OpenCAGELevelViewer::WebsocketThread::keepThreadActive.clear();
	OpenCAGELevelViewer::WebsocketThread::willConnect.clear();

	if (websocketThread.joinable()) {
		try {
			websocketThread.join();
		} catch (...) {
			// std::thread likes to be in a quauntum superposition sometimes...
			__debugbreak();
		}
	}

	if (websocketThread.joinable()) {
		// STILL?
		websocketThread.detach();
		__debugbreak();
	}

	return 0;
}
#pragma managed(pop)