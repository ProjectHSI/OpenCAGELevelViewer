#pragma once
#include "imgui.h"
//#include "ContentManager.h"
#include <mutex>
#include <optional>

namespace OpenCAGELevelViewer {
	namespace _3DView {
		unsigned int getFbo(void);

		//extern std::recursive_mutex _3dViewMutex;

		/*struct _3DViewCamera {

		};*/

		//void updateCamera(int32_t x, int32_t y);
		void updateCamera(signed char x, signed char y, signed char z, signed char roll, int32_t mouseX, int32_t mouseY, float scrollY, bool isShiftPressed, bool isCtrlPressed, float deltaTime);
		
		//typedef std::variant < OpenCAGELevelViewer::ContentManager::UnmanagedComposite, OpenCAGELevelViewer::ContentManager::UnmanagedModelReference > UsuableUnmanagedObjects;

		void Initalise(void);
		void UpdateScene(/*std::optional < OpenCAGELevelViewer::_3DView::UsuableUnmanagedObjects > unmanagedObject, */std::atomic_flag &isDone);
		void Render(ImVec2 windowSize/*, std::optional<std::variant<OpenCAGELevelViewer::ContentManager::UnmanagedModelReference, OpenCAGELevelViewer::ContentManager::UnmanagedComposite>> unmanagedObject*/);
		///void Render(ImVec2 windowSize, std::optional<OpenCAGELevelViewer::ContentManager::UnmanagedModelReference> unmanagedModelReference);
		void Quit(void);
	}
}