// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

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

#include <vector>
#include <map>
#include <unordered_map>
#include <string>
#include <filesystem>
#include <optional>
#include <variant>
#include <mutex>
#include <memory>
#include <atomic>
#include <cstdint>
#include <type_traits>

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

#define IMGUI_DEFINE_MATH_OPERATORS

namespace OpenCAGELevelViewer::AllInOne {
#pragma managed(push, off)
	const std::string &getApplicationPath(void);
	const std::filesystem::path &getApplicationPathAsStdPath(void);
#pragma managed(pop)

#pragma managed(push, off)
	inline size_t combineHash(std::size_t a, std::size_t b) {
		return a ^ (b + 0x9e3779b9 + (a << 6) + (a >> 2));
	}
#pragma managed(pop)

	template < typename T >
	constexpr std::vector < T > convertCliArray(array < T > ^tArray) {
		std::vector < T > tVector = std::vector < T >(tArray->Length);
		System::Runtime::InteropServices::Marshal::Copy(tArray, 0, System::IntPtr(tVector.data()), tArray->Length);

		return tVector;
	}
}

extern msclr::gcroot< msclr::interop::marshal_context ^ > msclr_context;

#define MarshalCliString(cliString) msclr_context->marshal_as<const char *>(cliString)
#define ConvertCliStringToCXXString(cliString) std::string(MarshalCliString(cliString))

#endif //PCH_H
