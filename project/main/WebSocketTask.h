#pragma once

#include <set>

#include "AsioTask.h"
#include "KeyValueBroker.h"

class WebSocketTask : public AsioTask {
private:
    asio::ip::tcp::acceptor	acceptor;
    KeyValueBroker::GeneralObserver	generalObserver;

    enum CloseStatus: uint16_t {
	normal			= 1000,
	goingAway		= 1001,
	protocolError		= 1002,
	dataError		= 1003,
	consistencyError	= 1007,
	policyError		= 1008,
	sizeError		= 1009,
	extensionError		= 1010,
	unexpectedError		= 1011,
    };

    enum OpCode: uint8_t {
	continuation	= 0x0,
	text		= 0x1,
	binary		= 0x2,
	close		= 0x8,
	ping		= 0x9,
	pong		= 0xa,
    };

    struct Frame {
    public:
	uint8_t	opCode		:4;
	uint8_t	reserved	:3;
	uint8_t	fin		:1;
	uint8_t	length		:7;
	uint8_t	mask		:1;
	Frame();
	Frame(size_t length, OpCode opCode, bool fin, bool mask = false);
    };

    void acceptSession();

    class Session : public std::enable_shared_from_this<Session> {
	friend WebSocketTask;
    private:
	WebSocketTask &		webSocketTask;
	asio::ip::tcp::socket	socket;

	asio::streambuf		streambuf;

	class Unmask {
	private:
	    size_t const	length;
	    uint8_t const	(&mask)[4];
	    size_t index;
	public:
	    Unmask(size_t length_, uint8_t (& mask_)[4]);
	    int operator()(std::streambuf & source);
	};

	void takeHold();

	void dropHold();

	void send(
	    void const *	message,
	    size_t		length,
	    OpCode		opCode	= text,
	    bool		fin	= true);

	// process message and return true if close was processed
	bool process(
	    Frame const &	frame,
	    size_t		length,
	    std::streambuf &&	payload);

	// process buffered messages and return true if close was processed.
	bool processBuffered();

	void receive();

	void handshake();

    public:

	Session(
	    WebSocketTask &		webSocketTask,
	    asio::ip::tcp::socket &&	socket_);

	~Session();
    };
    std::set<std::shared_ptr<Session>> heldSessions;

    void spray(
	void const *	message,
	size_t		length,
	OpCode		opCode	= text,
	bool		fin	= true);

public:

    WebSocketTask(KeyValueBroker &);

    ~WebSocketTask();
};
