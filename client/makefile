COMPILER = g++
COMPILER_FLAGS = -g -O2 -ftrapv -Wall -Werror -Wextra

all:
	$(COMPILER) $(FLAGS) -o client client.cpp socket.cpp socket.hpp lexer.cpp lexer.hpp parser.cpp parser.hpp executer.cpp executer.hpp dbms.cpp dbms.hpp
clean:
	rm client