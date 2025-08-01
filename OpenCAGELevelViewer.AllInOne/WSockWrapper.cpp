#include "WSockWrapper.h"

#include <cstdint>
#include <memory>
#include <string>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <map>
#include <assert.h>
#include <cassert>
#include <utility>
#include <ctime>
#include <chrono>

#pragma managed(push, off)
static std::pair<INT, int> unmanagedWSAStartup(WORD wVersionRequested, LPWSADATA lpWSAData) {
	int ret = WSAStartup(wVersionRequested, lpWSAData);
	int errorCode = WSAGetLastError();

	return {ret, errorCode};
}
#pragma managed(pop, off)

OpenCAGELevelViewer::WSockWrapper::WinSock::WinSock() {
	auto ret = unmanagedWSAStartup(MAKEWORD(2, 2), &wsaData);

	/*WSASetLastError(5);

	if (WSAGetLastError() != 5) {
		throw std::runtime_error("WinSock2 superbug detected. Halting immediately.");
		std::terminate();
	}*/

	if (ret.first)
		throw OpenCAGELevelViewer::WSockWrapper::WSAError(ret.first, ret.second);
}

OpenCAGELevelViewer::WSockWrapper::WinSock::~WinSock() {
	WSACleanup();
}

OpenCAGELevelViewer::WSockWrapper::AddrInfo::AddrInfo() {
	addrInfoPointer = std::shared_ptr<addrinfo>(new addrinfo {}, OpenCAGELevelViewer::WSockWrapper::simpleAddrInfoDeleter);
}

OpenCAGELevelViewer::WSockWrapper::AddrInfo::AddrInfo(const int family, const int sock_type, const int protocol) {
	addrInfoPointer = std::shared_ptr<addrinfo>(new addrinfo {}, OpenCAGELevelViewer::WSockWrapper::simpleAddrInfoDeleter);

	addrInfoPointer->ai_family = family;
	addrInfoPointer->ai_socktype = sock_type;
	addrInfoPointer->ai_protocol = protocol;
}

#pragma managed(push, off)
static std::pair<INT, int> unmanagedGetaddrinfo(PCSTR pNodeName, PCSTR pServiceName, const ADDRINFOA *pHints, PADDRINFOA *ppResult) {
	INT ret = getaddrinfo(pNodeName, pServiceName, pHints, ppResult);
	int errorCode = WSAGetLastError();

	return {ret, errorCode};
}
#pragma managed(pop, off)

OpenCAGELevelViewer::WSockWrapper::AddrInfo::AddrInfo(const std::string &host, const uint_least16_t port, const OpenCAGELevelViewer::WSockWrapper::AddrInfo & hints) {
	addrinfo *result;
	auto ret = unmanagedGetaddrinfo("localhost", /*std::to_string(port).c_str()*/"1702", hints.addrInfoPointer.get(), &result);

	if (ret.first)
		throw OpenCAGELevelViewer::WSockWrapper::WSAError(0, ret.second);

	addrInfoPointer = std::shared_ptr<addrinfo>(result, OpenCAGELevelViewer::WSockWrapper::generatedAddrinfoDeleter);
}

//OpenCAGELevelViewer::WSockWrapper::AddrInfo::~AddrInfo() { }

static std::map<SOCKET, uint_least8_t> socketRefCount;

#pragma managed(push, off)
static std::pair<SOCKET, int> unmanagedSocket(int af, int type, int protocol) {
	SOCKET ret = socket(af, type, protocol);
	int errorCode = WSAGetLastError();

	return {ret, errorCode};
}
#pragma managed(pop, off)

OpenCAGELevelViewer::WSockWrapper::Socket::Socket(int af, int type, int protocol) {
	auto ret = unmanagedSocket(af, type, protocol);
	_socket = ret.first;

	if (_socket == INVALID_SOCKET)
		throw OpenCAGELevelViewer::WSockWrapper::WSAError(_socket, ret.second);

	if (socketRefCount.contains(_socket)) {
		assert("Too many socket references.", socketRefCount[_socket] == 255);
		socketRefCount[_socket] += 1;
	} else {
		socketRefCount[_socket] = 1;
	}
}

