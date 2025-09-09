#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <filesystem>
#include <optional>

namespace OpenCAGELevelViewer {
	namespace AllInOne {
		namespace ContentManagerNew {
			class Archive {

			};

			class Level {
				
			};

			class BehaviourTree {

			};

			class BehaviourTreeManager {

			};

			class LocalisationLanguageFile {
			public:
				LocalisationLanguageFile(const std::filesystem::path &localisationLangaugeFilePath);
				LocalisationLanguageFile() = default;

				std::filesystem::path localisationLanguageFilePath;

				void load();
				void save();

				//std::string getTranslation(std::string_view key);

				std::map < std::string, std::string > localisations {};
			};

			class LocalisationLanguage {
			public:
				LocalisationLanguage(const std::filesystem::path &localisationLangaugeRoot);
				LocalisationLanguage() = default;

				std::filesystem::path localisationLanguageRoot;

				void load();
				void save();

				/**
				* 
				*/
				std::map < std::string, LocalisationLanguageFile > localisations {};
			};

			class LocalisationManager {
			public:
				LocalisationManager(const std::filesystem::path &localisationRoot);
				LocalisationManager() = default;

				void load();
				void save();

				std::filesystem::path localisationRoot;
				
				std::map < std::string, LocalisationLanguage > languages {};
			};

			class GBLItem {
				GBLItem(const std::filesystem::path &gblItemPath);
				GBLItem();

				std::filesystem::path gblItemPath;

				void importFromXml();
				void exportToXml();

				struct GBLItemSpecialSlot {
					std::string name;
					int64_t x;
					int64_t y;
					int64_t width;
					int64_t height;
				};
				std::vector < GBLItemSpecialSlot > specialSlots;


				
			};

			class ConfigurationManager {
				LocalisationManager localisationManager;
			};

			class ContentManager {
			public:
				ContentManager(const std::filesystem::path &gameRoot);

				void refresh();

				const std::filesystem::path &getGameRoot() const { return gameRoot; }
			private:
				std::filesystem::path gameRoot;

				std::map < std::string, Level > levels;
				BehaviourTreeManager behaviour;



			};
		}
	}
}