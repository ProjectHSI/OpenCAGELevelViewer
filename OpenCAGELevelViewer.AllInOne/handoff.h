#pragma once

#ifdef OpenCAGELevelViewer_AllInOne_EXPORTS
#define OpenCAGELevelViewer_AllInOne_API __declspec(dllexport)
#else
#define OpenCAGELevelViewer_AllInOne_API __declspec(dllimport)
#endif

OpenCAGELevelViewer_AllInOne_API int handoff(char **argv, int argc);

//__declspec(dllexport) void dummy();