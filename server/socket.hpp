// #pragma once
#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

using std::string;

// Classes declaration:
class BaseSocket {
protected:
	struct sockaddr_un sock_addr;	
	int sock, length;
	string text;
	char tmp;
public:
	BaseSocket();			
	virtual void messagesExchange() = 0;	
};

// ServerSocket virtual class:
class ServerSocket: public BaseSocket {
private:
	int fdServer;
public:
	ServerSocket(): BaseSocket(){};
	~ServerSocket();					
	void socketBind();			
	void socketListen();			
	void socketAccept();			
	virtual void messagesExchange();
};

// ClientSocket virrtual class:
class ClientSocket: public BaseSocket {
public:
	ClientSocket(): BaseSocket(){};
	~ClientSocket();			
	void socketConnect();		
	virtual void messagesExchange();		
};

#endif