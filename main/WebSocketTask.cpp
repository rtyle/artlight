#include <memory>
#include <streambuf>

#include <esp_log.h>
#include <hwcrypto/sha.h>
#include <mbedtls/base64.h>

#include "Filter.h"
#include "WebSocketTask.h"

WebSocketTask::Frame::Frame()
:
    opCode	(text),
    reserved	(0),
    fin		(true),
    length	(0),
    mask	(false)
{}

WebSocketTask::Frame::Frame(
    size_t	length_,
    OpCode	opCode_,
    bool	fin_,
    bool	mask_)
:
    opCode	(opCode_),
    reserved	(0),
    fin		(fin_),
    length	(length_),
    mask	(mask_)
{
    assert(126 > length);
}

void WebSocketTask::acceptSession() {
    ESP_LOGI(name, "socket %d acceptSession", acceptor.native_handle());
    acceptor.async_accept([this](
	std::error_code			ec,
	asio::ip::tcp::socket &&	socket)
    {
	if (ec) {
	    ESP_LOGE(name, "socket %d accept error: %d (%s)",
		acceptor.native_handle(), ec.value(), ec.message().c_str());
	} else {
	    ESP_LOGI(name, "socket %d accepted", socket.native_handle());
	    // create a new Session and hold on to it while it is useful
	    std::make_shared<Session>(
		*this, std::forward<asio::ip::tcp::socket>(socket)
	    )->handshake();
	}
	acceptSession();
    });
}

WebSocketTask::Session::Unmask::Unmask(
    size_t	length_,
    uint8_t (&	mask_)[4])
:
    length	(length_),
    mask	(mask_),
    index	(0)
{}

int WebSocketTask::Session::Unmask::operator()(std::streambuf & source) {
    if (index < length) {
	int result(source.sbumpc());
	if (std::streambuf::traits_type::eof() == result) return result;
	return result ^ mask[index++ % sizeof mask];
    } else {
	return std::streambuf::traits_type::eof();
    }
}

void WebSocketTask::Session::takeHold() {
    webSocketTask.heldSessions.insert(shared_from_this());
}

void WebSocketTask::Session::dropHold() {
    auto it = webSocketTask.heldSessions.find(shared_from_this());
    if (it != webSocketTask.heldSessions.end()) {
	webSocketTask.heldSessions.erase(it);
    }
}

void WebSocketTask::Session::send(
    void const *	message,
    size_t		length,
    OpCode		opCode,
    bool		fin)
{
    auto streambuf = std::make_shared<asio::streambuf>();
    std::ostream ostream(streambuf.get());
    Frame frame(length, opCode, fin);
    ostream.write(reinterpret_cast<char const *>(&frame), sizeof frame);
    ostream.write(static_cast<char const *>(message), length);
    auto hold(shared_from_this());
    asio::async_write(socket, *streambuf.get(),
	[this, hold, streambuf](
	    std::error_code	ec,
	    std::size_t		length)
	{
	    if (ec) {
		ESP_LOGE(webSocketTask.name, "socket %d write error: %d (%s)",
		    socket.native_handle(), ec.value(), ec.message().c_str());
		dropHold();
	    }
	}
    );
}

// process message and return true if communication should continue
bool WebSocketTask::Session::process(
    Frame const &	frame,
    size_t		length,
    std::streambuf &&	payload)
{
    // RFC 6455: All control frames ... MUST NOT be fragmented
    switch (frame.opCode) {
    case close: {
	    if (2 > length) {
		ESP_LOGI(webSocketTask.name, "socket %d send close",
		    socket.native_handle());
		send(nullptr, 0, close);
	    } else {
		uint16_t status;
		std::istream istream(&payload);
		istream.read(reinterpret_cast<char *>(&status),
		    sizeof status);
		ESP_LOGI(webSocketTask.name, "socket %d send close %d",
		    socket.native_handle(), ntohs(status));
		send(&status, sizeof status, close);
	    }
	}
	break;
    case ping:
	ESP_LOGI(webSocketTask.name, "socket %d send pong",
	    socket.native_handle());
	if (!length) {
	    send(nullptr, 0, pong);
	} else {
	    auto message = std::make_shared<char>(length);
	    std::istream istream(&payload);
	    istream.read(message.get(), length);
	    send(message.get(), length, pong);
	}
	return true;
    default: {
	    ESP_LOGE(webSocketTask.name, "socket %d send close protocol error",
		socket.native_handle());
	    uint16_t status = htons(CloseStatus::protocolError);
	    send(&status, sizeof status, close);
	}
	break;
    }
    dropHold();
    return false;
}

