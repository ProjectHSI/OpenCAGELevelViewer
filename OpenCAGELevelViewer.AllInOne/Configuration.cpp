#include "pch.h"

#include "Configuration.h"
#include <algorithm>
#include <fstream>

constexpr const char *saveLocation = "configuration.bin";
constexpr OpenCAGELevelViewer::AllInOne::Configuration::Version currentVersion {0, 0, 2};

OpenCAGELevelViewer::AllInOne::Configuration::Configuration OpenCAGELevelViewer::AllInOne::Configuration::configuration {};

static void defaultValues() {
	OpenCAGELevelViewer::AllInOne::Configuration::configuration.version = currentVersion;
	OpenCAGELevelViewer::AllInOne::Configuration::configuration.vsync = 1; // Default to vsync enabled.
	for (auto &profile : OpenCAGELevelViewer::AllInOne::Configuration::configuration.profile) {
		profile.exists = false;
		//std::fill(std::begin(profile.profileName), std::end(profile.profileName), '\0');
		std::fill(std::begin(profile.gamePath), std::end(profile.gamePath), '\0');
	}
}

void OpenCAGELevelViewer::AllInOne::Configuration::load() {
	std::ifstream istrm(getApplicationPathAsStdPath() / saveLocation, std::ios::binary);

	if (!istrm) {
		defaultValues();
		return;
	}

	std::streampos fsize = 0;
	{
		fsize = istrm.tellg();
		istrm.seekg(0, std::ios::end);
		fsize = istrm.tellg() - fsize;
	}

	istrm.seekg(0, std::ios::beg);

	if (fsize != sizeof configuration) {
		defaultValues();
		istrm.close();
		return;
	}

	istrm.read(reinterpret_cast< char * >(&configuration), sizeof configuration);

	istrm.close(); // close - no need to keep it open.

	if (configuration.version != currentVersion) {
		defaultValues();
		return;
	}

	// already loaded, no need to do anything.
}

void OpenCAGELevelViewer::AllInOne::Configuration::save() {
	std::ofstream ostrm(getApplicationPathAsStdPath() / saveLocation, std::ios::binary | std::ios::trunc);

	ostrm.write(reinterpret_cast< const char * >(&configuration), sizeof configuration);

	ostrm.close();
}