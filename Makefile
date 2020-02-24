all: send recv

link_emulator/lib.o:
	$(MAKE) -C link_emulator

send: send.o link_emulator/lib.o
	gcc -c aux.c -o aux.o
	gcc -g -std=c99 send.o link_emulator/lib.o link_emulator/queue.o aux.o -o send

recv: recv.o link_emulator/lib.o
	gcc -c aux.c -o aux.o
	gcc -g  -std=c99 recv.o link_emulator/lib.o link_emulator/queue.o aux.o -o recv
	
aux.o: aux.c
	gcc aux.c -o aux.o

.c.o:
	gcc -Wall -g -c $?

clean:
	$(MAKE) -C link_emulator clean
	-rm -f *.o send recv
