// OpenCAGELevelViewer.cpp : Defines the entry point for the application.
//

#define SDL_MAIN_HANDLED

#include "OpenCAGELevelViewer.h"
#include <SDL2/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <thread>
#include "WebsocketThread.h"
#include <ContentManager.h>


SDL_Window *sdlWindow;
SDL_Renderer *sdlRenderer;

void fatalSdlError(std::string baseErrorMessage) {
	std::string errorMessage = baseErrorMessage + "\n\n" + SDL_GetError() + "\n\n" + "OpenCAGE C++ Level Viewer will now exit.";

	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "OpenCAGE C++ Level Viewer", errorMessage.data(), NULL);

	exit(1);
}

static bool isDemoWindowOpen = true;

int main()
{
	initalize_contentManager(); // let CoreCLR sort itself out

	std::cout << "Hello CMake." << std::endl;

	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		fatalSdlError("SDL2 was unable to initalize one or more of its subsystems.");
	};

	sdlWindow = SDL_CreateWindow("OpenCAGE Level Viewer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WindowFlags::SDL_WINDOW_OPENGL);

	if (!sdlWindow) {
		fatalSdlError("SDL2 was unable to create a window.");
	}

	sdlRenderer = SDL_CreateRenderer(sdlWindow, -1, SDL_RendererFlags::SDL_RENDERER_PRESENTVSYNC);

	if (!sdlRenderer) {
		fatalSdlError("SDL2 was unable to create a renderer.");
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO(); ( void ) io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplSDL2_InitForSDLRenderer(sdlWindow, sdlRenderer);
	ImGui_ImplSDLRenderer2_Init(sdlRenderer);

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

	while (!userRequestedExit) {
		SDL_Event event;

		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL2_ProcessEvent(&event);

			if (event.type == SDL_QUIT)
				userRequestedExit = true;
		}

		ImGui_ImplSDLRenderer2_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		//ImGui::ShowDemoWindow(&isDemoWindowOpen);

		// Composite Tree
		static bool composite_tree_open = true;
		if (ImGui::Begin("Composite Tree", &composite_tree_open)) {
			if (ImGui::TreeNodeEx("Test")) {
				ImGui::TreePop();
			}
		}
		ImGui::End();

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
			auto contentManagerState = OpenCAGELevelViewer::ContentManager::getContentManagerContext()->contentManagerState;

			if (contentManagerState.contentManagerState != OpenCAGELevelViewer::ContentManager::ContentManagerStateEnum::READY) {
				ImGui::OpenPopup(contentManagerState.contentManagerState == OpenCAGELevelViewer::ContentManager::ContentManagerStateEnum::NO_LEVEL ? "No Level Loaded" : "Loading...", ImGuiPopupFlags_::ImGuiPopupFlags_NoReopen);

				if (ImGui::BeginPopupModal("No Level Loaded", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
					ImGui::Text("No level loaded!");
					ImGui::TextWrapped("Make sure that the commands editor has a level loaded.");

					if (contentManagerState.contentManagerState != OpenCAGELevelViewer::ContentManager::ContentManagerStateEnum::NO_LEVEL) {
						ImGui::CloseCurrentPopup();
					}

					ImGui::EndPopup();
				}

				if (ImGui::BeginPopupModal("Loading...", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
					ImGui::ProgressBar(contentManagerState.progress == -1 ? -1.0f * ( float ) ImGui::GetTime() : contentManagerState.progress, ImVec2(350.0f, 0.0f), contentManagerState.progressText.data());
					//ImGui::Text("No level loaded!");
					//ImGui::TextWrapped("Make sure that the commands editor has a level loaded.");

					if (contentManagerState.contentManagerState == OpenCAGELevelViewer::ContentManager::ContentManagerStateEnum::READY) {
						ImGui::CloseCurrentPopup();
					}

					ImGui::EndPopup();
				}
			} else {

			}
		}

		ImGui::ShowDemoWindow();

		ImGui::Render();

		SDL_RenderClear(sdlRenderer);

		ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), sdlRenderer);

		SDL_RenderPresent(sdlRenderer);
	}

	ImGui_ImplSDLRenderer2_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_DestroyRenderer(sdlRenderer);
	SDL_DestroyWindow(sdlWindow);

	SDL_Quit();

	return 0;
}
