ALL:client server
client:test_cli.cpp
	g++ $^ -o $@ -lpthread -std=c++11
server:test_svr.cpp
	g++ $^ -o $@ -lpthread -std=c++11
.PHONY:clean
clean:
	rm -rf client server