//#include "OpenCAGELevelViewer.h"
#include "handoff.h"

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <imgui_impl_opengl3.h>
#include <thread>
#include <chrono>
#include <iostream>
#include "WebsocketThread.h"
//#include <ContentManager.h> /* TODO: Port */
#include <functional>

#include <glbinding/glbinding.h>
#include <glbinding/Version.h>
#include <glbinding/FunctionCall.h>
#include <glbinding/CallbackMask.h>

#include <glbinding/gl/gl.h>
#include <glbinding/getProcAddress.h>

#include "3DView.h"

using namespace gl;
using namespace std::chrono_literals;

SDL_Window *sdlWindow;
SDL_Renderer *sdlRenderer;

void fatalSdlError(std::string baseErrorMessage) {
	std::string errorMessage = baseErrorMessage + "\n\n" + SDL_GetError() + "\n\n" + "OpenCAGE C++ Level Viewer will now exit.";

	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "OpenCAGE C++ Level Viewer", errorMessage.data(), NULL);

	exit(1);
}

static bool isDemoWindowOpen = true;

struct _3DViewConfiguration {
	int msaaSamples;
	bool useLods;
};

typedef std::chrono::steady_clock SteadyClock;
typedef std::chrono::time_point < SteadyClock > SteadyClockTimePoint;
SteadyClockTimePoint timeSinceConnectDialogOpen = SteadyClockTimePoint();

int handoff(char **argv, int argc) {
	//initalize_contentManager(); // let CoreCLR sort itself out, if needed.
	// I'm not sure if this is needed anymore, but we'll keep it anyway if there's any surprises.

	//openGlLoadingThreadIsDone.test_and_set();

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMEPAD) < 0) {
		fatalSdlError("SDL3 was unable to initalize one or more of its subsystems.");
	};

	sdlWindow = SDL_CreateWindow("OpenCAGE Level Viewer", 1600, 1200, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

	if (!sdlWindow) {
		fatalSdlError("SDL3 was unable to create a window.");
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
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

	SDL_GL_MakeCurrent(sdlWindow, gl_context);
	SDL_GL_SetSwapInterval(1);

	glbinding::initialize(reinterpret_cast< glbinding::ProcAddress(*)(const char *) >(SDL_GL_GetProcAddress), true);
	//glbinding::initialize(SDL_GL_GetProcAddress);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO(); ( void ) io;
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
	OpenCAGELevelViewer::WebsocketThread::willConnect.test_and_set();

	//fv::Tasks::Init();
	//fv::Tasks::RunAsync(OpenCAGELevelViewer::WebsocketThread::main);
	//fv::Tasks::Run();
	std::atomic_flag suspendFlag;
	suspendFlag.test_and_set();

	std::thread websocketThread(OpenCAGELevelViewer::WebsocketThread::main);
	//std::thread contentManagerThread([&suspendFlag]() { OpenCAGELevelViewer::ContentManager::contentManagerThread(suspendFlag); });

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
		static bool isForwardPressed = false;
		static bool isLeftPressed = false;
		static bool isRightPressed = false;
		static bool isBackwardPressed = false;
		static bool isUpPressed = false;
		static bool isDownPressed = false;

		static bool isShiftPressed = false;
		static bool isCtrlPressed = false;

		int32_t xMouse = 0;
		int32_t yMouse = 0;

		float yScroll = 0;

		SDL_Event event;

		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL3_ProcessEvent(&event);

			switch (event.type) {
				case SDL_EVENT_QUIT:
					userRequestedExit = true;
					break;
				case SDL_EVENT_MOUSE_MOTION:
					if (_3dViewLockInput) {
						xMouse = event.motion.xrel;
						yMouse = event.motion.yrel;
					}
					break;
				case SDL_EVENT_MOUSE_WHEEL:
					if (_3dViewLockInput) {
						yScroll = event.wheel.y;
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

							case SDLK_Q:
								isUpPressed = true;
								break;
							case SDLK_E:
								isDownPressed = true;
								break;
							case SDLK_W:
								isForwardPressed = true;
								break;
							case SDLK_A:
								isLeftPressed = true;
								break;
							case SDLK_D:
								isRightPressed = true;
								break;
							case SDLK_S:
								isBackwardPressed = true;
								break;
							case SDLK_LSHIFT:
								isShiftPressed = true;
								break;
							case SDLK_LCTRL:
								isCtrlPressed = true;
								break;
						}
					}
					break;
				case SDL_EVENT_KEY_UP:
					if (_3dViewLockInput) {
						switch (event.key.key) {
							case SDLK_Q:
								isUpPressed = false;
								break;
							case SDLK_E:
								isDownPressed = false;
								break;
							case SDLK_W:
								isForwardPressed = false;
								break;
							case SDLK_A:
								isLeftPressed = false;
								break;
							case SDLK_D:
								isRightPressed = false;
								break;
							case SDLK_S:
								isBackwardPressed = false;
								break;
							case SDLK_LSHIFT:
								isShiftPressed = false;
								break;
							case SDLK_LCTRL:
								isCtrlPressed = false;
								break;
						}
					}
					break;
			}

			if (event.type == SDL_EVENT_QUIT)
				userRequestedExit = true;
		}

		OpenCAGELevelViewer::_3DView::updateCamera(isRightPressed - isLeftPressed, isUpPressed - isDownPressed, isForwardPressed - isBackwardPressed, 0, xMouse, yMouse, yScroll, isShiftPressed, isCtrlPressed, static_cast< float >(currentFrameTime - lastFrameTime) / std::chrono::microseconds::period::den);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();
		//#if 0

			//ImGui::ShowDemoWindow(&isDemoWindowOpen);

		//auto contentManagerState = OpenCAGELevelViewer::ContentManager::getContentManagerContext();

		/*ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), 0);*/

	#pragma region ImGui Start
		{
			static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
			ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

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
		}

		//static std::optional<OpenCAGELevelViewer::ContentManager::UnmanagedModelReference> unmanagedModelReference;

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

		if (!OpenCAGELevelViewer::WebsocketThread::ready.test()) {
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

				ImGui::BeginDisabled();
				ImGui::SliderInt("MSAA Antialiasing", &msaaSamples, 0, 3, msaaSampleName);
				ImGui::SetItemTooltip("Antialiasing has not yet been implemented.");
				ImGui::EndDisabled();

				ImGui::Checkbox("Use Model LODs", &_3dviewConfiguration.useLods);
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

						OpenCAGELevelViewer::_3DView::Render(wsize);

						//glBindTexture(GL_TEXTURE_2D, 0);

						ImGui::Image(( ImTextureID ) OpenCAGELevelViewer::_3DView::getFbo(), wsize, ImVec2(0, 1), ImVec2(1, 0));
						ImGui::SetCursorPos(currentCursor);
						ImGui::SetNextItemAllowOverlap();

						if (ImGui::InvisibleButton("3DViewRenderPhantom", wsize, ImGuiButtonFlags_::ImGuiButtonFlags_MouseButtonLeft)) {
							_3dViewLockInput = true;

							ImGuiIO &io = ImGui::GetIO(); ( void ) io;
							io.ConfigFlags &= (0xFFFFFFFF ^ ImGuiConfigFlags_NavEnableKeyboard);
							io.ConfigFlags &= (0xFFFFFFFF ^ ImGuiConfigFlags_NavEnableGamepad);
							io.ConfigFlags |= ImGuiConfigFlags_NoMouse;

							SDL_SetWindowRelativeMouseMode(sdlWindow, true);
						}
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