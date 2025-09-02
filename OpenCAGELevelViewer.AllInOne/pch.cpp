// pch.cpp: source file corresponding to the pre-compiled header

#include "pch.h"

#include <filesystem>
#include <string>

msclr::gcroot < msclr::interop::marshal_context ^ > msclr_context = gcnew msclr::interop::marshal_context();

bool initalisedApplicationPath = false;
std::string applicationPath;
std::filesystem::path applicationPathPath;

static void initaliseApplicationPath() {
	applicationPath = std::string(SDL_GetBasePath());
	applicationPathPath = std::filesystem::path(applicationPath);
}

const std::string &OpenCAGELevelViewer::AllInOne::getApplicationPath() {
	if (!initalisedApplicationPath)
		initaliseApplicationPath();
	return applicationPath;
}

const std::filesystem::path &OpenCAGELevelViewer::AllInOne::getApplicationPathAsStdPath() {
	if (!initalisedApplicationPath)
		initaliseApplicationPath();
	return applicationPathPath;
}

//void OpenCAGELevelViewer::AllInOne::gcCleanup() {
//	{
//		System::GC::Collect();
//		System::GC::WaitForPendingFinalizers();
//		System::GC::Collect();
//		System::GC::WaitForPendingFinalizers();
//	}
//}

// When you are using pre-compiled headers, this source file is necessary for compilation to succeed.
