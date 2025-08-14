#pragma once

#include "pch.h"

#include <cstdint>
#include <memory>
#include <string>

#include <vector>
#include <tuple>
#include <chrono>
#include <initializer_list>
#include <system_error>
#include <array>
#include <bit>
#include <cstring>

//#define SOCKET_SELECT_TYPE_STRUCT 0
//#define SOCKET_SELECT_TYPE_TYPE 1
//#define SOCKET_SELECT_TYPE SOCKET_SELECT_TYPE_STRUCT

namespace OpenCAGELevelViewer {
	namespace WSockWrapper {
		class WSAError : public std::system_error {
		public:
			const int functionReturnCode;
			WSAError(int errorCode, int wsaCode) : std::system_error(std::error_code(wsaCode, std::system_category())), functionReturnCode(errorCode) { }
		};

		class WinSock {
		private:
			WSAData wsaData;
		public:
			const WSAData &getWSAData() const {
				return wsaData;
			}

			WinSock();
			~WinSock();
		};

		inline void simpleAddrInfoDeleter(addrinfo *addrInfo) {
			delete addrInfo;
		}

		inline void generatedAddrinfoDeleter(addrinfo *addrInfo) {
			freeaddrinfo(addrInfo);
		}
		
		class AddrInfo {
		public:
			std::shared_ptr<addrinfo> addrInfoPointer {};

			AddrInfo();
			AddrInfo(AddrInfo &other) = default;
			AddrInfo &operator=(const AddrInfo &other) = default;
			AddrInfo(const int family, const int sock_type, const int protocol);
			AddrInfo(const std::string &host, const uint_least16_t port, const AddrInfo &hints);
			//~AddrInfo();
		};

		enum SocketStatus {
			SocketStatus_Readable = 0b001,
			SocketStatus_Writable = 0b010,
			SocketStatus_Excepted = 0b100
		};
		typedef unsigned char SocketStatusBitField;

		inline timeval toTimeval(const std::chrono::nanoseconds timeout) {
			timeval tv {};

			decltype(timeval::tv_usec) nanoSecondCount = std::nano::den / std::nano::num;

			tv.tv_sec = timeout.count() / nanoSecondCount;
			tv.tv_usec = timeout.count() % nanoSecondCount;

			return tv;
		}

		class Socket {
		public:
			SOCKET _socket = INVALID_SOCKET;

			Socket(int af, int type, int protocol);
			Socket(const Socket &other);
			Socket operator=(const Socket &other);
			~Socket();


			void _connect(const addrinfo &addrInfo) const;
			void _connect(const AddrInfo &addrInfo) const {
				_connect(*addrInfo.addrInfoPointer.get());
			}


			std::vector<char> _getSockOpt(int level, int optname) const;

			template<typename T>
			T getSockOpt(int level, int optname) const {
				T t {};
				std::vector<char> charArray = _getSockOpt(level, optname);

				std::memcpy(&t, charArray.data(), std::min(sizeof(t), charArray.size()));

				return t;
			}

			template<>
			inline std::string getSockOpt(int level, int optname) const {
				auto stringVector = getSockOpt<std::vector<char>>(level, optname);

				return std::string(stringVector.begin(), stringVector.end());
			}


			void _setSockOpt(int level, int optname, const std::vector<char> &optval) const;

			template<typename T>
			inline void setSockOpt(int level, int optname, const T *optval, size_t optlen) const {
				/*std::array<char, optlen> tAsArray(reinterpret_cast<char *>(& optval), optlen);*/

				std::vector<char> tAsArray(optlen);
				memcpy(tAsArray.data(), optval, optlen);

				_setSockOpt(level, optname, tAsArray);
			}

			template<typename T>
			inline void setSockOpt(int level, int optname, const T &optval) const {
				setSockOpt(level, optname, optval, sizeof(optval));
			}
			
			template<>
			inline void setSockOpt<std::string>(int level, int optname, const std::string &optval) const {
				setSockOpt<const char>(level, optname, optval.c_str(), static_cast<int>(optval.size() + 1));
			}


			void _ioCtlSocket(long cmd, unsigned long arg) const;

			template<typename T>
			inline void ioCtlSocket(long cmd, const T &arg) const {
				_ioCtlSocket(cmd, static_cast<unsigned long>(arg));
			}

		#define SOCK_OPT(level, name, sockOptFriendly, type) type get_sockOptFriendly() const { return getsockopt<type>(level, name); } type get_sockOptFriendly(type value) const { return setsockopt<type>(level, name, value); }
		#define IOCTL_OPT(cmd, ioCtlFriendly, type) void set_##ioCtlFriendly(type value) const { ioCtlSocket<type>(cmd, value); }

			IOCTL_OPT(FIONBIO, nonBlocking, const bool);

			bool selectReadable(const std::chrono::nanoseconds timeout = std::chrono::nanoseconds(0)) const;
			bool selectWritable(const std::chrono::nanoseconds timeout = std::chrono::nanoseconds(0)) const;
			bool selectExceptable(const std::chrono::nanoseconds timeout = std::chrono::nanoseconds(0)) const;

			SocketStatusBitField select(const std::chrono::nanoseconds timeout = std::chrono::nanoseconds(0)) const;

			/*template<typename T>
			T Recv() const {
				Recv
			}*/
			//template<>
			std::vector<char> Recv() const;

			void Send(const std::vector<char> &data) const;
			/*inline void Send(const std::string &data) const {
				Send(std::vector(data.begin(), data.end()));
			}*/
		};

	//#if (SOCKET_SELECT_TYPE == SOCKET_SELECT_TYPE_TYPE)
	//	typedef std::tuple<std::vector<const Socket &>, std::vector<const Socket &>, std::vector<const Socket &>> SocketSelectType;
	//#elif (SOCKET_SELECT_TYPE == SOCKET_SELECT_TYPE_STRUCT)
		struct SocketSelectType {
			std::vector<Socket> readSockets {};
			std::vector<Socket> writeSockets {};
			std::vector<Socket> exceptSockets {};

			constexpr inline SocketSelectType() { }

			constexpr inline SocketSelectType(const std::initializer_list<const std::vector<Socket>> &socketVectors) {
				if (socketVectors.size() != 3) {
					throw std::invalid_argument("SocketSelectType requires exactly 3 vectors.");
				}

				readSockets = socketVectors.begin()[0];
				writeSockets = socketVectors.begin()[1];
				exceptSockets = socketVectors.begin()[2];
			}
		};
	//#endif

		std::vector<SocketStatusBitField> Select(const std::vector<Socket> &sockets, const std::chrono::nanoseconds timeout = std::chrono::nanoseconds(0));
		SocketSelectType Select(const SocketSelectType &socketsSelected, const std::chrono::nanoseconds timeout = std::chrono::nanoseconds(0));
	}
}