OpenCAGELevelViewer::WSockWrapper::Socket::Socket(const Socket &other) {
	_socket = other._socket;
	assert("Too many socket references.", socketRefCount[_socket] == 255);
	socketRefCount[_socket] += 1;
}

OpenCAGELevelViewer::WSockWrapper::Socket OpenCAGELevelViewer::WSockWrapper::Socket::operator=(const Socket &other) {
	return Socket(other);
	//assert("Too many socket references.", socketRefCount[_socket] == 255);
	//socketRefCount[_socket] += 1;
}

OpenCAGELevelViewer::WSockWrapper::Socket::~Socket() {
	socketRefCount[_socket] -= 1;

	if (socketRefCount[_socket] == 0) {
		socketRefCount.erase(_socket);
		closesocket(_socket);
	}
}

#pragma managed(push, off)
static std::pair<int, int> unmanagedConnect(SOCKET s, sockaddr *name, int namelen) {
	int ret = connect(s, name, namelen);
	int errorCode = WSAGetLastError();

	return {ret, errorCode};
}
#pragma managed(pop, off)

void OpenCAGELevelViewer::WSockWrapper::Socket::_connect(const addrinfo &addrInfo) const {
	auto ret = unmanagedConnect(_socket, addrInfo.ai_addr, static_cast< int >(addrInfo.ai_addrlen));

	if (ret.first == SOCKET_ERROR) {
		throw OpenCAGELevelViewer::WSockWrapper::WSAError(ret.first, ret.second);
	}
}

#pragma managed(push, off)
static std::pair<int, int> unmanagedgetsockopt(SOCKET s, int level, int optname, char *optval, int *optlen) {
	int ret = getsockopt(s, level, optname, optval, optlen);
	int errorCode = WSAGetLastError();

	return {ret, errorCode};
}
#pragma managed(pop, off)

std::vector<char> OpenCAGELevelViewer::WSockWrapper::Socket::_getSockOpt(int level, int optname) const {
	int optvalSize = 64;

	std::vector<char> optvalArray(64);

	/*auto ret = unmanagedgetsockopt(_socket, level, optname, &t, &optlen);

	if (ret.first)
		throw OpenCAGELevelViewer::WSockWrapper::WSAError(ret.first, ret.second);

	std::vector<char> optval(optlen);*/

	auto ret = unmanagedgetsockopt(_socket, level, optname, optvalArray.data(), &optvalSize);

	if (ret.first)
		throw OpenCAGELevelViewer::WSockWrapper::WSAError(ret.first, ret.second);

	return optvalArray;
}

void OpenCAGELevelViewer::WSockWrapper::Socket::_setSockOpt(int level, int optname, const std::vector<char> &optval) const {
	int ret = setsockopt(_socket, level, optname, optval.data(), static_cast< int >(optval.size()));

	if (ret)
		throw OpenCAGELevelViewer::WSockWrapper::WSAError(ret, WSAGetLastError());
}

#pragma managed(push, off)
static std::pair<int, int> unmanagedioctlsocket(SOCKET s, unsigned long cmd, unsigned long *argp) {
	int ret = ioctlsocket(s, cmd, argp);
	int errorCode = WSAGetLastError();

	return {ret, errorCode};
}
#pragma managed(pop)

void OpenCAGELevelViewer::WSockWrapper::Socket::_ioCtlSocket(long cmd, unsigned long arg) const {
	auto ret = unmanagedioctlsocket(_socket, cmd, &arg);

	if (ret.first)
		throw OpenCAGELevelViewer::WSockWrapper::WSAError(ret.first, ret.second);
}

static constexpr fd_set getFdSetFromVector(const std::vector<OpenCAGELevelViewer::WSockWrapper::Socket> &sockets) {
	if (sockets.size() == FD_SETSIZE) {
		throw std::invalid_argument("Too many sockets to fit into an \"fd_set\".");
	}

	fd_set fdSet { };

	for (const auto &socket : sockets) {
		FD_SET(socket._socket, &fdSet);
	}

	return fdSet;
}

