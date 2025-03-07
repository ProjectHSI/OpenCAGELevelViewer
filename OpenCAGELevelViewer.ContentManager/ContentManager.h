#include <atomic>
#include <mutex>
#include <string>
#include <optional>

#ifdef OpenCAGELevelViewer_ContentManager_EXPORTS
#define OpenCAGELevelViewer_ContentManager_API __declspec(dllexport)
#else
//define DllFunction __declspec(dllimport)
#define OpenCAGELevelViewer_ContentManager_API
#endif

OpenCAGELevelViewer_ContentManager_API void initalize_contentManager();

//OpenCAGELevelViewer_ContentManager_API void dummy();

namespace OpenCAGELevelViewer {
	namespace ContentManager {
		/*struct LoadLevelCommand {
			std::string_view level_name;
			std::string_view alien_path;
		};*/

		enum ContentManagerStateEnum {
			LOADING_LEVEL,
			LOADING_COMPOSITE,
			NO_LEVEL,
			READY
		};

		struct ContentManagerState {
			ContentManagerStateEnum contentManagerState;
			float progress;
			std::string progressText;
		};

		struct LoadCompositeCommand {
			std::string level_name;
			std::string alien_path;
			std::string composite_name;
		};

		struct UnmanagedComposite {

		};

		struct ContentManagerContext {
			ContentManagerState contentManagerState {NO_LEVEL, 0, ""};
			std::mutex contentManagerStateMutex {};

			//std::recursive_mutex 

			std::optional<LoadCompositeCommand> command {};
			std::mutex commandMutex {};

			bool isCompositeSet = false;

		};

		OpenCAGELevelViewer_ContentManager_API ContentManagerContext *getContentManagerContext(void);

		OpenCAGELevelViewer_ContentManager_API void contentManagerThread(std::atomic_flag &suspendFlag);

	}
}