// process buffered input and return true if communication should continue
bool WebSocketTask::Session::processBuffered() {
    for (;;) {
	size_t frameLength = 0;
	size_t available = streambuf.size();

	Frame frame;
	if (sizeof frame > available) {
	    break;
	}
	auto source = streambuf.data();
	asio::buffer_copy(asio::buffer(&frame, sizeof frame), source);
	frameLength	+= sizeof frame;
	source		+= sizeof frame;
	available	-= sizeof frame;

	size_t payloadLength;
	if (126 > frame.length) {
	    payloadLength = frame.length;
	} else {
	    ESP_LOGE(webSocketTask.name, "socket %d send close size error",
		socket.native_handle());
	    uint16_t status = htons(CloseStatus::sizeError);
	    send(&status, sizeof status, close);
	    dropHold();
	    return false;
	}

	uint8_t mask[4] {};
	if (frame.mask) {
	    if (sizeof mask > available) {
		break;
	    }
	    asio::buffer_copy(asio::buffer(mask, sizeof mask), source);
	    frameLength	+= sizeof mask;
	    source	+= sizeof mask;
	    available	-= sizeof mask;
	}

	if (payloadLength > available) {
	    break;
	}
	streambuf.consume(frameLength);

	bool close = process(frame, payloadLength,
	    InputFilter<Unmask>(streambuf, Unmask(payloadLength, mask)));
	streambuf.consume(payloadLength);
	if (close) {
	    return false;
	}
    }
    return true;
}

void WebSocketTask::Session::receive() {
    if (!processBuffered()) return;
    auto hold(shared_from_this());
    asio::async_read(socket, streambuf,
	[this](
	    std::error_code	ec,
	    size_t		length)
	->size_t
	{
	    return ec ? 0 : length ? 0 : sizeof(Frame) + 4 + 125;
	},
	[this, hold](
	    std::error_code	ec,
	    size_t		length)
	{
	    if (ec) {
		ESP_LOGE(webSocketTask.name,
		    "socket %d receive read error: %d (%s)",
		    socket.native_handle(), ec.value(), ec.message().c_str());
		dropHold();
	    } else {
		receive();
	    }
	}
    );
}

