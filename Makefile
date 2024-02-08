all:
	gcc http-server.c -o http-server.out

clean:
	rm http-server.out

run:
	./http-server.out