#pragma once
#include <atomic>
#include <mutex>
#include <string>
#include <optional>
#include <vector>
#include <variant>

#ifdef OpenCAGELevelViewer_ContentManager_EXPORTS
#define OpenCAGELevelViewer_ContentManager_API __declspec(dllexport)
#else
//define DllFunction __declspec(dllimport)
#define OpenCAGELevelViewer_ContentManager_API
#endif

OpenCAGELevelViewer_ContentManager_API void initalize_contentManager();

OpenCAGELevelViewer_ContentManager_API void dummy();

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

		template<typename T = double>
		struct Vector3 {
			T x = 0;
			T y = 0;
			T z = 0;

			Vector3() { }

			Vector3(T _x, T _y, T _z) {
				x = _x;
				y = _y;
				z = _z;
			}

			/*Vector3<>(T _x, T _y, T _z) {
				x = _x;
				y = _y;
				z = _z;
			}*/
		};

		template<typename T = double>
		struct Vector4 {
			T x = 0;
			T y = 0;
			T z = 0;
			T w = 0;

			Vector4() { }

			Vector4(T _x, T _y, T _z, T _w) {
				x = _x;
				y = _y;
				z = _z;
				w = _w;
			}

			/*Vector4(float _x, float _y, float _z, float _w) {
				x = _x;
				y = _y;
				z = _z;
				w = _w;
			}*/
		};

		struct Transform {
			Vector3<> position {};
			Vector3<> rotation {};
			std::optional<Transform *> parent {};

			Transform() { }

			//Transform(Transform &_transform) {
			//	position = _transform.position;
			//	rotation = _transform.rotation;
			//	parent = _transform.parent;
			//	/*if (_transform.parent.has_value()) {
			//		Transform &newParent = _transform.parent.value();

			//		parent = newParent;
			//	}*/
			//}

			Transform(Vector3<> _position, Vector3<> _rotation) {
				position = _position;
				rotation = _rotation;
			}

			Transform(Vector3<> _position, Vector3<> _rotation, Transform *_parent) {
				position = _position;
				rotation = _rotation;
				parent = _parent;
			}
		};

		struct UnmanagedModelReference {
			struct ModelStorage {
				std::vector<Vector3<float>> vertices;
				std::vector<uint16_t> indices;

				unsigned long long id; // so the host app doesn't have to generate 50 vertex arrays for the same object
			};

			std::string name;
			Transform transform;
			std::optional<ModelStorage> model;
		};

		struct UnmanagedComposite {
			std::string name;
			std::string instanceType;
			std::string shortGuid;
			Transform transform;
			std::vector<UnmanagedComposite> unmanagedCompositeChildren;
			std::vector<std::variant<UnmanagedModelReference>> unmanagedEntityChildren;
			/*std::vector<
				std::variant<>
			> unmanagedEntityChildren;*/
		};

		struct ContentManagerContext {
			ContentManagerState contentManagerState {NO_LEVEL, 0, ""};
			std::mutex contentManagerStateMutex {};

			std::atomic_flag newCompositeLoaded;

			//std::recursive_mutex 

			std::optional<LoadCompositeCommand> command {};
			std::mutex commandMutex {};

			//bool isCompositeSet = false;
			std::recursive_mutex unmanagedCompositeMutex;
			std::optional<UnmanagedComposite> unmanagedComposite;
		};

		OpenCAGELevelViewer_ContentManager_API ContentManagerContext *getContentManagerContext(void);

		OpenCAGELevelViewer_ContentManager_API void contentManagerThread(std::atomic_flag &suspendFlag);

	}
}