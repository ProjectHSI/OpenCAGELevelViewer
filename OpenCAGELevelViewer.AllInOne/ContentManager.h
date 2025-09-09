#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <atomic>
#include <cstdint>
#include <glm/fwd.hpp>
#include <map>
#include <msclr/gcroot.h>
#include <msclr/auto_gcroot.h>
#include <mutex>
#include <string>
#include <utility>
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
				glm::u8vec4 col;
			};
		#pragma pack()

			struct CMModel {
				int modelId {};
				std::vector < CMVertex >    vertices {};
				std::vector < uint16_t >    elements {};
				std::string meshName {};
				std::string lodName {};
			};

			struct CMMaterial {
				bool renderable = true; // if the type we want isn't renderable (no diffuse colour), then we can't render it.
				glm::fvec4 materialCol {};
			};

		#pragma pack(1)
			struct ModelReferenceGL {
				uint32_t instanceId = 0;
				uint32_t isRenderable = 1; // i would filter this out in the ContentManager, but that messes up the positioning, so...
				                           // additionally opengl doesn't allow bools in vertex attributes.
				glm::fmat4 worldMatrix {};
				glm::fvec4 modelCol {};
				glm::fvec4 colOffset {};
			};
		#pragma pack()

			typedef System::Numerics::Vector3 ManagedVector3;
			ref struct ManagedTransform {
				ManagedVector3 position {};
				ManagedVector3 rotation {};
			};

			typedef glm::vec3 Vector3;
			struct Transform {
				Vector3 position {};
				Vector3 rotation {};

				Transform() = default;

				Transform(const ManagedTransform ^managedTransform)
				{
					position = {managedTransform->position.X, managedTransform->position.Y, managedTransform->position.Z};
					rotation = {managedTransform->rotation.X, managedTransform->rotation.Y, managedTransform->rotation.Z};
				}
			};

			typedef ref struct EntityDataValue;
			typedef System::Collections::Generic::Dictionary < CATHODE::Scripting::ShortGuid, EntityDataValue ^ > ShortGuidEntityMap;

			ref struct EntityDataValue {
				ManagedTransform ^transform {};

				CATHODE::Scripting::Internal::Entity ^entity = nullptr;

				EntityDataValue ^parent = nullptr;
			};

			ref struct ModelReferenceDataValue : public EntityDataValue {
				size_t modelReferenceId {};
			};

			ref struct CompositeDataValue : public EntityDataValue {
				ShortGuidEntityMap ^children = gcnew ShortGuidEntityMap(0);
				CATHODE::Scripting::Composite ^composite = nullptr;
			};

			enum CMStatusEnum {
				READY,
				DIRTY,
				LOADING
			};

			struct CMStatus {
				CMStatusEnum currentStatus;
			};

			ref struct LevelContent {
				size_t combinedHash;
				CATHODE::Commands ^Commands;
				CATHODE::Models ^ModelsPAK;
				CATHODE::Materials ^ModelsMTL;
				//CATHODE::Textures ^Textures;
				CATHODE::LEGACY::ShadersPAK ^Shaders;
				CATHODE::RenderableElements ^Renderables;
			};

			extern std::atomic < CMStatus > cmStatus;
			extern std::recursive_mutex cmMutex;

			extern msclr::gcroot < System::Collections::Generic::List < System::Collections::Generic::List < CATHODE::Scripting::ShortGuid > ^ > ^ > compositesById;
			extern std::map < uint64_t, std::pair < CMModel, std::vector < std::pair < uint64_t, size_t > > > > models;
			extern std::map < uint64_t, CMMaterial > materials;
			extern std::map < uint64_t, std::pair < msclr::gcroot < ModelReferenceDataValue ^ >, std::vector < ModelReferenceGL > > > modelReferences;
			extern msclr::gcroot < CompositeDataValue ^ > entityDataRoot;
			extern msclr::gcroot < LevelContent ^ > levelContentInstance;

			/*struct Vec3Transform {
				glm::vec3 position {};
				glm::vec3 rotation {};
			};*/

			void setGameRoot(const std::string &gameRoot);
			std::string getGameRoot();
			// helper function to prevent cloning
			size_t getGameRootHash();

			void setLevel(const std::string &level);
			std::string getLevel();
			// helper function to prevent cloning
			size_t getLevelHash();

			void setComposite(uint32_t compositeGuid);
			uint32_t getComposite();

			std::vector < std::string > getAllLevels();

			void threadMain(std::atomic_flag &suspendFlag);
		}
	}
}