#include "WebsocketThread.h"

#include "WSockWrapper.h"
#include <array>
#include <atomic>
#include <chrono>
#include <iostream>
#include <limits>
#include <msclr/gcroot.h>
#include <msclr/marshal.h>
#include <mutex>
#include <nlohmann/json.hpp>
#include <random>
#include <source_location>
#include <system_error>
#include <variant>
//#include <boost/multiprecision/tommath.hpp>


#pragma comment(lib, "Ws2_32.lib")

//#include <ContentManager.h>

//#include <Level.h>

using namespace std::chrono_literals;

std::string commandsEditorHost = "127.0.0.1";
std::string commandsEditorPort = "1702";
std::string commandsEditorCombined = commandsEditorHost + ":" + commandsEditorPort;
std::string commandsEditorPath = "/commands_editor";

std::atomic_flag OpenCAGELevelViewer::WebsocketThread::willConnect = ATOMIC_FLAG_INIT;
std::atomic_flag OpenCAGELevelViewer::WebsocketThread::connected = ATOMIC_FLAG_INIT;
std::atomic_flag OpenCAGELevelViewer::WebsocketThread::ready = ATOMIC_FLAG_INIT;
std::atomic_flag wasConnected = ATOMIC_FLAG_INIT; // for internal use, just meant to be for checking if the connection was established, before doing the busy wait.

std::atomic_flag OpenCAGELevelViewer::WebsocketThread::keepThreadActive;

//bool OpenCAGELevelViewer::WebsocketThread::isConnected(void) {

//}

//void on_open(websocketpp::connection_hdl hdl, WebsocketPPClient *c) {
//    std::cout << "WebSocket connection opened!" << std::endl;
//    std::error_code ec;
//    //client::connection_ptr con = c->get_con_from_hdl(hdl, ec);
//
//    //if (ec) {
//    //    std::cout << "Failed to get connection pointer: " << ec.message() << std::endl;
//    //    return;
//    //}
//    //std::string payload = "{\"userKey\":\"API_KEY\", \"symbol\":\"EURUSD,GBPUSD\"}";
//    //c->send(con, payload, websocketpp::frame::opcode::text);
//}
//
//void on_message(websocketpp::connection_hdl, WebsocketPPClient::message_ptr msg) {
//    std::cout << "Received message: " << msg->get_payload() << std::endl;
//}
//
//void on_fail(websocketpp::connection_hdl hdl) {
//    std::cout << "WebSocket connection failed!" << std::endl;
//}
//
//void on_close(websocketpp::connection_hdl hdl) {
//    std::cout << "WebSocket connection closed!" << std::endl;
//}

//WebsocketPPClient websocketppClient;

//namespace beast = boost::beast;         // from <boost/beast.hpp>
//namespace http = beast::http;           // from <boost/beast/http.hpp>
//namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
//namespace net = boost::asio;            // from <boost/asio.hpp>
//using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

// taken directly from OpenCAGE's Commands Editor
enum MessageType {
	LEVEL_LOADED,

	COMPOSITE_SELECTED,
	COMPOSITE_RELOADED,
	COMPOSITE_DELETED,
	COMPOSITE_ADDED,

	ENTITY_SELECTED,
	ENTITY_MOVED,
	ENTITY_DELETED,
	ENTITY_ADDED,
	ENTITY_RESOURCE_MODIFIED,

	GENERIC_DATA_SYNC,
};

//beast::flat_buffer readBuffer;

//static void enterCallback(const brynet::net::TcpConnection::Ptr &session) {
//	session->setDataCallback([session](brynet::base::BasePacketReader &reader) {
//		//session->send(reader.begin(), reader.size());
//		handleMessage(std::string {reader.begin(), reader.size()});
//		reader.consumeAll(); 
//	});
//}

