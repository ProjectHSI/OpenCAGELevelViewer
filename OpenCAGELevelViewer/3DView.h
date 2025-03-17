#pragma once
#include "imgui.h"
#include <ContentManager.h>
#include <mutex>
#include <optional>

namespace OpenCAGELevelViewer {
	namespace _3DView {
		unsigned int getFbo(void);

		//extern std::recursive_mutex _3dViewMutex;

		/*struct _3DViewCamera {

		};*/

		void updateCamera(int32_t x, int32_t y);
		void updateCamera(signed char x, signed char y, signed char z, signed char roll);

		void Initalise(void);
		void UpdateScene(std::optional<std::variant<OpenCAGELevelViewer::ContentManager::UnmanagedModelReference, OpenCAGELevelViewer::ContentManager::UnmanagedComposite>> unmanagedObject);
		void Render(ImVec2 windowSize, std::optional<std::variant<OpenCAGELevelViewer::ContentManager::UnmanagedModelReference, OpenCAGELevelViewer::ContentManager::UnmanagedComposite>> unmanagedObject);
		///void Render(ImVec2 windowSize, std::optional<OpenCAGELevelViewer::ContentManager::UnmanagedModelReference> unmanagedModelReference);
		void Quit(void);
	}
}