#include "ContentManager.h"
#include <atomic>
#include <cassert>
#include <iostream>

#include <codecvt>

#include <msclr/gcroot.h>
#include <msclr/marshal_cppstd.h>

using namespace CATHODE;
using namespace CATHODE::Scripting;
using namespace CATHODE::LEGACY;
using namespace System;

msclr::gcroot< msclr::interop::marshal_context ^ > msclr_context = gcnew msclr::interop::marshal_context();

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
	ShadersPAK ^ShadersPAK;
	alien_shader_bin_pak ^ShadersBIN;
	IDXRemap ^ShadersIDXRemap;
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
std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> utf8Converter;

struct Vector3 {
	double x;
	double y;
	double z;
};

constexpr Vector3 zeroVector3 = Vector3(0, 0, 0);

static void parseComposite(CATHODE::Scripting::Composite ^composite) {
	/*List<AliasEntity> trimmedAliases = new List<AliasEntity>();
	for (int i = 0; i < aliases.Count; i++) {
		aliases[i].alias.path.RemoveAt(0);
		if (aliases[i].alias.path.Count != 0)
			trimmedAliases.Add(aliases[i]);
	}
	trimmedAliases.AddRange(composite.aliases);
	aliases = trimmedAliases;*/
	//composite.GetEntities

	System::Collections::Generic::List<CATHODE::Scripting::Internal::Entity ^> ^entities = composite->GetEntities();
	array< CATHODE::Scripting::Internal::Entity ^ > ^entitiesArray = entities->ToArray();

	for each (auto entity in entitiesArray) {
		std::cout << msclr_context->marshal_as<const char *>(entity->shortGUID.ToString()) << std::endl;
	}

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
				levelContentInstance = gcnew LevelContent();

				std::string levelPath = copiedCommand->alien_path + "/DATA/ENV/PRODUCTION/" + copiedCommand->level_name + "/";
				std::string worldPath = levelPath + "WORLD/";
				std::string renderablePath = levelPath + "RENDERABLE/";

				//renderablePath.

				//updateProgress({LOADING_LEVEL, static_cast< float >(1) / 4, std::string("Loading ") + "t" + "."});

			#define UPDATE_PROGRESS_FOR_LOAD(FilePath, Index, Max) updateProgress({LOADING_LEVEL, static_cast<float>(##Index) / ##Max, std::string("Loading \"") + FilePath + "\"."})
			#define GET_FULL_LEVEL_SPECIFIC_PATH(FilePath) (copiedCommand->alien_path + "/DATA/ENV/PRODUCTION/" + copiedCommand->level_name + "/" + FilePath)
			#define LOAD(VarName, CSType, FilePath, Index, Max) \
/*UPDATE_PROGRESS_FOR_LOAD(FilePath, Index, Max);*/ \
levelContentInstance->VarName = gcnew CSType(gcnew String(/*utf8Converter.from_bytes(*/GET_FULL_LEVEL_SPECIFIC_PATH(FilePath)/*)*/.c_str()))//; \
//while (levelContentInstance->VarName->Loaded == false) std::this_thread::yield()//; printf("%i", VarName->Loaded);

				//updateProgress({LOADING_LEVEL, 0, "Loading " + });
				//auto ModelsMVR = gcnew CATHODE::Movers(worldPath + "MODELS.MVR");
				//auto CommandsPAK = gcnew CATHODE::Commands(worldPath + "COMMANDS.PAK");
				/*LOAD(ModelsMVR, Movers, "WORLD/MODELS.MVR", 0, 14);*/
				LOAD(CommandsPAK, Commands, "WORLD/COMMANDS.PAK", 1, 14);
				//printf("%i\n", levelContentInstance->CommandsPAK->Loaded);
				//LOAD(RenderableREDS, RenderableElements, "WORLD/REDS.BIN", 2, 14);
				//LOAD(ResourcesBIN, CATHODE::Resources, "WORLD/RESOURCES.BIN", 3, 14);
				//LOAD(PhysicsMap, PhysicsMaps, "WORLD/PHYSICS.MAP", 4, 14);
				//LOAD(EnvironmentMap, EnvironmentMaps, "WORLD/ENVIRONMENT.BIN", 5, 14);
				//LOAD(CollisionMap, CollisionMaps, "WORLD/COLLISION.MAP", 6, 14);
				//// "class does not have a copy constructor"
				////LOAD(EnvironmentAnimation, EnvironmentAnimations, worldPath + "ENVIRONMENT_ANIMATION.DAT", 7, 14);
				///*UPDATE_PROGRESS_FOR_LOAD("WORLD/ENVIRONMENT_ANIMATION.DAT", 7, 14);
				//auto EnvironmentAnimation = gcnew CATHODE::EnvironmentAnimations(
				//	gcnew String(utf8Converter.from_bytes(GET_FULL_LEVEL_SPECIFIC_PATH("WORLD/ENVIRONMENT_ANIMATION.DAT")).c_str()),
				//	gcnew CATHODE::AnimationStrings(gcnew String(utf8Converter.from_bytes((copiedCommand->alien_path + "/DATA/GLOBAL/ANIMATION.PAK")).c_str())));*/
				//
				////(gcnew String(((copiedCommand->alien_path + "/DATA/ENV/PRODUCTION/" + copiedCommand->level_name + "/" + worldPath + "ENVIRONMENT_ANIMATION.DAT").c_str())));
				//// TODO: ModelsCST?
				//LOAD(ModelsMTL, Materials, "RENDERABLE/LEVEL_MODELS.MTL", 9, 14);
				//LOAD(ModelsPAK, Models, "RENDERABLE/LEVELS_MODELS.PAK", 10, 14);
				//LOAD(ShadersPAK, CATHODE::LEGACY::ShadersPAK, "RENDERABLE/LEVEL_SHADERS_DX11.PAK", 11, 14); // we need might to recompile these for OpenGL if directx isn't simple enough for me to understand
				//LOAD(ShadersIDXRemap, IDXRemap, "RENDERABLE/LEVEL_SHADERS_DX11_IDX_REMAP.PAK", 12, 14);
				//LOAD(LevelTextures, Textures, "RENDERABLE/LEVEL_TEXTURES.ALL.PAK", 13, 14);

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
				updateProgress({LOADING_COMPOSITE, -1, "pretending"});

				//String ^shortGuidString = gcnew String(utf8Converter.from_bytes(copiedCommand->composite_name).c_str());
				//ShortGuid ^shortGuid = gcnew ShortGuid(shortGuidString);

				//gcnew String()

				Commands ^commandsPakTest = levelContentInstance->CommandsPAK;

				parseComposite(levelContentInstance->CommandsPAK->GetComposite(gcnew String(copiedCommand->composite_name.c_str())));

				//std::this_thread::sleep_for(std::chrono::seconds(2));

				updateProgress({READY, 0, ""});
			}
		}
	}
}

//OpenCAGELevelViewer_ContentManager_API void dummy() { }