static void handleMessage(const std::string &data/*beast::error_code const& ec, std::size_t bytes_written*/) {
	//std::cout << "called..." << std::endl;

	//if (ec.value() != 0) {
	//	__debugbreak();
	//}

	std::cout << "\t" << "DATA | " << data << std::endl;

	auto parsedJson = nlohmann::json::parse(data);

	MessageType messageType = static_cast< MessageType >(parsedJson["packet_event"]);

	long long version;

	switch (messageType) {
		case GENERIC_DATA_SYNC: // ??? 
			if (parsedJson["version"] != 4) {
				OpenCAGELevelViewer::WebsocketThread::willConnect.clear();

				std::cout << "\tVersion mismatch." << std::endl;
			}

			OpenCAGELevelViewer::WebsocketThread::ready.test_and_set();

			break;

			/*
		case LOAD_LEVEL:
			// ignore the packet as it's basically meaningless.
			// it's done every time before LOAD_COMPOSITE for no reason
			break;

		case LOAD_COMPOSITE:
			std::lock_guard lockGuard((*OpenCAGELevelViewer::ContentManager::getContentManagerContext()).commandMutex);

			OpenCAGELevelViewer::ContentManager::LoadCompositeCommand newCommand;

			newCommand.level_name = parsedJson["level_name"];
			newCommand.alien_path = parsedJson["alien_path"];
			newCommand.composite_name = parsedJson["composite_name"];

			(*OpenCAGELevelViewer::ContentManager::getContentManagerContext()).command = newCommand;

			break;
			*/
	}
}

// ...
//static void readThread(websocket::stream<tcp::socket> *ws) {
//	try {
//		beast::flat_buffer readBuffer;
//
//		while (OpenCAGELevelViewer::WebsocketThread::willConnect.test()) {
//			std::cout << "Reading..." << std::endl;
//			(*ws).read(readBuffer);
//
//			std::cout << "\t" << beast::make_printable(readBuffer.data()) << std::endl;
//
//			auto parsedJson = nlohmann::json::parse(beast::buffers_to_string(readBuffer.data()));
//
//
//
//			MessageType messageType = static_cast< MessageType >(parsedJson["packet_event"]);
//
//			long long version;
//
//			switch (messageType) {
//				case GENERIC_DATA_SYNC: // ??? 
//					if (parsedJson["version"] != 4) {
//						OpenCAGELevelViewer::WebsocketThread::willConnect.clear();
//
//						std::cout << "\tVersion mismatch." << std::endl;
//					}
//
//					OpenCAGELevelViewer::WebsocketThread::ready.test_and_set();
//
//					break;
//
//					/*
//				case LOAD_LEVEL:
//					// ignore the packet as it's basically meaningless.
//					// it's done every time before LOAD_COMPOSITE for no reason
//					break;
//
//				case LOAD_COMPOSITE:
//					std::lock_guard lockGuard((*OpenCAGELevelViewer::ContentManager::getContentManagerContext()).commandMutex);
//
//					OpenCAGELevelViewer::ContentManager::LoadCompositeCommand newCommand;
//
//					newCommand.level_name = parsedJson["level_name"];
//					newCommand.alien_path = parsedJson["alien_path"];
//					newCommand.composite_name = parsedJson["composite_name"];
//
//					(*OpenCAGELevelViewer::ContentManager::getContentManagerContext()).command = newCommand;
//
//					break;
//					*/
//			}
//
//			//long long version = parsedJson["version"].get<long long>();
//
//
//
//			readBuffer.clear();
//		}
//	} catch (std::system_error e) {
//		//if (e.code())
//
//		__debugbreak();
//	} catch (boost::wrapexcept<boost::system::system_error> e) {
//		std::cout << "Boost System Error: " << e.code().message() << std::endl;
//
//		__debugbreak();
//	} catch (...) {
//		__debugbreak();
//	}
//}

/*
	!!! IMPORTANT NOTICE !!!
	Do not replace the code in this function with any websocket library, networking code or otherwise.
	Nothing else works, I've tried several libraries and they either don't work, have missing features, or have broken features. Believe me, I've tried.

	For now, I'm sticking with Winsock directly, since I can't be bothered to find a library that works, this'll work anyway.

	Graveyard:
		- Boost's "Beast" Library - read_async doesn't work (skill issue?)
		- ixwebsocket             - Doesn't connect
		- brynet                  - Doesn't connect
		- libdatachannel          - Doesn't connect
		- WebsocketPP/++          - I forgot how this one didn't work, I think it doesn't connect.
*/

//template<typename T>
//constexpr size_t generateNonceNumberIterations() {
//	return std::numeric_limits< NonceNumber >().digits / std::numeric_limits<T>().digits;
//}

// THE BASE64 LIBRARIES ALL USE STRINGS
// WILL I HAVE TO IMPLEMENT EVERYTHING MYSELF?


