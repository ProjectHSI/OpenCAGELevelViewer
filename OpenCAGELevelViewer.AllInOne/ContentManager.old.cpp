#if 0
#include "ContentManager.h"
#include <atomic>
#include <cassert>
#include <iostream>

//#include <codecvt>

#include <array>
#include <bit>
#include <cstdint>
#include <limits>
#include <map>
#include <msclr/gcroot.h>
#include <msclr/marshal.h>
#include <optional>
#include <utility>
//#include "CS2MeshParser.cs"

// *sigh*
#undef max
#undef min

using namespace System::Collections::Generic;

using namespace CATHODE;
using namespace CATHODE::Scripting;
using namespace CATHODE::LEGACY;
using namespace System;
using namespace OpenCAGELevelViewer;
//using namespace static CATHODE::Models;

static msclr::gcroot< msclr::interop::marshal_context ^ > msclr_context = gcnew msclr::interop::marshal_context();

static OpenCAGELevelViewer::ContentManager::ContentManagerContext contentManagerContext;

static std::string currentLevelName = "";
static std::string currentCompositeGuid = "";

ref struct LevelContent {
	Movers ^ModelsMVR;
	Commands ^CommandsPAK;
	RenderableElements ^RenderableREDS;
	CATHODE::Resources ^ResourcesBIN;
	PhysicsMaps ^PhysicsMap;
	EnvironmentMaps ^EnvironmentMap;
	CollisionMaps ^CollisionMap;
	/*EnvironmentAnimations EnvironmentAnimation;
	byte[] ModelsCST;*/
	Materials ^ModelsMTL;
	Models ^ModelsPAK;
	Textures ^LevelTextures;
	Shaders ^Shaders;
	//alien_shader_bin_pak ^ShadersBIN;
	//IDXRemap ^ShadersIDXRemap;
};

msclr::gcroot<LevelContent ^> levelContentInstance = gcnew LevelContent();

// this runs some basic .NET code to get CoreCLR together
OpenCAGELevelViewer_ContentManager_API void initalize_contentManager() {
	/*double _9sqrt = */System::Math::Sqrt(9);
}

OpenCAGELevelViewer_ContentManager_API OpenCAGELevelViewer::ContentManager::ContentManagerContext *OpenCAGELevelViewer::ContentManager::getContentManagerContext(void) {
	return &contentManagerContext;
}

static void updateProgress(OpenCAGELevelViewer::ContentManager::ContentManagerState state) {
	contentManagerContext.contentManagerState = state;
}

// because C#
//std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> utf8Converter;

//struct Vector3<> {
//	double x;m
//	double y;
//	double z;
//};

#define MarshalCliString(cliString) msclr_context->marshal_as<const char *>(cliString)
#define ConvertCliStringToCXXString(cliString) std::string(MarshalCliString(cliString))

static inline void updateCompositeParse(CATHODE::Scripting::Composite ^composite) {
	updateProgress({OpenCAGELevelViewer::ContentManager::ContentManagerStateEnum::LOADING_COMPOSITE, -1, std::string("Parsing composite ") + MarshalCliString(composite->shortGUID.ToString()) + "..."});
}

//System::Numerics::Vector3<> Vector3<>zero = System::Numerics::Vector3<>(0, 0, 0);

static OpenCAGELevelViewer::ContentManager::Vector3<double> convertCSVector3ToUnmanagedVector3(System::Numerics::Vector3 numericsVector3) {
	return OpenCAGELevelViewer::ContentManager::Vector3<double>(static_cast< double >(numericsVector3.X), static_cast< double >(numericsVector3.Y), static_cast< double >(numericsVector3.Z));
}

