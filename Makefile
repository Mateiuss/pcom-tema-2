build: server subscriber

server: server.cpp
	g++ -std=c++11 -o server server.cpp

subscriber: subscriber.cpp
	g++ -std=c++11 -o subscriber subscriber.cpp

clean:
	rm -f server subscriber