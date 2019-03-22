#include <algorithm>
#include <cstring>

#include <esp_log.h>

#include "fromString.h"
#include "PeerTask.h"

void PeerTask::receive() {
    peer.async_receive_from(
	asio::buffer(receiveMessage, sizeof receiveMessage - 1),
	receiveEndpoint,	// not modified!?!
	[this](std::error_code error, std::size_t length){
	    ESP_LOGI(name, "receive error %d length %u key %s value %s port %d %d",
		error.value(), length,
		receiveMessage,
		receiveMessage + strlen(receiveMessage) + 1,
		sendEndpoint.port(),
		receiveEndpoint.port());
	    if (!error.value()
		    && 2 < length
		    && sizeof receiveMessage > length) {
		receiveMessage[length] = 0;
		char const * key = receiveMessage;
		size_t keySize = strlen(key) + 1;
		char const * value = key + keySize;
		if (length == keySize + strlen(value) + 1) {
		    keyValueBroker.remotePublish(key, value);
		}
	    }
	    receive();
	});
}

PeerTask::PeerTask(
    KeyValueBroker &		keyValueBroker_)
:
    AsioTask		("peerTask", 5, 4096, 0),

    peer		(io, asio::ip::udp::endpoint(asio::ip::udp::v4(), 0)),
    sendEndpoint	(asio::ip::address_v4(IPADDR_BROADCAST), 0),

    keyValueBroker	(keyValueBroker_),

    portObserver	(keyValueBroker, "_port", "16180",
	[this](char const * value){
	    static unsigned short constexpr min = 1023;		// privileged max
	    static unsigned short constexpr max = 0xc000;	// ephemeral min
	    unsigned short port = fromString<unsigned short>(value);
	    if (min < port && port < max) {
		io.post([this, port](){
		    ESP_LOGI(name, "port %d", static_cast<int>(port));
		    peer.bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), port));
		    sendEndpoint.port(port);
		});
	    }
	}),

    remoteObserver	(keyValueBroker,
	[this](char const * key, char const * value) {
	    if ('_' != *key) {
		size_t keySize = strlen(key) + 1;
		size_t messageSize = keySize + strlen(value) + 1;
		char * message = new char[messageSize];
		std::strcpy(message, key);
		std::strcpy(message + keySize, value);
		io.post([this, message, messageSize](){
		    peer.async_send_to(asio::buffer(message, messageSize),
			sendEndpoint,
			[this, message](std::error_code, std::size_t){
			    ESP_LOGI(name, "sent %s %s",
				message, message + strlen(message) + 1);
			    delete message;
			});
		});
	    }
	})
{
    receive();
}
