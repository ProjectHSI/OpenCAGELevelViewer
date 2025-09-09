#pragma once

#include <imgui.h>
//#include "ContentManager.h"
#include <mutex>
#include <optional>
#include <map>
#include <array>

namespace OpenCAGELevelViewer {
	namespace _3DView {
		unsigned int getFbo(void);
		unsigned int getSelectFbo(void);

		extern float fov;
		extern float mouseSensitivity;
		extern bool  axisArrows;
		extern bool  ignoreColW;

		
		enum VertexColourMode {
			MAT_BASED = 0,
			MODEL_BASED = 1,
			INSTANCE_ID_BASED = 2,
			POSITION_BASED = 3,
		};
		constexpr std::array < std::pair < VertexColourMode, const char * >, 4 > vertexColourModeNames = {
			std::pair < VertexColourMode, const char * > { VertexColourMode::MAT_BASED,   "Material Based" },
			std::pair < VertexColourMode, const char * > { VertexColourMode::MODEL_BASED, "Model Based" },
			std::pair < VertexColourMode, const char * > { VertexColourMode::INSTANCE_ID_BASED, "Instance ID Based" },
			std::pair < VertexColourMode, const char * > { VertexColourMode::POSITION_BASED, "Position Based" }
		};
		extern VertexColourMode vertexColourMode;

		//extern std::recursive_mutex _3dViewMutex;

		/*struct _3DViewCamera {

		};*/

		//void updateCamera(int32_t x, int32_t y);
		void updateCamera(const float x, const float y, const float z, const float roll, const float dYaw, const float dPitch, const float dFov, const float speed, const bool smooth, const float deltaTime);

		void markForSelect(ImVec2 coordinates);

		void setInstanceIdSelected(const int64_t selected);
		const int64_t getUserSelectedInstanceId(const ImVec2 windowSize, const ImVec2 selectedCoordinates = ImVec2(0.5f, 0.5f));
		
		//typedef std::variant < OpenCAGELevelViewer::ContentManager::UnmanagedComposite, OpenCAGELevelViewer::ContentManager::UnmanagedModelReference > UsuableUnmanagedObjects;

		void Initalise(void);
		void UpdateScene(/*std::optional < OpenCAGELevelViewer::_3DView::UsuableUnmanagedObjects > unmanagedObject, */std::atomic_flag &isDone);
		void Render(ImVec2 windowSize/*, std::optional<std::variant<OpenCAGELevelViewer::ContentManager::UnmanagedModelReference, OpenCAGELevelViewer::ContentManager::UnmanagedComposite>> unmanagedObject*/);
		///void Render(ImVec2 windowSize, std::optional<OpenCAGELevelViewer::ContentManager::UnmanagedModelReference> unmanagedModelReference);
		void Quit(void);
	}
}