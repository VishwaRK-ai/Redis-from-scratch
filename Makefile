all:
	clang++ -std=c++14 main.cpp RedisServer.cpp ClientHandler.cpp RESPParser.cpp Store.cpp -o server

clean:
	rm -f server