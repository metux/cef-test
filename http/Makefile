
server: main.c minihttp.c
	$(CC) -std=c11 -O2 -Wall -pthread $^ -o $@

clean:
	rm -f *.o server

.PHONY: clean
