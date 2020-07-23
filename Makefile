http:
	g++ epoll_http_server.cpp -std=c++11 -O2 -lpthread -o server.out

http_debug:
	g++ epoll_http_server.cpp -std=c++11 -O2 -lpthread -D DEBUG -o server.out

chat:
	g++ epoll_chat_server.cpp -std=c++11 -O2 -o server.out

chat_debug:
	g++ epoll_chat_server.cpp -std=c++11 -O2 -D DEBUG -o server.out
