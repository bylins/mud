#include "influxdb.h"

#include "utils/logger.h"

#include <chrono>

#ifndef WIN32
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>

constexpr int INVALID_SOCKET = -1;
constexpr int SOCKET_ERROR = -1;
#endif

namespace influxdb {
class SenderImpl {
 public:
	SenderImpl(const std::string &host, const unsigned short port);

	bool ready() const { return !m_host.empty(); }
	bool send(const std::string &data);

 private:
	std::string m_host;
	int m_port;

	socket_t m_socket;
	struct sockaddr_in m_addr;
};

SenderImpl::SenderImpl(const std::string &host, const unsigned short port) :
	m_host(host),
	m_port(port),
	m_socket(INVALID_SOCKET) {
	memset(&m_addr, 0, sizeof(m_addr));

	if (m_host.empty()) {
		return;
	}

	m_addr.sin_family = AF_INET;
	m_addr.sin_port = htons(port);

	struct hostent *hp = gethostbyname(m_host.c_str());
	if (hp) {
		in_addr *server_address = reinterpret_cast<in_addr *>(hp->h_addr_list[0]);
		log("Statistics server has been resolved to '%s'.\n",
			inet_ntoa(*server_address));
		memcpy(&m_addr.sin_addr, server_address, hp->h_length);
	} else {
		log("SYSERR: failed to resolve server name '%s'. Turning sending statistics off.\n", m_host.c_str());
		m_host.clear();
	}

	m_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (INVALID_SOCKET == m_socket) {
		log("SYSERR: Couldn't create UDP socket. Turning sending statistics off.\n");
		m_host.clear();
	}
}

bool SenderImpl::send(const std::string &data) {
	if (INVALID_SOCKET != m_socket) {
		const int result = sendto(m_socket, data.c_str(), static_cast<int>(data.size()),
								  0, reinterpret_cast<sockaddr *>(&m_addr), sizeof(m_addr));

		return SOCKET_ERROR != result;
	}

	return false;
}

Sender::Sender(const std::string &host, const unsigned short port) :
	m_implementation(new SenderImpl(host, port)) {
}

Sender::~Sender() {
	delete m_implementation;
}

bool Sender::ready() const {
	return m_implementation->ready();
}

bool Sender::send(const Record &record) const {
	std::string data;
	if (!record.get_data(data)) {
		return false;
	}

	return m_implementation->send(data);
}

bool Record::get_data(std::string &data) const {
	if (m_fields.empty()) {
		log("SYSERR: Attempt to send statistics record without any field.\n");
		return false;
	}

	std::stringstream ss;
	ss << m_measurement;
	for (const auto &tag : m_tags) {
		ss << "," << tag;
	}
	ss << " ";

	bool first = true;
	for (const auto &field : m_fields) {
		ss << (first ? "" : ",") << field;
		first = false;
	}

	using namespace std::chrono;
	nanoseconds timestamp = duration_cast<nanoseconds>(system_clock::now().time_since_epoch());
	ss << " " << timestamp.count();

	data = ss.str();
	return true;
}
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
