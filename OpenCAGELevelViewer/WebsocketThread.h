//#ifdef _MSC_VER
//#   define _WIN32_WINNT 0x0601
//#   pragma warning (disable: 4068)
//#   pragma comment (lib, "Crypt32.lib")
//#	ifdef _RESUMABLE_FUNCTIONS_SUPPORTED
//#		undef _RESUMABLE_FUNCTIONS_SUPPORTED
//#	endif
//#	ifndef __cpp_lib_coroutine
//#		define __cpp_lib_coroutine
//#	endif
//#endif
//
//#include <fv/declare.hpp>

#include <atomic>
#include <mutex>
namespace OpenCAGELevelViewer {
	namespace WebsocketThread {
		extern std::atomic_flag connect;
		extern std::atomic_flag connected;
		extern std::atomic_flag ready;

		extern std::atomic_flag keepThreadActive;

		struct OpenCAGECommandsEditorState {
			long long version;
		};

		extern std::recursive_mutex commandsEditorStateMutex;

		void main(void);
	}
}