constexpr static OpenCAGELevelViewer::WSockWrapper::SocketSelectType getSocketSelectTypeFromFdSets(const OpenCAGELevelViewer::WSockWrapper::SocketSelectType originalSockets, const fd_set readSet, const fd_set writeSet, const fd_set exceptSet) noexcept {
	OpenCAGELevelViewer::WSockWrapper::SocketSelectType socketSelectType;
	
	for (const auto &socket : originalSockets.readSockets) {
		if (FD_ISSET(socket._socket, &readSet))
			socketSelectType.readSockets.push_back(socket);
	}

	for (const auto &socket : originalSockets.writeSockets) {
		if (FD_ISSET(socket._socket, &writeSet))
			socketSelectType.writeSockets.push_back(socket);
	}

	for (const auto &socket : originalSockets.exceptSockets) {
		if (FD_ISSET(socket._socket, &exceptSet))
			socketSelectType.exceptSockets.push_back(socket);
	}

	return socketSelectType;
}

#define TRANSFER_SOCKETSELECTTYPE_TO_FD_SET(custom, fdSet) for (const auto &socket : custom) { FD_SET(socket._socket, &fdSet); }
//#define TRANSFER_SOCKETSELECTTYPE_TO_FD_SETS(custom) fd_set custom_readSet, ##custom_writeSet, ##custom_exceptSet; TRANSFER_SOCKETSELECTTYPE_TO_FD_SET(custom.readSockets, custom_readSet); TRANSFER_SOCKETSELECTTYPE_TO_FD_SET(custom.writeSockets, custom_writeSet); TRANSFER_SOCKETSELECTTYPE_TO_FD_SET(custom.exceptSockets, custom_exceptSet);

#pragma managed(push, off)
static std::pair<int, int> unmanagedselect(int nfds, fd_set *readSet, fd_set *writeSet, fd_set *exceptSet, const timeval *timeout) {
	int ret = select(nfds, readSet, writeSet, exceptSet, timeout);
	int errorCode = WSAGetLastError();
	return {ret, errorCode};
}
#pragma managed(pop)

static inline void _select(fd_set *readSet, fd_set *writeSet, fd_set *exceptSet, const std::chrono::nanoseconds timeout) {
	auto _timeval = OpenCAGELevelViewer::WSockWrapper::toTimeval(timeout);
	auto ret = unmanagedselect(0, readSet, writeSet, exceptSet, &_timeval);
	if (ret.first == SOCKET_ERROR)
		throw OpenCAGELevelViewer::WSockWrapper::WSAError(ret.first, ret.second);
}

static OpenCAGELevelViewer::WSockWrapper::SocketStatusBitField getSocketStatusFromSocket(const OpenCAGELevelViewer::WSockWrapper::Socket &socket, const fd_set &readSet, const fd_set &writeSet, const fd_set &exceptSet) noexcept {
	OpenCAGELevelViewer::WSockWrapper::SocketStatusBitField socketStatus = 0;
	SOCKET wsaSocket = socket._socket;

	if (FD_ISSET(wsaSocket, &readSet))
		socketStatus |= OpenCAGELevelViewer::WSockWrapper::SocketStatus_Readable;

	if (FD_ISSET(wsaSocket, &writeSet))
		socketStatus |= OpenCAGELevelViewer::WSockWrapper::SocketStatus_Writable;

	if (FD_ISSET(wsaSocket, &exceptSet))
		socketStatus |= OpenCAGELevelViewer::WSockWrapper::SocketStatus_Excepted;

	return socketStatus;
}

std::vector<OpenCAGELevelViewer::WSockWrapper::SocketStatusBitField> OpenCAGELevelViewer::WSockWrapper::Select(const std::vector<OpenCAGELevelViewer::WSockWrapper::Socket> &sockets, const std::chrono::nanoseconds timeout) {
	fd_set set = getFdSetFromVector(sockets);
	fd_set readSet = set, writeSet = set, exceptSet = set;

	_select(&readSet, &writeSet, &exceptSet, timeout);

	std::vector<OpenCAGELevelViewer::WSockWrapper::SocketStatusBitField> ret(sockets.size(), 0);

	for (size_t i = 0; i < sockets.size(); ++i)
		ret[i] = getSocketStatusFromSocket(sockets[i], readSet, writeSet, exceptSet);

	return ret;
}

