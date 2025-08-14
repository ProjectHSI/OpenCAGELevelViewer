// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// here to make sure everything is managed by default, because sometimes it isn't being managed by default.
#pragma managed(on)

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#pragma warning( push )
#pragma warning( disable : 4820 )
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#pragma warning( pop )

// SINNER
#undef min
#undef max

#include <msclr/gcroot.h>
#include <msclr/marshal.h>

#include <SDL3/SDL.h>

#include <glbinding/CallbackMask.h>
#include <glbinding/FunctionCall.h>
#include <glbinding/glbinding.h>
#include <glbinding/Version.h>
#include <glbinding/getProcAddress.h>
#include <glbinding/gl/gl.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#endif //PCH_H