constexpr const char getBase64CharFromByte(const unsigned char byte) {
	if (byte >= 0 && byte <= 25) {
		return 'A' + byte;
	} else if (byte >= 26 && byte <= 51) {
		return 'a' + (byte - 26);
	} else if (byte >= 52 && byte <= 61) {
		return '0' + (byte - 52);
	} else if (byte == 62) {
		return '+';
	} else if (byte == 63) {
		return '/';
	} else {
		return '\0'; // Invalid byte for Base64
	}
}

typedef char Base64Digit;

std::string generateBase64String(const std::vector<Base64Digit> &data) {
	std::string buffer;

	for (size_t i = 0; i < data.size() / 3; i++) {
		uint32_t _24grouping {};
		_24grouping |= (static_cast< uint32_t >(data[3 * i]) & 0xFF) << 16;
		_24grouping |= (static_cast< uint32_t >(data[3 * i + 1]) & 0xFF) << 8;
		_24grouping |= (static_cast< uint32_t >(data[3 * i + 2]) & 0xFF);

		std::array<const Base64Digit, 4> base64CharsPerGrouping {};

		for (size_t v = 0; v < 4; v++) {
			buffer.push_back(getBase64CharFromByte(static_cast< Base64Digit >((_24grouping >> (18 - (6 * v))) & 0b111111)));
		}
	}

	if (data.size() % 3 != 0) {
		uint16_t _grouping {};
		if (data.size() % 3 == 2) {
			_grouping |= (static_cast< uint8_t >(data[data.size() - 2]) & 0xFF) << 8;
			_grouping |= (static_cast< uint8_t >(data[data.size() - 1]) & 0xFF);

			buffer.push_back(getBase64CharFromByte(static_cast< Base64Digit >((_grouping >> 10 & 0b111111))));
			buffer.push_back(getBase64CharFromByte(static_cast< Base64Digit >((_grouping >> 4 & 0b111111))));
			buffer.push_back(getBase64CharFromByte(static_cast< Base64Digit >((_grouping << 2 & 0b111100))));

			buffer += "=";
		} else if (data.size() % 3 == 1) {
			_grouping |= (static_cast< uint8_t >(data[data.size() - 1]) & 0xFF);

			buffer.push_back(getBase64CharFromByte(static_cast< Base64Digit >((_grouping >> 2 & 0b111111))));
			buffer.push_back(getBase64CharFromByte(static_cast< Base64Digit >((_grouping << 4 & 0b110000))));

			buffer += "==";
		} else {
			[[unreachable]];
		}
	}

	return buffer;
}

typedef std::pair<std::string, std::string> NoncePair;
//typedef boost::multiprecision::uint128_t NonceNumber;
constexpr size_t NonceBytes = 10;
typedef std::random_device NonceDevice;
typedef NonceDevice::result_type NonceDeviceResult;
// typedef NonceDeviceResult NonceDeviceResultCast // enable if the static assertion belows suceeds with this value. atm it doesn't
												   // generally you want to use this when you can, but we can't. so...
typedef unsigned short NonceDeviceResultCast;
NonceDevice nonceDevice;

NoncePair generateNoncePair() {
	/*NonceNumber nonce = static_cast< NonceNumber >(nonceDevice());

	for (size_t i = 0; i < generateNonceNumberIterations<NonceDeviceResult>(); ++i) {
		nonce <<= std::numeric_limits<NonceDeviceResult>().digits;
		nonce |= static_cast< NonceNumber >(nonceDevice());
	}*/

	std::vector<Base64Digit> buffer(NonceBytes);

	constexpr int sizeOfNonceDeviceResultCast = sizeof(NonceDeviceResultCast);

	static_assert(NonceBytes % sizeOfNonceDeviceResultCast == 0);

	for (size_t i = 0; i < NonceBytes / sizeOfNonceDeviceResultCast; i++) {
		NonceDeviceResultCast result = static_cast< NonceDeviceResultCast >(nonceDevice());
		for (size_t v = 0; v < sizeOfNonceDeviceResultCast; v++) {
			buffer[( v ) +(i * sizeOfNonceDeviceResultCast)] = (result >> ((sizeOfNonceDeviceResultCast - 1 - v) * 8)) & 0b11111111;
		}
	}

	NoncePair noncePair;
	noncePair.first = generateBase64String(buffer);
	std::string secondNoncePair = noncePair.first + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	noncePair.second = generateBase64String(std::vector<Base64Digit>(secondNoncePair.begin(), secondNoncePair.end()));

	return noncePair;
}