void WebSocketTask::Session::handshake() {
    ESP_LOGI(webSocketTask.name, "socket %d handshake", socket.native_handle());
    static char constexpr eom[] = "\r\n\r\n";
    auto hold(shared_from_this());
    asio::async_read_until(socket, streambuf, eom,
	[this, hold](
	    std::error_code	ec,
	    std::size_t		length)
	{
	    if (ec) {
		ESP_LOGE(webSocketTask.name,
		    "socket %d handshake read error: %d (%s)",
		    socket.native_handle(), ec.value(), ec.message().c_str());
	    } else {
		std::string headers{
		    buffers_begin(streambuf.data()),
		    buffers_begin(streambuf.data()) + length
		      - (sizeof eom - 1)};
		streambuf.consume(length);

		static char constexpr header[] = "\r\nSec-WebSocket-Key:";
		std::string::size_type index = headers.find(header);
		if (index == std::string::npos) {
		    ESP_LOGE(webSocketTask.name,
			"socket %d handshake key missing",
			socket.native_handle());
		} else {
		    index += sizeof header - 1;
		    while (std::isspace(headers[index])) ++index;
		    std::string key(headers.substr(index, 24));
		    ESP_LOGI(webSocketTask.name,
			"socket %d handshake key %s",
			socket.native_handle(), key.c_str());

		    std::vector<asio::const_buffer> response;

		    static char constexpr som[] =
			"HTTP/1.1 101 Switching Protocols\r\n"
			"Upgrade: websocket\r\n"
			"Connection: Upgrade\r\n"
			"Sec-WebSocket-Accept: ";
		    response.push_back(asio::buffer(
			som, sizeof som - 1));

		    std::string text(key
			+ "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
		    unsigned char sha1[20];
		    esp_sha(SHA1,
			reinterpret_cast<unsigned char const *>(text.c_str()),
			text.size(), sha1);
		    size_t size;	// includes null terminator
		    mbedtls_base64_encode(nullptr, 0, &size,
			sha1, sizeof sha1);
		    std::shared_ptr<unsigned char> base64(new unsigned char[size]);
		    size_t length;	// excludes null terminator
		    mbedtls_base64_encode(base64.get(), size, &length,
			sha1, sizeof sha1);
		    response.push_back(asio::buffer(base64.get(), length));

		    response.push_back(asio::buffer(eom, sizeof eom - 1));

		    ESP_LOGI(webSocketTask.name,
			"socket %d handshake response %s",
			socket.native_handle(), base64.get());
		    socket.async_send(response,
			[this, hold, base64](
			    std::error_code	ec,
			    std::size_t		length)
			{
			    if (ec) {
				ESP_LOGE(webSocketTask.name,
				    "socket %d handshake write error: %d (%s)",
				    socket.native_handle(),
				    ec.value(), ec.message().c_str());
			    } else {
				takeHold();
				receive();
			    }
			}
		    );
		}
	    }
	}
    );
}

WebSocketTask::Session::Session(
    WebSocketTask &		webSocketTask_,
    asio::ip::tcp::socket &&	socket_)
:
    webSocketTask(webSocketTask_),
    socket(std::move(socket_))
{
    ESP_LOGI(webSocketTask.name, "socket %d Session::Session",
	socket.native_handle());
}

WebSocketTask::Session::~Session() {
    ESP_LOGI(webSocketTask.name, "socket %d Session::~Session",
	socket.native_handle());
}

void WebSocketTask::spray(
    void const *	message,
    size_t		length,
    OpCode		opCode,
    bool		fin)
{
    if (!heldSessions.size()) return;
    auto streambuf = std::make_shared<asio::streambuf>();
    std::ostream ostream(streambuf.get());
    Frame frame(length, opCode, fin);
    ostream.write(reinterpret_cast<char const *>(&frame), sizeof frame);
    ostream.write(static_cast<char const *>(message), length);
    for (auto & session: heldSessions) {
	asio::async_write(session->socket, *streambuf.get(),
	    [this, streambuf, session](
		std::error_code	ec,
		std::size_t	length)
	    {
		if (ec) {
		    ESP_LOGE(name, "socket %d write error: %d (%s)",
			session->socket.native_handle(),
			ec.value(), ec.message().c_str());
		    io.post([session](){session->dropHold();});
		}
	    }
	);
    }
}

WebSocketTask::WebSocketTask(KeyValueBroker & keyValueBroker)
:
    AsioTask("webSocketTask", 5, 4096, 0),
    acceptor(io, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 81)),
    generalObserver(keyValueBroker,
	[this](char const * key, char const * value, bool fromPeer) {
	    std::string message = KeyValueBroker::serialize(key, value);
	    io.post([this, message](){
		ESP_LOGI(name, "spray %s", message.c_str());
		spray(message.c_str(), message.size());
	    });
	}
    )
{
    ESP_LOGI(name, "socket %d port %d WebSocketTask::WebSocketTask",
	acceptor.native_handle(), acceptor.local_endpoint().port());
    acceptSession();
}

WebSocketTask::~WebSocketTask() {
    stop();
    ESP_LOGI(name, "socket %d port %d WebSocketTask::~WebSocketTask",
	acceptor.native_handle(), acceptor.local_endpoint().port());
}
