#include "socket.hpp"

#include <iostream>

using std::cout;

int main()
{
	try{
		ServerSocket server;
		server.socketBind();
		server.socketListen();
		server.socketAccept();
		server.messagesExchange(); // взаимодействие сервера и клиента
	}
	catch (std::exception & except) {
		cout << except.what() << '\n';
	}

	return 0;
}