WSADATA wsaData {};

std::unordered_map<std::string, std::string> base64TestVectors = {
	{"", ""},
	{"f", "Zg=="},
	{"fo", "Zm8="},
	{"foo", "Zm9v"},
	{"foob", "Zm9vYg=="},
	{"fooba", "Zm9vYmE="},
	{"foobar", "Zm9vYmFy"}
};

bool base64Tests() {
	bool passedAll = true;

	std::cout << "=== Base64 Smoke Test ===" << std::endl;

	for (std::pair<std::string, std::string> testVector : base64TestVectors) {
		//if (testVector.first.size() % 3 != 0)
			//continue;

		std::cout << "\"" << testVector.first << "\"=\"" << testVector.second << "\": ";

		std::vector<Base64Digit> data(testVector.first.begin(), testVector.first.end());
		std::string base64String = generateBase64String(data);

		if (testVector.second == base64String) {
			std::cout << "PASS" << std::endl;
		} else {
			std::cout << "FAIL (\"" << base64String << "\")" << std::endl;
			passedAll = false;
		}
	}

	if (passedAll)
		std::cout << "=== PASS ===" << std::endl;
	else
		std::cout << "=== FAIL ===" << std::endl;

	return passedAll;
}

msclr::gcroot< msclr::interop::marshal_context ^ > msclr_context = gcnew msclr::interop::marshal_context();
#define MarshalCliString(cliString) msclr_context->marshal_as<const char *>(cliString)
#define ConvertCliStringToCXXString(cliString) std::string(MarshalCliString(cliString))

