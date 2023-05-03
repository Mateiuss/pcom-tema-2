build: server subscriber

server: server.cpp
	g++ -g -std=c++11 -o server server.cpp common.cpp

subscriber: subscriber.cpp
	g++ -g -std=c++11 -o subscriber subscriber.cpp common.cpp

clean:
	rm -f server subscriber