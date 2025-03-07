#include <string_view>

namespace OpenCAGELevelViewer {
	namespace ContentManager {
		namespace Level {
			class __declspec(dllexport) Level {
				Level(std::string_view alienPath, std::string_view levelName);

				void getComposite(std::string_view shortGuid);
			};
		}
	}
}