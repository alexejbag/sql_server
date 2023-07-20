#include "socket.hpp"

#include <iostream>

using std::cout;

int main()
{
	try {
		ClientSocket client;
		client.socketConnect();
		client.messagesExchange(); // взаимодействие сервера и клиента
	}
	catch (std::exception & except) {
		cout << except.what() << '\n';
	}
	
	return 0;
}