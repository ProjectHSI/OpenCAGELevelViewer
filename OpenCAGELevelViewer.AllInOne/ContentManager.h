#pragma once

#include <string>
#include <mutex>

namespace OpenCAGELevelViewer {
	namespace AllInOne {
		namespace ContentManager {
			/*struct ContentManagerState {
				struct {
					std::string loadingText {};
					bool isLoading {};
					double loadingProgress {};
				} loadingState;

				struct {
					
				} level;
			};*/

			//extern std::recursive_mutex contentManagerMutex;
			//extern ContentManagerState contentManagerState;

			void threadMain();
		}
	}
}