static bool GetEntityTransform(CATHODE::Scripting::Internal::Entity ^entity, OpenCAGELevelViewer::ContentManager::Transform &transformToChange) {
	//position = Vector3<>zero;
	//rotation = Vector3<>zero;
	if (entity == nullptr) return false;
	Parameter ^positionParam = entity->GetParameter("position");
	if (positionParam != nullptr && positionParam->content != nullptr) {
	#pragma warning(disable: 4062)
		switch (positionParam->content->dataType) {
			case DataType::TRANSFORM:
				cTransform ^transform = dynamic_cast<cTransform ^>(positionParam->content);
				transformToChange.position = convertCSVector3ToUnmanagedVector3(transform->position);
				transformToChange.rotation = convertCSVector3ToUnmanagedVector3(transform->rotation);
				return true;
		}
	#pragma warning(default: 4062)
	}
	return false;
}

ref class TrimmedAliasAndFunctionEntityComparer {
	CATHODE::Scripting::ShortGuid shortGuid;
	bool checkForOneLength = false;

public:
	TrimmedAliasAndFunctionEntityComparer(CATHODE::Scripting::ShortGuid _shortGuid, bool _checkForOneLength) {
		shortGuid = _shortGuid;
		checkForOneLength = _checkForOneLength;
	}

	TrimmedAliasAndFunctionEntityComparer(CATHODE::Scripting::ShortGuid _shortGuid) {
		shortGuid = _shortGuid;
	}

	bool compare(AliasEntity ^obj) {
		return (!checkForOneLength || (gcnew List<ShortGuid>(obj->alias->path))->Count == 1) && obj->alias->path[0] == shortGuid;
	}

	//bool compare(AliasEntity ^obj) { return compare(obj, false); }
};

ref class TrimmedAliasAndFunctionEntitySomeThingElseComparer {
	CATHODE::Scripting::ShortGuid shortGuid;

public:
	TrimmedAliasAndFunctionEntitySomeThingElseComparer(CATHODE::Scripting::ShortGuid _shortGuid) {
		shortGuid = _shortGuid;
	}

	bool compare(AliasEntity ^obj) {
		auto aliasPathCount = (gcnew List<ShortGuid>(obj->alias->path))->Count;

		return (aliasPathCount == (obj->alias->path[aliasPathCount - 1] == ShortGuid::Invalid ? 2 : 1) && obj->alias->path[0] == shortGuid);
	}
};

static std::map<int, OpenCAGELevelViewer::ContentManager::UnmanagedModelReference::ModelStorage> modelCache {};

template<typename T>
T getInstanceFromUnsignedCharVector(cli::array<unsigned char> ^submeshContent, size_t &submeshContentPointerCurrentIndex) {
	std::array<unsigned char, sizeof(T)> TbitContents {};
	//std::copy_n(submesh->content.begin() + submeshContentPointerCurrentIndex, sizeof(T), TbitContents.begin());

	for (size_t i = 0; i < sizeof(T); i++) {
		TbitContents[i] = submeshContent[submeshContentPointerCurrentIndex + i];
	}

	T tInstance = std::bit_cast< T >(TbitContents);

	submeshContentPointerCurrentIndex += sizeof(T);

	return tInstance;
}

static OpenCAGELevelViewer::ContentManager::UnmanagedModelReference::ModelStorage parseRenderableElement(RenderableElements::Element ^renderableElement) {
	OpenCAGELevelViewer::ContentManager::UnmanagedModelReference::ModelStorage modelStorage;

	Models::CS2::Component::LOD::Submesh ^submesh = levelContentInstance->ModelsPAK->GetAtWriteIndex(renderableElement->ModelIndex);
	//if (submesh == nullptr) return modelStorage;
	Models::CS2::Component::LOD ^lod = levelContentInstance->ModelsPAK->FindModelLODForSubmesh(submesh);
	Models::CS2 ^mesh = levelContentInstance->ModelsPAK->FindModelForSubmesh(submesh);

	OpenCAGELevelViewer::ContentManager::CSManaged::ParsedCS2Mesh ^managedParsedCs2Mesh = OpenCAGELevelViewer::ContentManager::CSManaged::CathodeLibExtensions::SubmeshToParsedMesh(submesh);

	for each (auto vertex in managedParsedCs2Mesh->vertices) {
		modelStorage.vertices.push_back(OpenCAGELevelViewer::ContentManager::Vector3<float>(vertex.x, vertex.y, vertex.z));
	}

	for each(auto index in managedParsedCs2Mesh->indices) {
		modelStorage.indices.push_back(index);
	}

	modelStorage.id = renderableElement->ModelIndex;

	return modelStorage;
}

