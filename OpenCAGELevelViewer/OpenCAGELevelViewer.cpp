// OpenCAGELevelViewer.cpp : Defines the entry point for the application.
//

#define SDL_MAIN_HANDLED

#include "OpenCAGELevelViewer.h"
#include <SDL2/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <imgui_impl_opengl3.h>
#include <thread>
#include "WebsocketThread.h"
#include <ContentManager.h>
#include <functional>

#include <glbinding/glbinding.h>
#include <glbinding/Version.h>
#include <glbinding/FunctionCall.h>
#include <glbinding/CallbackMask.h>

#include <glbinding/gl/gl.h>
#include <glbinding/getProcAddress.h>

#include <glbinding/gl/gl.h>
using namespace gl;

#include "3DView.h"

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

int main()
{
	initalize_contentManager(); // let CoreCLR sort itself out

	std::cout << "Hello CMake." << std::endl;

	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		fatalSdlError("SDL2 was unable to initalize one or more of its subsystems.");
	};

	sdlWindow = SDL_CreateWindow("OpenCAGE Level Viewer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1600, 1200, SDL_WindowFlags::SDL_WINDOW_OPENGL | SDL_WindowFlags::SDL_WINDOW_RESIZABLE);

	if (!sdlWindow) {
		fatalSdlError("SDL2 was unable to create a window.");
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

	glbinding::initialize(reinterpret_cast< glbinding::ProcAddress(*)(const char *) >(SDL_GL_GetProcAddress), false);
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
	//ImGui::StyleColorsDark();
	ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	//ImGui_ImplSDL2_InitForSDLRenderer(sdlWindow, sdlRenderer);
	ImGui_ImplSDL2_InitForOpenGL(sdlWindow, SDL_GL_GetCurrentContext());
	ImGui_ImplOpenGL3_Init();
	//ImGui_ImplSDLRenderer2_Init(sdlRenderer);

	OpenCAGELevelViewer::WebsocketThread::keepThreadActive.test_and_set();
	OpenCAGELevelViewer::WebsocketThread::connect.test_and_set();

	//fv::Tasks::Init();
	//fv::Tasks::RunAsync(OpenCAGELevelViewer::WebsocketThread::main);
	//fv::Tasks::Run();
	std::atomic_flag suspendFlag;
	suspendFlag.test_and_set();

	std::thread websocketThread(OpenCAGELevelViewer::WebsocketThread::main);
	std::thread contentManagerThread([&suspendFlag]() { OpenCAGELevelViewer::ContentManager::contentManagerThread(suspendFlag); });

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

	while (!userRequestedExit) {
		SDL_Event event;

		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL2_ProcessEvent(&event);

			switch (event.type) {
				case SDL_QUIT:
					userRequestedExit = true;
					break;
				case SDL_MOUSEMOTION:
					if (_3dViewLockInput) {
						// do something with event.motion.xrel and yrel;
						OpenCAGELevelViewer::_3DView::updateCamera(event.motion.xrel, event.motion.yrel);
					}
				case SDL_KEYDOWN:
					if (_3dViewLockInput) {
						switch (event.key.keysym.sym) {
							case SDL_KeyCode::SDLK_RCTRL:
								{
									_3dViewLockInput = false;

									ImGuiIO &io = ImGui::GetIO(); ( void ) io;
									io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
									io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
									io.ConfigFlags &= (0xFFFFFFFF ^ ImGuiConfigFlags_NoMouse);
								}
								break;
								
							case SDL_KeyCode::SDLK_q:
								// roll camera to left
								break;
							case SDL_KeyCode::SDLK_e:
								// roll camera to right
								break;
							case SDL_KeyCode::SDLK_w:
								OpenCAGELevelViewer::_3DView::updateCamera(0, 0, 1, 0);
								break;
							case SDL_KeyCode::SDLK_a:
								OpenCAGELevelViewer::_3DView::updateCamera(0, 1, 0, 0);
								break;
							case SDL_KeyCode::SDLK_d:
								OpenCAGELevelViewer::_3DView::updateCamera(0, -1, 0, 0);
								break;
							case SDL_KeyCode::SDLK_s:
								OpenCAGELevelViewer::_3DView::updateCamera(0, 0, -1, 0);
								break;
						}
					}
			}

			if (event.type == SDL_QUIT)
				userRequestedExit = true;
		}

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		//ImGui::ShowDemoWindow(&isDemoWindowOpen);

		auto contentManagerState = OpenCAGELevelViewer::ContentManager::getContentManagerContext();

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

		static std::optional<OpenCAGELevelViewer::ContentManager::UnmanagedModelReference> unmanagedModelReference;

		// Composite Tree
		{
			

			//if (contentManagerState->unmanagedComposite.has_value()) {
				static bool composite_tree_open = true;
				if (ImGui::Begin("Composite Tree", &composite_tree_open, ImGuiWindowFlags_HorizontalScrollbar)) {
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
											/*ImGui::InputDouble("Position X", &unmanagedComposite.transform.position.x);
											ImGui::InputDouble("Position Y", &unmanagedComposite.transform.position.y);
											ImGui::InputDouble("Position Z", &unmanagedComposite.transform.position.z);
											ImGui::InputDouble("Rotation X", &unmanagedComposite.transform.rotation.x);
											ImGui::InputDouble("Rotation Y", &unmanagedComposite.transform.rotation.y);
											ImGui::InputDouble("Rotation Z", &unmanagedComposite.transform.rotation.z);*/
											if (ImGui::Button("Select")) {
												_unmanagedModelReference = std::get< OpenCAGELevelViewer::ContentManager::UnmanagedModelReference >(newEntity);
											}

											ImGui::TreePop();
										}
									}
								}
								};

							std::function<void(OpenCAGELevelViewer::ContentManager::UnmanagedComposite &)> renderComposite;

							renderComposite = [&renderEntity, &renderComposite](OpenCAGELevelViewer::ContentManager::UnmanagedComposite &unmanagedComposite) {
								if (ImGui::TreeNodeEx(("Composite \"" + unmanagedComposite.name + "\" (\"" + unmanagedComposite.instanceType + "\" @ " + unmanagedComposite.shortGuid + ")").c_str())) {
									//ImGui::CollapsingHeader()
									/*ImGui::InputDouble("Position X", &unmanagedComposite.transform.position.x);
									ImGui::InputDouble("Position Y", &unmanagedComposite.transform.position.y);
									ImGui::InputDouble("Position Z", &unmanagedComposite.transform.position.z);
									ImGui::InputDouble("Rotation X", &unmanagedComposite.transform.rotation.x);
									ImGui::InputDouble("Rotation Y", &unmanagedComposite.transform.rotation.y);
									ImGui::InputDouble("Rotation Z", &unmanagedComposite.transform.rotation.z);*/
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
				}
				ImGui::End();
			//}
		}

		if (!OpenCAGELevelViewer::WebsocketThread::ready.test()) {
			ImGui::OpenPopup("Connecting", ImGuiPopupFlags_::ImGuiPopupFlags_NoReopen);

			if (ImGui::BeginPopupModal("Connecting", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
				if (OpenCAGELevelViewer::WebsocketThread::connected.test()) {
					ImGui::Text("Handshaking...");
				} else {
					ImGui::Text("Connecting to the commands editor...");
					ImGui::TextWrapped("Make sure that the commands editor is running and that \"Connect To Unity\" is selected.");
				}

				if (OpenCAGELevelViewer::WebsocketThread::ready.test()) {
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}
		} else {
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

			}
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
		}
	#pragma endregion

	#pragma region OpenGL
		{
			if (ImGui::Begin("3D View")) {
				if (ImGui::BeginChild("3DViewRender")) {
					ImVec2 wsize = ImGui::GetWindowSize();

					ImVec2 currentCursor = ImGui::GetCursorPos();

					OpenCAGELevelViewer::_3DView::Render(wsize, unmanagedModelReference);

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
					}

					ImGui::EndChild();
				}

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
		

	#pragma region ImGui End
		
	#pragma endregion

		ImGui::ShowDemoWindow();

		glViewport(0, 0, ( int ) io.DisplaySize.x, ( int ) io.DisplaySize.y);
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui::Render();

		//SDL_RenderClear(sdlRenderer);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		SDL_GL_SwapWindow(sdlWindow);
		//SDL_RenderPresent(sdlRenderer);
	}

	OpenCAGELevelViewer::_3DView::Quit();

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	//SDL_DestroyRenderer(sdlRenderer);
	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(sdlWindow);

	SDL_Quit();

	return 0;
}
