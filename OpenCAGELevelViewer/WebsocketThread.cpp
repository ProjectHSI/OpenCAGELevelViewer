#include "WebsocketThread.h"

#include <iostream>
#include <mutex>
//#include <ixwebsocket/IXNetSystem.h>
//#include <ixwebsocket/IXWebSocket.h>
//#include <ixwebsocket/IXUserAgent.h>

//#include <fv/fv.h>
//#include <fv/declare.hpp>

//#include <websocketpp/config/asio_no_tls_client.hpp>
//#include <websocketpp/client.hpp>
//
//typedef websocketpp::client<websocketpp::config::asio_client> WebsocketPPClient;

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <nlohmann/json.hpp>

#include <ContentManager.h>

//#include <Level.h>

std::string commandsEditorHost = "localhost";
std::string commandsEditorPort = "1702";
std::string commandsEditorCombined = commandsEditorHost + ":" + commandsEditorPort;
std::string commandsEditorPath = "/commands_editor";

std::atomic_flag OpenCAGELevelViewer::WebsocketThread::connect;
std::atomic_flag OpenCAGELevelViewer::WebsocketThread::connected;
std::atomic_flag OpenCAGELevelViewer::WebsocketThread::ready;

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

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

// taken directly from OpenCAGE's Commands Editor
enum MessageType {
	SYNC_VERSION,

	LOAD_LEVEL,
	LOAD_COMPOSITE,

	GO_TO_POSITION,
	SHOW_ENTITY_NAME,
};

// ...
static void readThread(websocket::stream<tcp::socket> *ws) {
	try {
		beast::flat_buffer readBuffer;

		while (OpenCAGELevelViewer::WebsocketThread::connect.test()) {
			(*ws).read(readBuffer);

			std::cout << "\t" << beast::make_printable(readBuffer.data()) << std::endl;

			auto parsedJson = nlohmann::json::parse(beast::buffers_to_string(readBuffer.data()));



			MessageType messageType = static_cast<MessageType>(parsedJson["type"]);

			long long version;

			switch (messageType) {
				case SYNC_VERSION: // only version 
					if (parsedJson["version"] != 3) {
						OpenCAGELevelViewer::WebsocketThread::connect.clear();

						std::cout << "\tVersion mismatch." << std::endl;
					}

					OpenCAGELevelViewer::WebsocketThread::ready.test_and_set();

					break;
				
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
			}

			//long long version = parsedJson["version"].get<long long>();
			
			

			readBuffer.clear();
		}
	} catch (std::system_error e) {
		//if (e.code())

		__debugbreak();
	} catch (...) {
		__debugbreak();
	}
}

void OpenCAGELevelViewer::WebsocketThread::main() {
	/*std::shared_ptr<fv::WsConn> _conn = co_await fv::ConnectWS(commandsEditorUrl);
	while (_conn) {
		auto [_data, _type] = co_await _conn->Recv();
		std::cout << _data << std::endl;
	}
	fv::Tasks::Stop();*/

	while (keepThreadActive.test()) {
		connected.clear();
		ready.clear();

		if (!connect.test()) {
			std::this_thread::yield();
			continue;
		}
		
		std::cout << "Connecting to " + commandsEditorCombined << std::endl;

		net::io_context ioc;

		tcp::resolver resolver {ioc};
		websocket::stream<tcp::socket> ws {ioc};

		try {
			auto const resolvedEndpoints = resolver.resolve(commandsEditorHost, commandsEditorPort);

			std::cout << "\tResolved..." << std::endl;

			auto ep = net::connect(ws.next_layer(), resolvedEndpoints);

			connected.test_and_set();

			std::cout << "\tConnected." << std::endl;

			ws.handshake(commandsEditorCombined, commandsEditorPath);

			std::cout << "\tWebSocket connection successful!" << std::endl;

			/*ws.async_read(readBuffer, [readBuffer](const std::error_code &ec, std::size_t bytes_written) -> void {
				if (ec.value() != 0)
					__debugbreak();

				auto readBufferReadable = beast::make_printable(readBuffer.data());

				std::cout << readBufferReadable << std::endl;

				__debugbreak();
						  });*/

			//ws.async_read(readBuffer, [readBuffer](std::error_code, std::size_t) {
			
			//	__debugbreak();
			//});

			auto wsPointer = &ws;
			std::thread websocketReadThread([wsPointer]() { readThread(wsPointer); });

			while (ws.is_open() && connect.test()) {
				std::this_thread::yield();
			}

			if (ws.is_open())
				ws.close(websocket::close_code::going_away);
		} catch (...) {
			std::cout << "\terror..." << std::endl;
		}

	}

	connected.clear();
	ready.clear();
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