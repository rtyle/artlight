#pragma once

#include "asio/ip/udp.hpp"

#include "AsioTask.h"
#include "KeyValueBroker.h"

class PeerTask: public AsioTask {
private:
    asio::ip::udp::socket		peer;
    asio::ip::udp::endpoint		sendEndpoint;
    KeyValueBroker &			keyValueBroker;
    KeyValueBroker::Observer		portObserver;
    KeyValueBroker::GeneralObserver	generalObserver;

    char				receiveMessage[128];
    asio::ip::udp::endpoint		receiveEndpoint;

    void receive();
public:

    PeerTask(
	KeyValueBroker &	keyValueBroker);
};