void OpenCAGELevelViewer::WebsocketThread::main() {
#ifndef NDEBUG
	assert(base64Tests());
#endif

	OpenCAGELevelViewer::WSockWrapper::WinSock wsa;

	while (keepThreadActive.test()) {
		connected.clear();
		ready.clear();
		wasConnected.clear();

		if (!willConnect.test()) {
			std::this_thread::yield();
			continue;
		}

		std::cout << "Connecting to " + commandsEditorCombined << std::endl;

		try {
		#pragma region Address Resolution
			OpenCAGELevelViewer::WSockWrapper::AddrInfo hints(AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP);

			OpenCAGELevelViewer::WSockWrapper::AddrInfo result(commandsEditorHost, std::stoi(commandsEditorPort), hints);
		#pragma endregion

		#pragma region Socket Creation
			OpenCAGELevelViewer::WSockWrapper::Socket ConnectSocket = OpenCAGELevelViewer::WSockWrapper::Socket(result.addrInfoPointer->ai_family, result.addrInfoPointer->ai_socktype, result.addrInfoPointer->ai_protocol);
		#pragma endregion

			ConnectSocket.set_nonBlocking(true);

		#pragma region Connection
			try {
				ConnectSocket._connect(result);
			} catch (OpenCAGELevelViewer::WSockWrapper::WSAError/*System::Runtime::InteropServices::SEHException ^*/e) {
				//__debugbreak();
				//std::cout << /*MarshalCliString(e->Message)*/"test" << std::endl;
				// WSAEWOULDBLOCK is expected from a non-blocking socket.
				if (e.code().value() != WSAEWOULDBLOCK) {
					std::cout << "\tReceived unexpected WinSock2 error: " << e.code().value() << std::endl;
					continue;
				}
			}

			//while (continueAttemptingToConnect) {
				//wsaResult = connect(ConnectSocket, result->ai_addr, static_cast< int >(result->ai_addrlen));
				//if (wsaResult == SOCKET_ERROR) {
				//	int wsaErrorCode = WSAGetLastError();
				//	if (wsaErrorCode != WSAEWOULDBLOCK && WSAGetLastError() != 0 /* ??? */) {
				//		std::cout << "WSA failure; " << WSAGetLastError() << " \"" << std::system_category().message(WSAGetLastError()) << "\"" << std::endl << std::flush;
				//		//#ifndef NDEBUG
				//		__debugbreak();
				//		//#endif
				//		continue;
				//		freeaddrinfo(result);
				//	} else {
				//		//__debugbreak();
				//	}
				//} else {
				//	__debugbreak();
				//}
			//}

			/*freeaddrinfo(result);

			fd_set writableSockets;
			FD_ZERO(&writableSockets);
			FD_SET(ConnectSocket, &writableSockets);

			fd_set errorSockets;
			FD_ZERO(&errorSockets);
			FD_SET(ConnectSocket, &errorSockets);

			TIMEVAL timeout {};
			timeout.tv_sec = 5;
			timeout.tv_usec = 0;

			wsaResult = select(0, nullptr, &writableSockets, nullptr, &timeout);

			if (wsaResult == SOCKET_ERROR) {
				__debugbreak();
			} else if (wsaResult == 0) {
				__debugbreak();
			} else {
				if (!FD_ISSET(ConnectSocket, &writableSockets)) {
					std::cout << "\tTimed out." << std::endl;
					continue;
				} else if (!FD_ISSET(ConnectSocket, &errorSockets)) {
					std::cout << "\tSocket error" << std::endl;
				}
			}*/

			OpenCAGELevelViewer::WSockWrapper::SocketStatusBitField status = ConnectSocket.select(10s);

			if (status & OpenCAGELevelViewer::WSockWrapper::SocketStatus::SocketStatus_Excepted) {
				try {
					int error = ConnectSocket.getSockOpt<int>(SOL_SOCKET, SO_ERROR);
					switch (error) {
						case WSAECONNREFUSED:
							std::cout << "\tConnection refused." << std::endl;
							break;
						default:
							std::cout << "\tUnaccounted; " << error << std::endl;
							break;
					}
					//std::cout << "\t"  << std::endl;
				} catch (OpenCAGELevelViewer::WSockWrapper::WSAError e) {
					std::cout << "double!" << std::endl;
				}
				continue;
			} else if (status & OpenCAGELevelViewer::WSockWrapper::SocketStatus::SocketStatus_Readable) {
				std::cout << "\tTimed out." << std::endl;
				continue;
			}

		#pragma endregion

			connected.test_and_set();

			std::pair<std::string, std::string> nonce = generateNoncePair();

			std::string testRequest =
				"GET " + commandsEditorPath + " HTTP/1.1\r\n"
				"Host: " + commandsEditorCombined + "\r\n"
				"Connection: upgrade\r\n"
				"Upgrade: websocket\r\n"
				"Sec-Websocket-Version: 13\r\n"
				"Sec-Websocket-Key: " + nonce.first + "\r\n"
				"\r\n\r\n";

			ConnectSocket.Send(std::vector<char>(testRequest.begin(), testRequest.end()));

			static thread_local bool didHttpHandshake = false;

			while ((!wasConnected.test() || connected.test()) && willConnect.test()) {
				OpenCAGELevelViewer::WSockWrapper::SocketStatusBitField status = ConnectSocket.select();

				if (status & OpenCAGELevelViewer::WSockWrapper::SocketStatus::SocketStatus_Readable) {
					std::vector<char> currentResponse = ConnectSocket.Recv();
					
					if (currentResponse.empty()) {
						__debugbreak();
					}

					if (!didHttpHandshake) {
						std::string responseString(currentResponse.begin(), currentResponse.end());
						std::cout << responseString << std::endl;
					}

					continue;
				}

				std::this_thread::yield();
			}

			if (connected.test()) {
				//ws.close();
			}

			//if (ws.is_open())
				//ws.close(websocket::close_code::going_away);
		} catch (...) {
			std::cout << "\terror..." << std::endl;
		}

	}

	WSACleanup();

	connected.clear();
	ready.clear();
	wasConnected.clear();
}

//   ix::initNetSystem();

   //ix::WebSocket webSocket;

   //webSocket.disablePerMessageDeflate();

   //webSocket.setUrl(commandsEditorUrl);

   //webSocket.setOnMessageCallback([](const ix::WebSocketMessagePtr &msg) {
   //	if (msg->type == ix::WebSocketMessageType::Message) {
   //		std::cout << "received message: " << msg->str << std::endl;
   //	} else if (msg->type == ix::WebSocketMessageType::Open) {
   //		std::cout << "Connection established" << std::endl;
   //	} else if (msg->type == ix::WebSocketMessageType::Error) {
   //		// Maybe SSL is not configured properly
   //		std::cout << msg->errorInfo.reason << std::endl;
   //		std::cout << commandsEditorUrl << std::endl;
   //	}
   //	}
   //);

   //webSocket.start();

   //while (_connect) {
   //	std::this_thread::yield();
   //}

   //webSocket.stop();
