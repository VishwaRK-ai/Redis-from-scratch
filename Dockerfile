FROM ubuntu:22.04

RUN apt-get update && apt-get install -y clang

WORKDIR /app

COPY . .

RUN clang++ -std=c++14 main.cpp RedisServer.cpp ClientHandler.cpp RESPParser.cpp Store.cpp -o server

EXPOSE 6379

CMD ["./server"]