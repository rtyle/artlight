#include <algorithm>
#include <cstring>

#include "esp_log.h"

#include "fromString.h"
#include "PeerTask.h"

void PeerTask::receive() {
    peer.async_receive_from(
	asio::buffer(receiveMessage, sizeof receiveMessage - 1),
	// receiveEndpoint will not be returned because of a lwip_recvmsg bug
	// https://savannah.nongnu.org/bugs/index.php?55987
	receiveEndpoint,
	[this](std::error_code error, std::size_t length){
	    if (error) {
		ESP_LOGE(name, "receive error: %s", error.message().c_str());
		if (asio::error::not_connected == error) return;
	    } else if (sendEndpoint.port()) {
		if (!(2 < length && sizeof receiveMessage > length)) {
		    ESP_LOGE(name, "receive bad length");
		} else {
		    receiveMessage[length] = 0;
		    char const * key = receiveMessage;
		    size_t keySize = strlen(key) + 1;
		    char const * value = key + keySize;
		    if (length != keySize + strlen(value) + 1) {
			ESP_LOGE(name, "receive bad message");
		    } else {
			ESP_LOGI(name, "receive %s %s", key, value);
			keyValueBroker.publish(key, value, true);
		    }
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
	    static unsigned short constexpr min = 1023;	  // privileged max
	    static unsigned short constexpr max = 0xc000; // ephemeral min
	    unsigned short port = fromString<unsigned short>(value);
	    if (0 == port || (min < port && port < max)) {
		io.post([this, port](){
		    ESP_LOGI(name, "port %d", static_cast<int>(port));
		    peer.bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), port));
		    sendEndpoint.port(port);
		});
	    }
	}),

    generalObserver	(keyValueBroker,
	[this](char const * key, char const * value, bool fromPeer) {
	    // keys that start with an underscore are not for our peers
	    if (!fromPeer && '_' != *key && sendEndpoint.port()) {
		size_t keySize = strlen(key) + 1;
		size_t messageSize = keySize + strlen(value) + 1;
		char * message = new char[messageSize];
		std::strcpy(message, key);
		std::strcpy(message + keySize, value);
		io.post([this, message, messageSize](){
		    peer.async_send_to(asio::buffer(message, messageSize),
			sendEndpoint,
			[this, message](std::error_code error, std::size_t){
			    if (error) {
				ESP_LOGE(name, "send error: %s",
				    error.message().c_str());
			    } else {
				ESP_LOGI(name, "send %s %s",
				    message, message + strlen(message) + 1);
			    }
			    delete message;
			});
		});
	    }
	}),

    receiveMessage {},
    receiveEndpoint()
{
    receive();
}
