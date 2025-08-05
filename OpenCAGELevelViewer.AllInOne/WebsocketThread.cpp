#include "WebsocketThread.h"

#include "WSockWrapper.h"
#include <array>
#include <atomic>
#include <bit>
#include <bitset>
#include <chrono>
#include <cryptopp/sha.h>
#include <cstdlib>
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
#include <intrin.h>
//#include <boost/multiprecision/tommath.hpp>

#undef min
#undef max

#pragma comment(lib, "Ws2_32.lib")

//#include <ContentManager.h>

//#include <Level.h>

using namespace std::chrono_literals;

#pragma managed (push, off)

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

		case LEVEL_LOADED:

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

constexpr std::string generateBase64String(const std::vector<Base64Digit> &data) {
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

CryptoPP::SHA1 sha1Instance;

constexpr static std::array< CryptoPP::byte, CryptoPP::SHA1::DIGESTSIZE > sha1(const std::vector<char> &data) {
	std::array< CryptoPP::byte, CryptoPP::SHA1::DIGESTSIZE > digest;
	sha1Instance.Update(reinterpret_cast< const CryptoPP::byte * >(data.data()), data.size());
	sha1Instance.Final(digest.data());

	return digest;
	//data.push_back(0b10000000); // append 1 bit
	//__debugbreak();
}

struct NoncePair {
	std::string key;
	std::string accept;
};
//typedef boost::multiprecision::uint128_t NonceNumber;
constexpr size_t NonceBytes = 16;
typedef std::random_device NonceDevice;
typedef NonceDevice::result_type NonceDeviceResult;
typedef NonceDeviceResult NonceDeviceResultCast; // enable if the static assertion belows suceeds with this value. atm it doesn't
												// generally you want to use this when you can, but we can't. so...
//typedef unsigned short NonceDeviceResultCast;
NonceDevice nonceDevice;

static std::string generateNonceComplement(const std::string &nonce) {
	std::string secondNoncePair = nonce + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	//std::string secondNoncePairBase64 = generateBase64String(std::vector<Base64Digit>(secondNoncePair.begin(), secondNoncePair.end()));

	auto sha1Hash = sha1(std::vector<char>(secondNoncePair.begin(), secondNoncePair.end()));
	std::string generated = generateBase64String(std::vector<Base64Digit>(sha1Hash.begin(), sha1Hash.end()));

	return generated;
}

NoncePair generateNoncePair() {
	assert(generateNonceComplement("dGhlIHNhbXBsZSBub25jZQ==") == "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=");

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
	noncePair.key = generateBase64String(buffer);
	noncePair.accept = generateNonceComplement(noncePair.key);

	return noncePair;
}


WSADATA wsaData {};

constexpr bool base64Tests() {
	std::vector<std::pair<std::string, std::string>> base64TestVectors = {
	{"", ""},
	{"f", "Zg=="},
	{"fo", "Zm8="},
	{"foo", "Zm9v"},
	{"foob", "Zm9vYg=="},
	{"fooba", "Zm9vYmE="},
	{"foobar", "Zm9vYmFy"}
	};

	bool passedAll = true;

	/*std::cout << "=== Base64 Smoke Test ===" << std::endl;*/

	for (std::pair<std::string, std::string> testVector : base64TestVectors) {
		//if (testVector.first.size() % 3 != 0)
			//continue;

		/*std::cout << "\"" << testVector.first << "\"=\"" << testVector.second << "\": ";*/

		std::vector<Base64Digit> data(testVector.first.begin(), testVector.first.end());
		std::string base64String = generateBase64String(data);

		if (testVector.second == base64String) {
			/*std::cout << "PASS" << std::endl;*/
		} else {
			/*std::cout << "FAIL (\"" << base64String << "\")" << std::endl;*/
			passedAll = false;
		}
	}

	/*if (passedAll)
		std::cout << "=== PASS ===" << std::endl;
	else
		std::cout << "=== FAIL ===" << std::endl;*/

	//assert(passedAll);

	return passedAll;
}
constexpr bool passedBase64Tests = base64Tests();

static_assert(passedBase64Tests == true, "Base64 Test Vectors did not match expected values");

msclr::gcroot< msclr::interop::marshal_context ^ > msclr_context = gcnew msclr::interop::marshal_context();
#define MarshalCliString(cliString) msclr_context->marshal_as<const char *>(cliString)
#define ConvertCliStringToCXXString(cliString) std::string(MarshalCliString(cliString))

static std::vector<std::string> split(std::string s, const std::string &delimiter) {
	std::vector<std::string> tokens;
	size_t pos = 0;
	std::string token;
	while ((pos = s.find(delimiter)) != std::string::npos) {
		token = s.substr(0, pos);
		tokens.push_back(token);
		s.erase(0, pos + delimiter.length());
	}
	tokens.push_back(s);

	return tokens;
}

static std::vector<char> maskData(std::vector<char> data, const uint32_t maskKey) {
	std::array<uint8_t, 4> maskKeyArray;
	
	for (size_t i = 0; i < sizeof(maskKey); i++) {
		maskKeyArray[i] = maskKey >> 24 - (i * 8);
	}

	for (size_t i = 0; i < data.size(); i++) {
		data[i] ^= maskKeyArray[i % 4];
	}

	return data;
}

struct MaskedDataCombo {
	std::vector<char> maskedData;
	uint32_t key;
};

static MaskedDataCombo maskData(const std::vector<char> &data) {
	MaskedDataCombo maskedDataCombo;

	maskedDataCombo.key = (((static_cast< uint32_t >(nonceDevice()) << 16)) | nonceDevice());
	maskedDataCombo.maskedData = maskData(data, maskedDataCombo.key);

	return maskedDataCombo;
}

enum BufferType {
	FrameType_Undefined,
	FrameType_Text,
	FrameType_Binary
};

static std::vector<char> buffer;
static std::variant < std::vector<char>, std::string > outputBuffer;
static BufferType bufferType = FrameType_Undefined;

static void finaliseBuffer() {
	if (bufferType == FrameType_Text) {
		outputBuffer = std::string(buffer.begin(), buffer.end());
		//std::cout << std::get<std::string>(outputBuffer) << std::endl;
		handleMessage(std::get<std::string>(outputBuffer));
	} else if (bufferType == FrameType_Binary) {
		outputBuffer = std::vector<char>(buffer.begin(), buffer.end());
	} else {
		std::cout << "Invalid buffer type." << std::endl;
	}
	buffer.clear();
	bufferType = FrameType_Undefined;
}

#define ALLOW_SERVER_MASKED_FRAMES 0

enum WebSocketFrameType {
	CONT,
	DATA,
	CTRL
};

class WebSocketFrame {
public:
	virtual uint8_t getOpCode() const = 0;
	virtual std::vector<char> getData() const = 0;
	virtual WebSocketFrameType getDataType() const = 0;
	virtual bool isFinal() const = 0;
};

class WebSocketPongFrame : public WebSocketFrame {
private:
	std::vector<char> pongData {};
public:
	WebSocketPongFrame(const std::vector<char> &data) : pongData(data) { }

	virtual uint8_t getOpCode() const {
		return 0xA;
	}

	virtual std::vector<char> getData() const {
		return pongData;
	}

	virtual WebSocketFrameType getDataType() const {
		return WebSocketFrameType::CTRL;
	}

	virtual bool isFinal() const {
		return true; // Control frames are always final.
	}
};

std::vector<char> transmitBuffer;

static constexpr std::vector<char> generateWebsocketMessage(const WebSocketFrame &data, bool mask = true) {
	assert(data.getOpCode() <= 0xF);

	std::vector<char> messageBuffer;

	messageBuffer.push_back(data.getOpCode());

	if (data.isFinal())
		messageBuffer[0] |= 0b10000000;

	std::vector<char> dataBuffer = data.getData();

	// % 0x7F because of 7-bit limitation
	messageBuffer.push_back(static_cast<uint8_t>(dataBuffer.size() % 0x7F));

	int payloadLengthExtension = 0;
	if (dataBuffer.size() >= 0x7E) {
		messageBuffer[1] = 0x7E;
		payloadLengthExtension = 1;
		if (dataBuffer.size() <= std::numeric_limits<uint16_t>::max()) {
			messageBuffer[1] = 0x7F;
			payloadLengthExtension = 2;
		}
	}

	switch (payloadLengthExtension) {
		case 0:
			// nothing
			break;
		case 1:
			// 2-byte length
			messageBuffer.push_back(static_cast<char>(dataBuffer.size() >> 8));
			messageBuffer.push_back(static_cast<char>(dataBuffer.size() & 0xFF));
			break;
		case 2:
			// 8-byte length
			for (size_t i = 0; i < 8; i++) {
				messageBuffer.push_back(static_cast<char>((dataBuffer.size() >> (56 - (8 * i))) & 0xFF));
			}
			break;
	}

	if (mask) {
		messageBuffer[1] |= 0b10000000; // set mask bit
		
		MaskedDataCombo maskedData = maskData(dataBuffer);

		dataBuffer = maskedData.maskedData;
		
		static_assert(sizeof(decltype(maskedData.key)) == 4);
		[[assume(sizeof(decltype(maskedData.key)) == 4)]];

		for (size_t i = 0; i < 4; i++) {
			messageBuffer.push_back(static_cast< char >((maskedData.key >> (24 - (8 * i))) & 0xFF));
		}
	}

	messageBuffer.insert(messageBuffer.end(), dataBuffer.begin(), dataBuffer.end());

	return messageBuffer;
}

static void processWebsocketMessage(std::vector<char> &data) {
	assert(data.size() != 0);
	[[assume(data.size() == 0)]]; // where this is called will make it so that data.size() is always higher than 0.
	if (data.size() < 2) {
		std::cout << "invalid frame" << std::endl;
	}

	bool isFinalFrame = data[0] & 0b10000000;

	uint8_t opcode = data[0] & 0b00001111;
	bool isControlFrame = (opcode >> 3 & 0b1);

	// RFC doesn't require any masking from Server -> Client, but I don't think ever says whether or not Server -> Client communications can be masked.
	bool isMasked = data[1] & 0b10000000;

#if !(ALLOW_SERVER_MASKED_FRAMES)
	if (isMasked) {
		__debugbreak();
	}
#endif

	uint8_t originalPayloadLength = data[1] & 0b01111111;
	uint64_t payloadLength = originalPayloadLength;

	uint8_t headerSize = 2;

	switch (originalPayloadLength) {
		case 127:
			headerSize += 8;
			if (data.size() < headerSize) {
				std::cout << "\tinvalid frame" << std::endl;
				__debugbreak();
			}
			payloadLength = 0;
			for (size_t i = 0; i < 8; i++) {
				payloadLength |= (static_cast< uint64_t >(data[i + 2]) << (56 - (8 * i)));
			}
			break;
		case 126:
			headerSize += 2;
			if (data.size() < headerSize) {
				std::cout << "\tinvalid frame" << std::endl;
				__debugbreak();
			}
			payloadLength = 0;
			for (size_t i = 0; i < 2; i++) {
				std::bitset<16> debugging(static_cast< uint16_t >(data[i + 2]) & 0xFF);
				std::cout << "\tframe payload length byte " << i + 1 << ": " << debugging << " (" << static_cast< uint16_t >(data[i + 2]) << ")" << std::endl;
				std::cout << "\tshift: " << (8 - (8 * i)) << std::endl;
				debugging <<= (8 - (8 * i));
				std::cout << "\tshifted: " << debugging << "(" << debugging.to_ullong() << ")" << std::endl;

				//static_cast< uint16_t >(data[i + 2])
				payloadLength |= ((static_cast< uint16_t >(data[i + 2]) & 0xFF) << (8 - (8 * i)));
			}
			break;
	}

#if (ALLOW_SERVER_MASKED_FRAMES)
	headerSize += 4;
	if (data.size() < headerSize) {
		std::cout << "\tinvalid frame" << std::endl;
		__debugbreak();
	}

	uint32_t maskKey = 0;
	for (size_t i = 0; i < 4; i++) {
		maskLength |= (data[i + headerSize] << 32 - i);
	}
#endif

	if (data.size() < headerSize + payloadLength) {
		std::cout << "\tinvalid frame: invalid size (" << headerSize + payloadLength << ") vs. " << data.size() << std::endl;
		__debugbreak();
	}
	std::vector<char> payload(data.begin() + headerSize, data.begin() + headerSize + payloadLength);

#if (ALLOW_SERVER_MASKED_FRAMES)
	payload = maskData(payload, maskKey);
#endif

	if (!isControlFrame) {
		switch (opcode) {
			case 0x0: // continuation
				if (bufferType == FrameType_Undefined) {
					std::cout << "\tReceived continuation frame, but no previous frame to continue." << std::endl;
				}
				buffer.insert(buffer.end(), payload.begin(), payload.end());
				if (isFinalFrame)
					finaliseBuffer();
				break;
			case 0x1: // text
				bufferType = FrameType_Text;
				buffer = payload;
				if (isFinalFrame)
					finaliseBuffer();
				break;
			case 0x2: // binary
				bufferType = FrameType_Binary;
				buffer = payload;
				if (isFinalFrame)
					finaliseBuffer();
				break;
			default:
				__debugbreak();
		}
	} else {
		switch (opcode) {
			case 0x8:
				{
					std::array<char, 2> closeCode = {payload[0], payload[1]};

					static_assert(sizeof(std::array<char, 2>) == sizeof(uint16_t));

					uint16_t closeCodeValue = std::bit_cast< uint16_t, std::array<char, 2> >(closeCode);;
					if constexpr (std::endian::native == std::endian::little)
						closeCodeValue = _byteswap_ushort(closeCodeValue); // TODO: Swap this for std::byteswap when C++/CLI supports C++23 ... yeah. MSVC itself supports C++23, but just not C++/CLI.

					std::cout << "\tReceived close code " << closeCodeValue << "!";
				}

				__debugbreak();
				break;
			case 0x9:
				//std::cout << std::string(payload.begin(), payload.end()) << std::endl;
				//__debugbreak();
				{
					auto pongFrame = generateWebsocketMessage(WebSocketPongFrame(payload), true);
					transmitBuffer.insert(transmitBuffer.end(), pongFrame.begin(), pongFrame.end());
				}
				break;
			case 0xA:
				// intentionally empty, no need to respond.
				break;
			default:
				__debugbreak();
		}
	}

	data.erase(data.begin(), data.begin() + headerSize + payloadLength);
}

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
			{
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
			}

		#pragma endregion

			connected.test_and_set();

			NoncePair nonce = generateNoncePair();

			std::string testRequest =
				"GET " + commandsEditorPath + " HTTP/1.1\r\n"
				"Host: " + commandsEditorCombined + "\r\n"
				"Connection: upgrade\r\n"
				"Upgrade: websocket\r\n"
				"Sec-Websocket-Version: 13\r\n"
				"Sec-Websocket-Key: " + nonce.key + "\r\n"
				"\r\n";

			ConnectSocket.Send(std::vector<char>(testRequest.begin(), testRequest.end()));

			static bool wasError = false;

			while ((!wasConnected.test() || connected.test()) && (willConnect.test() && !wasError)) {
				OpenCAGELevelViewer::WSockWrapper::SocketStatusBitField status = ConnectSocket.select(100ms);

				if (status & OpenCAGELevelViewer::WSockWrapper::SocketStatus::SocketStatus_Readable) {
					std::vector<char> currentResponse = ConnectSocket.Recv();
					
					if (currentResponse.empty()) {
						continue;
					}

					static bool didHttpHandshake = false;

					if (!didHttpHandshake) {
						// This isn't technically RFC compliant.
						// We can get away with this because Websocket-Sharp is deterministic.

						std::string responseString(currentResponse.begin(), currentResponse.end());
						//std::cout << responseString << std::endl;

						auto preMainSplit = split(responseString, "\r\n\r\n");
						auto mainSplit = split(preMainSplit[0], "\r\n");

						std::cout << preMainSplit[0] << std::endl;

						if (mainSplit[0] != "HTTP/1.1 101 Switching Protocols")
							continue;
						
						size_t conditionsSatisfied = 0;

						for (size_t i = 1; i < mainSplit.size(); i++) {
							std::vector<std::string> header = split(mainSplit[i], ":");
							std::string headerName = header[0];
							std::string headerValue = header[1].substr(1);

							if (headerName == "Connection") {
								if (headerValue == "Upgrade") { 
									conditionsSatisfied++;
								}
							} else if (headerName == "Upgrade") {
								if (headerValue == "websocket") {
									conditionsSatisfied++;
								}
							} else if (headerName == "Sec-WebSocket-Accept") { 
								if (headerValue == nonce.accept) {
									conditionsSatisfied++;
								}
							} else if (headerName == "Server") {
								if (headerValue == "websocket-sharp/1.0") {
									conditionsSatisfied++;
								}
							}
						}

						if (conditionsSatisfied == 4) {
							didHttpHandshake = true;
						} else {
							continue;
						}

						currentResponse.erase(currentResponse.begin(), currentResponse.begin() + preMainSplit[0].size() + 4); // 4 for the \r\n\r\n at the end.
						ready.test_and_set();
					}

					while (currentResponse.size() > 0) {
						processWebsocketMessage(currentResponse);
					}
				}

				static bool didSendPongFrame = false;

				if (status & WSockWrapper::SocketStatus::SocketStatus_Writable) {
					if (!didSendPongFrame) {
						std::vector<char> pongFrameData = generateWebsocketMessage(WebSocketPongFrame(std::vector<char> {}));
						transmitBuffer.insert(transmitBuffer.end(), pongFrameData.begin(), pongFrameData.end());
						didSendPongFrame = true;
					}
					ConnectSocket.Send(transmitBuffer);
				}

				if (status & WSockWrapper::SocketStatus_Excepted) {
					__debugbreak();
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

#pragma managed(pop)