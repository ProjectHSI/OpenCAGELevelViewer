#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <glm/fwd.hpp>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

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

		#pragma pack(1)
			struct CMVertex {
				glm::fvec3 pos;
				glm::uvec4 col;
			};
		#pragma pack()

			struct CMModel {
				int modelId;
				std::vector < CMVertex >    vertices {};
				std::vector < uint16_t >    elements {};
				std::string meshName;
				std::string lodName;
			};

		#pragma pack(1)
			struct ModelReferenceGL {
				uint32_t instanceId = 0;
				glm::fmat4 worldMatrix {};
				float shine = 0;
			};
		#pragma pack()

			/*struct Vec3Transform {
				glm::vec3 position {};
				glm::vec3 rotation {};
			};*/

			void setGameRoot(const std::string &gameRoot);
			std::string getGameRoot();

			void setLevel(const std::string &level);
			std::string getLevel();

			void setComposite(uint32_t compositeGuid);
			uint32_t getComposite();

			void threadMain(std::atomic_flag &suspendFlag);
		}
	}
}