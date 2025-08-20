#pragma once

#ifdef OpenCAGELevelViewer_AllInOne_EXPORTS
#define OpenCAGELevelViewer_AllInOne_API __declspec(dllexport)
#else
#define OpenCAGELevelViewer_AllInOne_API __declspec(dllimport)
#endif

#ifdef OpenCAGELevelViewer_AllInOne_EXPORTS
#include <array>
#include <string_view>
#endif

OpenCAGELevelViewer_AllInOne_API int handoff(char **argv, int argc);

#ifdef OpenCAGELevelViewer_AllInOne_EXPORTS
namespace OpenCAGELevelViewer {
	namespace AllInOne {
		namespace Handoff {
			enum SceneRenderingStrategy {
				INDIRECT_BATCHING = 0,
				PER_MODEL_BATCHING = 1,
				NO_BATCHING = 2
			};

			constexpr std::array < std::pair < SceneRenderingStrategy, std::string_view >, 3 > sceneRenderingStrategy {
				std::pair < SceneRenderingStrategy, std::string_view > { INDIRECT_BATCHING, "Indirect Batching" },
				std::pair < SceneRenderingStrategy, std::string_view > { PER_MODEL_BATCHING, "Per-Model Batching" },
				std::pair < SceneRenderingStrategy, std::string_view > { NO_BATCHING, "No Batching" }
			};

			extern SceneRenderingStrategy currentSceneRenderingStrategy;


			extern std::string gameRoot;
		}
	}
}
#endif

//__declspec(dllexport) void dummy();