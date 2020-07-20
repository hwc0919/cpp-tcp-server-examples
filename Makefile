epoll:
	g++ epoll_server.cpp -std=c++11 -O2 -lpthread -o server.out

epoll_debug:
	g++ epoll_server.cpp -std=c++11 -O2 -lpthread -D DEBUG -o server.out