static void parseModelReferenceWithRenderable(OpenCAGELevelViewer::ContentManager::UnmanagedModelReference &unmanagedModelReference, RenderableElements::Element ^renderableElement) {
	//OpenCAGELevelViewer::ContentManager::UnmanagedModelReference::ModelStorage modelStorage

	//switch (resourceType) {
	//	case ResourceType::RENDERABLE_INSTANCE:
	{
		int modelIndex = renderableElement->ModelIndex;

		//OpenCAGELevelViewer::ContentManager::UnmanagedModelReference modelStorage;

		if (!modelCache.contains(modelIndex)) {
			modelCache[modelIndex] = parseRenderableElement(renderableElement);
		}

		unmanagedModelReference.model = modelCache[modelIndex];
				//modelStorage
	}

			//modelCache[renderableElement->ModelIndex] 
	//}
}

// Matt Filer does some weird stuff everytime.
// This abstracts that so it's easier to port between versions of CATHODELib.
static bool isFunctionEntityCompositeInstance(const LevelContent ^levelContent, const CATHODE::Scripting::FunctionEntity ^functionEntity) {
	return (levelContent->CommandsPAK->GetComposite(functionEntity->function) != nullptr);
}

static OpenCAGELevelViewer::ContentManager::UnmanagedComposite parseComposite(CATHODE::Scripting::Composite ^composite, OpenCAGELevelViewer::ContentManager::Transform parentTransform, List<AliasEntity ^> ^aliases/*, std::optional<OpenCAGELevelViewer::ContentManager::UnmanagedComposite &> unmanagedComposite = {}*/) {
	updateCompositeParse(composite);

	OpenCAGELevelViewer::ContentManager::UnmanagedComposite newUnmanagedComposite {};
	newUnmanagedComposite.instanceType = std::string(msclr_context->marshal_as<const char *>(composite->name->ToString()));
	newUnmanagedComposite.shortGuid = std::string(msclr_context->marshal_as<const char *>(composite->shortGUID.ToString()));
	newUnmanagedComposite.transform = parentTransform;

	List<AliasEntity ^> ^trimmedAliases = gcnew List<AliasEntity ^>();
	for (int i = 0; i < aliases->Count; i++) {
		(gcnew List<ShortGuid> (aliases->ToArray()[i]->alias->path))->RemoveAt(0);
		if ((gcnew List<ShortGuid>(aliases->ToArray()[i]->alias->path))->Count != 0)
			trimmedAliases->Add(aliases->ToArray()[i]);
	}
	trimmedAliases->AddRange(composite->aliases);
	aliases = trimmedAliases;

	for each (CATHODE::Scripting::FunctionEntity ^functionEntity in composite->functions->ToArray()) {
		if (isFunctionEntityCompositeInstance(levelContentInstance, functionEntity)) {
			Composite ^compositeNext = levelContentInstance->CommandsPAK->GetComposite(functionEntity->function);
			if (compositeNext != nullptr) {
				//Find all overrides that are appropriate to take through to the next composite
				TrimmedAliasAndFunctionEntityComparer ^trimmedAliasAndFunctionEntityComparer = gcnew TrimmedAliasAndFunctionEntityComparer(functionEntity->shortGUID);

				Predicate<AliasEntity ^> ^aliasCarryOver = gcnew Predicate<AliasEntity ^>(trimmedAliasAndFunctionEntityComparer, &TrimmedAliasAndFunctionEntityComparer::compare);

				List<AliasEntity ^> ^overridesNext = trimmedAliases->FindAll(aliasCarryOver);

				OpenCAGELevelViewer::ContentManager::Transform newTransform;
				newTransform.parent = &newUnmanagedComposite.transform;
				AliasEntity ^ovrride = trimmedAliases->Find(gcnew Predicate<AliasEntity ^>(gcnew TrimmedAliasAndFunctionEntitySomeThingElseComparer(functionEntity->shortGUID), &TrimmedAliasAndFunctionEntitySomeThingElseComparer::compare));
				if (!GetEntityTransform(ovrride, newTransform))
					GetEntityTransform(functionEntity, newTransform);

				newUnmanagedComposite.unmanagedCompositeChildren.push_back(parseComposite(compositeNext, newTransform, overridesNext/*, newUnmanagedComposite*/));
				newUnmanagedComposite.unmanagedCompositeChildren[newUnmanagedComposite.unmanagedCompositeChildren.size() - 1].name = std::string(msclr_context->marshal_as<const char *>(levelContentInstance->CommandsPAK->Utils->GetEntityName(composite->shortGUID, functionEntity->shortGUID)));
				updateCompositeParse(composite);

				//Continue
				//ParseComposite(compositeNext, compositeGO, position, Quaternion.Euler(rotation), overridesNext);

				//Work out our position, accounting for overrides
				/*Vector3<> position, rotation;
				AliasEntity ovrride = trimmedAliases.FirstOrDefault(o = > o.alias.path.Count == (o.alias.path[o.alias.path.Count - 1] == ShortGuid.Invalid ? 2 : 1) && o.alias.path[0] == function.shortGUID);
				if (!GetEntityTransform(ovrride, out position, out rotation))
					GetEntityTransform(function, out position, out rotation);*/

				//Continue
				//ParseComposite(compositeNext, compositeGO, position, Quaternion.Euler(rotation), overridesNext);
			}
		} else {
		#pragma warning(disable: 4061)
			switch (functionEntity->function.AsFunctionType) {
				case FunctionType::ModelReference:
					{
						OpenCAGELevelViewer::ContentManager::UnmanagedModelReference unmanagedModelReference;
						unmanagedModelReference.name = ConvertCliStringToCXXString(levelContentInstance->CommandsPAK->Utils->GetEntityName(composite->shortGUID, functionEntity->shortGUID));

						OpenCAGELevelViewer::ContentManager::Transform newTransform;
						newTransform.parent = &newUnmanagedComposite.transform;
						AliasEntity ^ovrride = trimmedAliases->Find(gcnew Predicate<AliasEntity ^>(gcnew TrimmedAliasAndFunctionEntityComparer(functionEntity->shortGUID, true), &TrimmedAliasAndFunctionEntityComparer::compare));
						if (!GetEntityTransform(ovrride, newTransform))
							GetEntityTransform(functionEntity, newTransform);

						unmanagedModelReference.transform = newTransform;

						Parameter ^resourceParam = functionEntity->GetParameter("resource");
						if (resourceParam != nullptr && resourceParam->content != nullptr) {
							switch (resourceParam->content->dataType) {
								case DataType::RESOURCE:
									cResource ^resource = dynamic_cast< cResource ^ >(resourceParam->content);
									for each (ResourceReference ^resourceRef in resource->shortGUID.) {
										for (int i = 0; i < resourceRef->count; i++) {
											RenderableElements::Element ^renderable = levelContentInstance->RenderableREDS->Entries[resourceRef->index + i];
											switch (resourceRef->resource_type) {
												case ResourceType::RENDERABLE_INSTANCE:
													//SpawnModel(renderable.ModelIndex, renderable.MaterialIndex, nodeModel);
													//renderable->
													parseModelReferenceWithRenderable(unmanagedModelReference, renderable);
													break;
												case ResourceType::COLLISION_MAPPING:
													break;
											}
										}
									}
									break;
							}
						}

						newUnmanagedComposite.unmanagedEntityChildren.push_back(unmanagedModelReference);
					}
					break;

				default:
					break;
			}
		#pragma warning(default: 4061)
		}
	}

	/*if (unmanagedComposite.has_value())
		unmanagedComposite->unmanagedCompositeChildren.push_back(newUnmanagedComposite);*/

	return newUnmanagedComposite;

	//for (FunctionEntity function : ) {
	//	std::cout << msclr_context->marshal_as<const char*>(function.ToString()) << std::endl;
	//}
}

