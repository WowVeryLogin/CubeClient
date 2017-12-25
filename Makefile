all: test

build: cli.o protocol.o net.o
	gcc cli.o protocol.o net.o -o cube

cli.o: cli.c
	gcc -c cli.c

net.o: net.c
	gcc -c net.c

protocol.o: protocol.c
	gcc -c protocol.c

test: build build_unittests build_tests
	./unittests && ./test

build_tests: test.c fake_server
	gcc test.c -o test

fake_server: fake_server.c protocol.o
	gcc fake_server.c protocol.o -o fake_server

build_unittests: unittests.c protocol.o
	gcc unittests.c protocol.o -o unittests

clean:
	rm -rf *.o cube unittests test fake_server
