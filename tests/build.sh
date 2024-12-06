#!/bin/bash
mkdir -p build
g++ -std=c++17 -O3 -o build/EchoServer EchoServer.cpp -lpthread -lrt
g++ -std=c++17 -O3 -o build/EchoClient EchoClient.cpp -lpthread -lrt
g++ -std=c++17 -O3 -o build/PubServer PubServer.cpp -lpthread -lrt
g++ -std=c++17 -O3 -o build/SubClient SubClient.cpp -lpthread -lrt
