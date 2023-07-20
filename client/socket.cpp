#include "socket.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "executer.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <stack>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>

using std::cin;
using std::cout;
using std::string;

const char *address = "../local_socket_address";

// BaseSocket methods:
BaseSocket::BaseSocket()
{
	if((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		throw std::logic_error("Can't create socket.");
	}
	sock_addr.sun_family = AF_UNIX;
	strcpy(sock_addr.sun_path, address);
}

// ServerSocket methods:
void ServerSocket::socketBind() {
	unlink(address);
	length = sizeof (sock_addr.sun_family) + strlen (sock_addr.sun_path);
	if (bind(sock, (struct sockaddr *)&sock_addr, length) < 0) {
		throw std::logic_error("Can't bind the socket.");
	}
}

void ServerSocket::socketListen() {
	if (listen(sock, 1) < 0) {
		throw std::logic_error("An error in listen function occured.");
	}
}

void ServerSocket::socketAccept() {
	if ((fdServer = accept(sock, NULL, NULL)) < 0 ) {
		throw std::logic_error("Can't accept connection on the socket.");
	}
}

void ServerSocket::messagesExchange() {
    for(;;) {
		int msgLen;

        if (recv(fdServer, &msgLen, sizeof(msgLen), 0) < 0) {
            throw std::logic_error("Can't receive the message length.");
        }
    
        string command;
        
        char c;
        for (int i = 0; i < msgLen; i++) {
            if (recv(fdServer, &c, 1, 0) < 0) {
                throw std::logic_error("Can't receive the message.");    
            }  
            command += c;
        }
        
        if (command == "quit\n") {
            break;    
        }

        std::vector<string> result;

        try {
            lexer::curPos = 0;
            lexer::c = command[lexer::curPos];
            lexer::getLex(command);
            parser::sqlSentence(command);
            result = executer::execute(parser::poliz);
        } catch (std::exception& e) {
            result.push_back("error");
            result.push_back(e.what());            
        }

        parser::poliz.clear();
        while (parser::polizStack.size()) {
            parser::polizStack.pop();    
        }

        if (result[0] == "error") {
            int resultLen = result[1].size();

            if (send(fdServer, &resultLen, sizeof(resultLen), 0) < 0) {
                throw std::logic_error("Can't send the result length.");
            }

            if(send(fdServer, result[1].c_str(), result[1].size(), 0) < 0) {
                throw std::logic_error("Can't send the answer to the client.");
            }
        } else if (result[0] == "OK") {
            int resultLen = 2;

            if (send(fdServer, &resultLen, sizeof(resultLen), 0) < 0) {
                throw std::logic_error("Can't sent the result length.");
            }

            if (send(fdServer, result[0].c_str(), result[0].size(), 0) < 0) {
                throw std::logic_error("Can't send the answer to the client.");
            }

        } else {
            // result[0] == "file"

            int fdAnswer;
            if ((fdAnswer = open(result[1].c_str(), O_RDONLY)) < 0) {
                throw std::logic_error("Can't open the file \
                with SELECT command result.");
            }
            
            int resultLen;

            if ((resultLen = lseek(fdAnswer, 0, SEEK_END)) < 0) {
                throw std::logic_error("Can't get the result file size.");
            }
            if (send(fdServer, &resultLen, sizeof(resultLen), 0) < 0) {
                throw std::logic_error("Can't send the result file size.");
            }
            
            if (lseek(fdAnswer, 0, SEEK_SET) < 0) {
                throw std::logic_error("Can't move to the beginning of the result file.");        
            }

            char buf[256];
            int nr;

            while ((nr = read(fdAnswer, buf, 256)) > 0) {
                if (send(fdServer, buf, nr, 0) < 0) {
                    throw std::logic_error("Can't send the answer to the client.");
                }
            }
            
            unlink("select_res");

            if (nr < 0) {
                throw std::logic_error("Can't read from the file with  SELECT command result.");
            }
	    }
    }
}

ServerSocket::~ServerSocket() {
    close(sock);
}

// ClientSocket methods:
void ClientSocket::socketConnect() {
	length = sizeof(sock_addr.sun_family) + strlen(sock_addr.sun_path);
	if (connect(sock, (struct sockaddr *)&sock_addr, length) < 0) {
		throw std::logic_error("Can't connect to the server.");
	}
}

void ClientSocket::messagesExchange() {
    for (;;) {
        string command;
        getline(cin, command);
        command += '\n';
        
        int msgLen = command.size();
        if (send(sock, &msgLen, sizeof(msgLen), 0) < 0) {
            throw std::logic_error("Can't send the message length.");    
        }

        for (size_t i = 0; i < command.size(); i++) {
            char c = command[i];
            if (send(sock, &c, 1, 0) < 0) {
                throw std::logic_error("Can't send the message.");
            }
        }

        if (command == "quit\n") {
            break;
        }

        int resultLen;
        if (recv(sock, &resultLen, sizeof(resultLen), 0) < 0) {
            throw std::logic_error("Can't receive the message length.");
        }
         
        char c;
        for (int i = 0; i < resultLen; i++) {
            if (recv(sock, &c, 1, 0) < 0) {
                throw std::logic_error("Can't receive the message.");
            }
            cout << c;
        }
        cout << '\n';
    }
}

ClientSocket::~ClientSocket() {
	close(sock);
}