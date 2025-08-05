#include "ContentManager.h"



void OpenCAGELevelViewer::AllInOne::ContentManager::threadMain() {
//	std::string levelPath = std::string("D:/Steam/steamapps/common/Alien Isolation") + "/DATA/ENV/PRODUCTION/" + "BSP_TORRENS" + "/";
//	std::string worldPath = levelPath + "WORLD/";
//	std::string renderablePath = levelPath + "RENDERABLE/";
//
//	//renderablePath.
//
//	//updateProgress({LOADING_LEVEL, static_cast< float >(1) / 4, std::string("Loading ") + "t" + "."});
//
//#define UPDATE_PROGRESS_FOR_LOAD(FilePath, Index, Max) /*updateProgress({LOADING_LEVEL, static_cast<float>(##Index) / ##Max, std::string("Loading \"") + FilePath + "\"."})*/
//#define GET_FULL_LEVEL_SPECIFIC_PATH(FilePath) (copiedCommand->alien_path + "/DATA/ENV/PRODUCTION/" + copiedCommand->level_name + "/" + FilePath)
//
//#define LOAD_WITHOUT_WAIT(VarName, CSType, FilePath, Index, Max) \
//UPDATE_PROGRESS_FOR_LOAD(FilePath, Index, Max); \
//levelContentInstance->VarName = gcnew CSType(gcnew String(/*utf8Converter.from_bytes(*/GET_FULL_LEVEL_SPECIFIC_PATH(FilePath)/*)*/.c_str()));
//
//#define LOAD(VarName, CSType, FilePath, Index, Max) LOAD_WITHOUT_WAIT(VarName, CSType, FilePath, Index, Max); while (levelContentInstance->VarName->Loaded == false) std::this_thread::yield()//; printf("%i", VarName->Loaded);
//
//	//updateProgress({LOADING_LEVEL, 0, "Loading " + });
//	//auto ModelsMVR = gcnew CATHODE::Movers(worldPath + "MODELS.MVR");
//	//auto CommandsPAK = gcnew CATHODE::Commands(worldPath + "COMMANDS.PAK");
//	LOAD(ModelsMVR, Movers, "WORLD/MODELS.MVR", 0, 12);
//	LOAD(CommandsPAK, Commands, "WORLD/COMMANDS.PAK", 1, 12);
//	//printf("%i\n", levelContentInstance->CommandsPAK->Loaded);
//	LOAD(RenderableREDS, RenderableElements, "WORLD/REDS.BIN", 2, 12);
//	LOAD(ResourcesBIN, CATHODE::Resources, "WORLD/RESOURCES.BIN", 3, 12);
//	LOAD(PhysicsMap, PhysicsMaps, "WORLD/PHYSICS.MAP", 4, 12);
//	LOAD(EnvironmentMap, EnvironmentMaps, "WORLD/ENVIRONMENTMAP.BIN", 5, 12);
//	LOAD(CollisionMap, CollisionMaps, "WORLD/COLLISION.MAP", 6, 12);
//	//// "class does not have a copy constructor"
//	//LOAD(EnvironmentAnimation, EnvironmentAnimations, worldPath + "ENVIRONMENT_ANIMATION.DAT", 7, 12);
//	///*UPDATE_PROGRESS_FOR_LOAD("WORLD/ENVIRONMENT_ANIMATION.DAT", 6, 13);
//	//auto EnvironmentAnimation = gcnew CATHODE::EnvironmentAnimations(
//	//	gcnew String(utf8Converter.from_bytes(GET_FULL_LEVEL_SPECIFIC_PATH("WORLD/ENVIRONMENT_ANIMATION.DAT")).c_str()),
//	//	gcnew CATHODE::AnimationStrings(gcnew String(utf8Converter.from_bytes((copiedCommand->alien_path + "/DATA/GLOBAL/ANIMATION.PAK")).c_str())));*/
//	//
//	////(gcnew String(((copiedCommand->alien_path + "/DATA/ENV/PRODUCTION/" + copiedCommand->level_name + "/" + worldPath + "ENVIRONMENT_ANIMATION.DAT").c_str())));
//	//// TODO: ModelsCST?
//	// 7
//	LOAD(ModelsMTL, Materials, "RENDERABLE/LEVEL_MODELS.MTL", 8, 12);
//	LOAD(ModelsPAK, Models, "RENDERABLE/LEVEL_MODELS.PAK", 9, 12);
//	LOAD(Shaders, Shaders, "RENDERABLE/LEVEL_SHADERS_DX11.PAK", 10, 12); // we need might to recompile these for OpenGL if directx isn't simple enough for me to understand
//	//LOAD_WITHOUT_WAIT(ShadersIDXRemap, IDXRemap, "RENDERABLE/LEVEL_SHADERS_DX11_IDX_REMAP.PAK", 11, 13);
//	LOAD(LevelTextures, Textures, "RENDERABLE/LEVEL_TEXTURES.ALL.PAK", 11, 12);
}