OpenCAGELevelViewer_ContentManager_API void OpenCAGELevelViewer::ContentManager::contentManagerThread(std::atomic_flag &suspendFlag) {
	while (suspendFlag.test()) {
		std::optional< OpenCAGELevelViewer::ContentManager::LoadCompositeCommand > copiedCommand {};

		{
			std::lock_guard commandsLockGuard(contentManagerContext.commandMutex);

			if (contentManagerContext.command.has_value()) {
				copiedCommand = contentManagerContext.command;

				contentManagerContext.command.reset();
			}
		}

		if (copiedCommand.has_value()) {
			if (copiedCommand->level_name != currentLevelName) {
				currentLevelName = copiedCommand->level_name;

				levelContentInstance = gcnew LevelContent();
				modelCache.clear();

				std::string levelPath = copiedCommand->alien_path + "/DATA/ENV/PRODUCTION/" + copiedCommand->level_name + "/";
				std::string worldPath = levelPath + "WORLD/";
				std::string renderablePath = levelPath + "RENDERABLE/";

				//renderablePath.

				//updateProgress({LOADING_LEVEL, static_cast< float >(1) / 4, std::string("Loading ") + "t" + "."});

			#define UPDATE_PROGRESS_FOR_LOAD(FilePath, Index, Max) updateProgress({LOADING_LEVEL, static_cast<float>(##Index) / ##Max, std::string("Loading \"") + FilePath + "\"."})
			#define GET_FULL_LEVEL_SPECIFIC_PATH(FilePath) (copiedCommand->alien_path + "/DATA/ENV/PRODUCTION/" + copiedCommand->level_name + "/" + FilePath)

			#define LOAD_WITHOUT_WAIT(VarName, CSType, FilePath, Index, Max) \
UPDATE_PROGRESS_FOR_LOAD(FilePath, Index, Max); \
levelContentInstance->VarName = gcnew CSType(gcnew String(/*utf8Converter.from_bytes(*/GET_FULL_LEVEL_SPECIFIC_PATH(FilePath)/*)*/.c_str()));

			#define LOAD(VarName, CSType, FilePath, Index, Max) LOAD_WITHOUT_WAIT(VarName, CSType, FilePath, Index, Max); while (levelContentInstance->VarName->Loaded == false) std::this_thread::yield()//; printf("%i", VarName->Loaded);

				//updateProgress({LOADING_LEVEL, 0, "Loading " + });
				//auto ModelsMVR = gcnew CATHODE::Movers(worldPath + "MODELS.MVR");
				//auto CommandsPAK = gcnew CATHODE::Commands(worldPath + "COMMANDS.PAK");
				LOAD(ModelsMVR, Movers, "WORLD/MODELS.MVR", 0, 12);
				LOAD(CommandsPAK, Commands, "WORLD/COMMANDS.PAK", 1, 12);
				//printf("%i\n", levelContentInstance->CommandsPAK->Loaded);
				LOAD(RenderableREDS, RenderableElements, "WORLD/REDS.BIN", 2, 12);
				LOAD(ResourcesBIN, CATHODE::Resources, "WORLD/RESOURCES.BIN", 3, 12);
				LOAD(PhysicsMap, PhysicsMaps, "WORLD/PHYSICS.MAP", 4, 12);
				LOAD(EnvironmentMap, EnvironmentMaps, "WORLD/ENVIRONMENTMAP.BIN", 5, 12);
				LOAD(CollisionMap, CollisionMaps, "WORLD/COLLISION.MAP", 6, 12);
				//// "class does not have a copy constructor"
				//LOAD(EnvironmentAnimation, EnvironmentAnimations, worldPath + "ENVIRONMENT_ANIMATION.DAT", 7, 12);
				///*UPDATE_PROGRESS_FOR_LOAD("WORLD/ENVIRONMENT_ANIMATION.DAT", 6, 13);
				//auto EnvironmentAnimation = gcnew CATHODE::EnvironmentAnimations(
				//	gcnew String(utf8Converter.from_bytes(GET_FULL_LEVEL_SPECIFIC_PATH("WORLD/ENVIRONMENT_ANIMATION.DAT")).c_str()),
				//	gcnew CATHODE::AnimationStrings(gcnew String(utf8Converter.from_bytes((copiedCommand->alien_path + "/DATA/GLOBAL/ANIMATION.PAK")).c_str())));*/
				//
				////(gcnew String(((copiedCommand->alien_path + "/DATA/ENV/PRODUCTION/" + copiedCommand->level_name + "/" + worldPath + "ENVIRONMENT_ANIMATION.DAT").c_str())));
				//// TODO: ModelsCST?
				// 7
				LOAD(ModelsMTL, Materials, "RENDERABLE/LEVEL_MODELS.MTL", 8, 12);
				LOAD(ModelsPAK, Models, "RENDERABLE/LEVEL_MODELS.PAK", 9, 12);
				LOAD(Shaders, Shaders, "RENDERABLE/LEVEL_SHADERS_DX11.PAK", 10, 12); // we need might to recompile these for OpenGL if directx isn't simple enough for me to understand
				//LOAD_WITHOUT_WAIT(ShadersIDXRemap, IDXRemap, "RENDERABLE/LEVEL_SHADERS_DX11_IDX_REMAP.PAK", 11, 13);
				LOAD(LevelTextures, Textures, "RENDERABLE/LEVEL_TEXTURES.ALL.PAK", 11, 12);

				updateProgress({READY, 0, ""});

				//CATHODE::Shaders
				
				//LOAD()
				//updateProgress({LOADING_LEVEL, 0, "Loading level " + copiedCommand->level_name});
				//std::this_thread::sleep_for(std::chrono::seconds(5));
				//updateProgress({LOADING_LEVEL, 0.5, "Loading level " + copiedCommand->level_name});
				//std::this_thread::sleep_for(std::chrono::seconds(5));
				//updateProgress({LOADING_LEVEL, 1, "heheha"});
				//std::this_thread::sleep_for(std::chrono::seconds(1));
				//updateProgress({LOADING_LEVEL, -1, "heheha2"});
				//std::this_thread::sleep_for(std::chrono::seconds(5));
				//updateProgress({READY, 1, ""});
			}

			if (copiedCommand->composite_name != currentCompositeGuid) {
				currentCompositeGuid = copiedCommand->composite_name;

				//updateProgress({LOADING_COMPOSITE, -1, "Parsing..."});

				String ^shortGuidString = gcnew String(copiedCommand->composite_name.c_str());
				ShortGuid ^shortGuid = gcnew ShortGuid(shortGuidString);

				//gcnew String()

				//Commands ^commandsPakTest = levelContentInstance->CommandsPAK;
				{
					//Transform transform = Transform();
					ContentManager::UnmanagedComposite intermediary = parseComposite(levelContentInstance->CommandsPAK->GetComposite(*shortGuid), {}, gcnew List<AliasEntity ^>());

					std::lock_guard unmanagedCompositeLockGuard(contentManagerContext.unmanagedCompositeMutex);

					contentManagerContext.unmanagedComposite = intermediary;
				}

				//std::this_thread::sleep_for(std::chrono::seconds(2));

				updateProgress({READY, 0, ""});

				contentManagerContext.newCompositeLoaded.test_and_set();
			}
		}
	}
}

OpenCAGELevelViewer_ContentManager_API void dummy() { }
#endif