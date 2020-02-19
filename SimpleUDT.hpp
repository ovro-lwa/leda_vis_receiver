
#pragma once

#include <sstream>
#include <stdexcept>

#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <netdb.h>

#include <udt.h>

// Library-level initialisation and cleanup
// TODO: How important is this?
struct SimpleUDT_ScopedContext {
   SimpleUDT_ScopedContext()  { UDT::startup(); }
   ~SimpleUDT_ScopedContext() { UDT::cleanup(); }
};

void* simpleUDT_connection_opened(void* connection_ptr);

class SimpleUDT_Connection {
public:
	typedef void (*callback_type)(SimpleUDT_Connection* connection,
	                              const char*           address,
	                              unsigned short        port,
	                              void*                 user_data);
private:
	UDTSOCKET      _sock;
	callback_type  _user_callback;
	void*          _user_data;
	std::string    _client_addr;
	unsigned short _client_port;
public:
	SimpleUDT_Connection(const UDTSOCKET& connection,
	                     callback_type    user_callback,
	                     void*            user_data,
	                     const char*      client_addr,
	                     const char*      client_port);
	~SimpleUDT_Connection();
	void user_callback();
	void receive(char* data, size_t size);
};

class SimpleUDT_Server {
	UDTSOCKET _sock;
public:
	SimpleUDT_Server(unsigned short port,
	                 int mss=9000);
	~SimpleUDT_Server();
	void set_mtu(int mtu);
	void accept(SimpleUDT_Connection::callback_type user_callback,
	            void*                               user_data=0);
};

SimpleUDT_Server::SimpleUDT_Server(unsigned short port,
                                   int mss) {
	std::stringstream ss;
	ss << port;
	const char* port_str = ss.str().c_str();

	struct addrinfo hints, *local;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_flags    = AI_PASSIVE;
	hints.ai_family   = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	if( getaddrinfo(NULL, port_str, &hints, &local) != 0 ) {
		throw std::runtime_error("Invalid network port");
	}
	_sock = UDT::socket(local->ai_family, local->ai_socktype, local->ai_protocol);
	if( UDT::bind(_sock, local->ai_addr, local->ai_addrlen) == UDT::ERROR ) {
		throw std::runtime_error(std::string("Failed to bind: ")+UDT::getlasterror().getErrorMessage());
	}
	freeaddrinfo(local);
	UDT::setsockopt(_sock, 0, UDT_MSS, &mss, sizeof(int));
	int rcvbuf_udt = 8*128*1024*1024;
	UDT::setsockopt(_sock, 0, UDT_RCVBUF, &rcvbuf_udt, sizeof(int));
	int fc = rcvbuf_udt/mss;
	UDT::setsockopt(_sock, 0, UDT_FC, &fc, sizeof(int));
	
	//int rcvbuf_udp =  16*1024*1024;
	//UDT::setsockopt(_sock, 0, UDP_RCVBUF, &rcvbuf_udp, sizeof(int));
	int max_pending = 32;
	if( UDT::listen(_sock, max_pending) == UDT::ERROR ) {
		throw std::runtime_error(std::string("Failed to listen: ")+UDT::getlasterror().getErrorMessage());
	}
}
SimpleUDT_Server::~SimpleUDT_Server() {
	UDT::close(_sock);
}
// TODO: This needs to update UDT_FC as well in cases where UDT_RCVBUF is set very high
//void SimpleUDT_Server::set_mtu(int mtu) {
//	UDT::setsockopt(_sock, 0, UDT_MSS, &mtu, sizeof(int));
//}
void SimpleUDT_Server::accept(SimpleUDT_Connection::callback_type user_callback,
                              void* user_data) {
	sockaddr_storage clientaddr;
	int addrlen = sizeof(clientaddr);
	UDTSOCKET connection;
	if( (connection = UDT::accept(_sock, (sockaddr*)&clientaddr, &addrlen)) == UDT::INVALID_SOCK ) {
		throw std::runtime_error(std::string("Failed to accept: ")+UDT::getlasterror().getErrorMessage());
	}
	char client_addr[NI_MAXHOST];
	char client_port[NI_MAXSERV];
	getnameinfo((sockaddr *)&clientaddr, addrlen,
	            client_addr, sizeof(client_addr),
	            client_port, sizeof(client_port),
	            NI_NUMERICHOST | NI_NUMERICSERV);
	//std::cout << "New connection from " << client_addr << ":" << client_port << std::endl;
	pthread_t recv_thread;
	pthread_create(&recv_thread, NULL, simpleUDT_connection_opened,
	               new SimpleUDT_Connection(connection,
	                                        user_callback, user_data,
	                                        client_addr, client_port));
	pthread_detach(recv_thread);
}

void* simpleUDT_connection_opened(void* connection_ptr) {
	SimpleUDT_Connection connection = *(SimpleUDT_Connection*)connection_ptr;
	delete (SimpleUDT_Connection*)connection_ptr;
	connection.user_callback();
	return 0;
}