OpenCAGELevelViewer::WSockWrapper::SocketSelectType OpenCAGELevelViewer::WSockWrapper::Select(const SocketSelectType &socketsSelected, const std::chrono::nanoseconds timeout) {
	//TRANSFER_SOCKETSELECTTYPE_TO_FD_SETS(socketsSelected);

	fd_set 
		readSet = getFdSetFromVector(socketsSelected.readSockets),
		writeSet = getFdSetFromVector(socketsSelected.writeSockets),
		exceptSet = getFdSetFromVector(socketsSelected.exceptSockets);

	_select(&readSet, &writeSet, &exceptSet, timeout);

	return getSocketSelectTypeFromFdSets(socketsSelected, readSet, writeSet, exceptSet);
}

bool OpenCAGELevelViewer::WSockWrapper::Socket::selectReadable(const std::chrono::nanoseconds timeout) const {
	fd_set set {};
	FD_SET(this->_socket, &set);

	_select(&set, nullptr, nullptr, timeout);

	return FD_ISSET(this->_socket, &set);
}

bool OpenCAGELevelViewer::WSockWrapper::Socket::selectWritable(const std::chrono::nanoseconds timeout) const {
	fd_set set {};
	FD_SET(this->_socket, &set);

	_select(nullptr, &set, nullptr, timeout);

	return FD_ISSET(this->_socket, &set);
}

bool OpenCAGELevelViewer::WSockWrapper::Socket::selectExceptable(const std::chrono::nanoseconds timeout) const {
	fd_set set {};
	FD_SET(this->_socket, &set);

	_select(nullptr, nullptr, &set, timeout);

	return FD_ISSET(this->_socket, &set);
}

OpenCAGELevelViewer::WSockWrapper::SocketStatusBitField OpenCAGELevelViewer::WSockWrapper::Socket::select(const std::chrono::nanoseconds timeout) const {
	fd_set readSet {}, writeSet {}, exceptSet {};

	FD_SET(this->_socket, &readSet);
	FD_SET(this->_socket, &writeSet);
	FD_SET(this->_socket, &exceptSet);

	_select(&readSet, &writeSet, &exceptSet, timeout);

	return getSocketStatusFromSocket(*this, readSet, writeSet, exceptSet);
}

constexpr size_t outputBufferIncrements = 2048;

#pragma managed(push, off)
static std::pair<int, int> unmanagedrecv(SOCKET s, char *buf, int len, int flags) {
	int ret = recv(s, buf, len, flags);
	int errorCode = WSAGetLastError();
	return {ret, errorCode};
}
#pragma managed(pop)

std::vector<char> OpenCAGELevelViewer::WSockWrapper::Socket::Recv() const {
	std::vector<char> outputBuffer {};

	std::pair<int, int> recvResult;
	
	while (true) {
		outputBuffer.resize(outputBuffer.size() + outputBufferIncrements);
		recvResult = unmanagedrecv(_socket, outputBuffer.data(), static_cast< int >(outputBuffer.size()), MSG_PEEK);

		if (recvResult.first >= 0) {
			if (WSAGetLastError() != WSAEMSGSIZE) {
				outputBuffer.resize(recvResult.first);
				break;
			}
		} else if (recvResult.first == SOCKET_ERROR) {
			throw OpenCAGELevelViewer::WSockWrapper::WSAError(recvResult.first, recvResult.second);
		}
	}

	recvResult = unmanagedrecv(_socket, outputBuffer.data(), static_cast< int >(outputBuffer.size()), 0);
	if (recvResult.first == SOCKET_ERROR) {
		throw OpenCAGELevelViewer::WSockWrapper::WSAError(recvResult.first, recvResult.second);
	}

	return outputBuffer;
}

void OpenCAGELevelViewer::WSockWrapper::Socket::Send(const std::vector<char> &data) const {
	int ret = send(_socket, data.data(), data.size(), 0);
	if (ret == SOCKET_ERROR) {
		throw OpenCAGELevelViewer::WSockWrapper::WSAError(ret, WSAGetLastError());
	}
}