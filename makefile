all:
	gcc -g -o master master.c comm.c 
	gcc -g -o cli nat_cli.c comm.c -lpthread
	gcc -g -o srv nat_srv.c comm.c -lpthread

clean:
	rm -rf *.o master cli srv