SimpleUDT_Connection::SimpleUDT_Connection(const UDTSOCKET& connection,
                                           callback_type    user_callback,
                                           void*            user_data,
                                           const char*      client_addr,
                                           const char*      client_port)
	: _sock(connection),
	  _user_callback(user_callback),
	  _user_data(user_data),
	  _client_addr(client_addr),
	  _client_port(atoi(client_port)) {}
SimpleUDT_Connection::~SimpleUDT_Connection() { /*UDT::close(_sock);*/ }
void SimpleUDT_Connection::user_callback() {
	return _user_callback(this, _client_addr.c_str(), _client_port, _user_data);
}
void SimpleUDT_Connection::receive(char* data, size_t size) {
	size_t total_recv = 0;
	while( total_recv < size ) {
		int received;
		if( (received = UDT::recv(_sock, data+total_recv, size-total_recv, 0)) == UDT::ERROR ) {
			throw std::runtime_error(std::string("Failed to recv: ")+UDT::getlasterror().getErrorMessage());
		}
		total_recv += received;
	}
}

class SimpleUDT_Client {
	UDTSOCKET _sock;
	bool      _connected;
public:
	SimpleUDT_Client() : _connected(false) {
		create();
	}
	~SimpleUDT_Client() {
		UDT::close(_sock);
	}
	void create() {
		_connected = false;
		struct addrinfo hints, *local;
		memset(&hints, 0, sizeof(struct addrinfo));
		hints.ai_flags    = AI_PASSIVE;
		hints.ai_family   = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		const char* port_str = "9999"; // TODO: Is this value important?
		if( getaddrinfo(NULL, port_str, &hints, &local) != 0 ) {
			throw std::runtime_error("Invalid network port");
		}
		_sock = UDT::socket(local->ai_family, local->ai_socktype, local->ai_protocol);
		freeaddrinfo(local);
		int mss = 9000;
		UDT::setsockopt(_sock, 0, UDT_MSS, &mss, sizeof(int));
		// Note: This is important for avoiding stalls on UDT::close(_sock);
		struct linger lin;
		lin.l_onoff  = 0;//1;
		lin.l_linger = 0;//3;
		UDT::setsockopt(_sock, 0, UDT_LINGER, &lin, sizeof(struct linger));
		/*
		bool blocking = false;//true;
		UDT::setsockopt(_sock, 0, UDT_SNDSYN, &blocking, sizeof(bool));
		*/
		/*
		bool reuse_addr = false;//true;
		UDT::setsockopt(_sock, 0, UDT_REUSEADDR, &reuse_addr, sizeof(bool));
		*/
		// TODO: Check if these improve performance
		int sndbuf_udt = 8*128*1024*1024;
		UDT::setsockopt(_sock, 0, UDT_SNDBUF, &sndbuf_udt, sizeof(int));
		//int sndbuf_udp =  16*1024*1024;
		//UDT::setsockopt(_sock, 0, UDP_SNDBUF, &sndbuf_udp, sizeof(int));
	}
	void recreate() {
		UDT::close(_sock);
		create();
	}
	void connect(std::string addr, unsigned short port) {
		_connected = false;
		std::stringstream ss;
		ss << port;
		const char* addr_str = addr.c_str();
		const char* port_str = ss.str().c_str();
		struct addrinfo hints, *remote;
		memset(&hints, 0, sizeof(struct addrinfo));
		hints.ai_flags    = AI_PASSIVE;
		hints.ai_family   = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		if( getaddrinfo(addr_str, port_str, &hints, &remote) != 0 ) {
			throw std::runtime_error("Invalid network address");
		}
		if( UDT::connect(_sock, remote->ai_addr, remote->ai_addrlen) == UDT::ERROR ) {
			freeaddrinfo(remote);
			throw std::runtime_error(std::string("Failed to connect: ")+UDT::getlasterror().getErrorMessage());
		}
		freeaddrinfo(remote);
		_connected = true;
	}
	void set_mtu(int mtu) {
		UDT::setsockopt(_sock, 0, UDT_MSS, &mtu, sizeof(int));
	}
	void set_timeout(float secs) {
		int ms = int(secs*1e3);
		UDT::setsockopt(_sock, 0, UDT_SNDTIMEO, &ms, sizeof(int));
	}
	void send(const char* data, size_t size) {
		if( !_connected ) {
			throw std::runtime_error("Cannot send, not connected");
		}
		
		size_t total_sent = 0;
		while( total_sent < size ) {
			int sent = UDT::send(_sock, data+total_sent, size-total_sent, 0);
			if( sent == UDT::ERROR ) {
				recreate();
				throw std::runtime_error(std::string("Failed to send: ")+UDT::getlasterror().getErrorMessage());
			}
			total_sent += sent;
		}
	}
	bool connected() const { return _connected; }
};
