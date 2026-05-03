main:
	gcc -Iheaders src/server.c src/handlers.c src/dbh.c src/socks.c src/interdb.c -o server -lmysqlclient -lpthread

client:
	gcc -Iheaders src/client.c src/socks.c -lraylib -lm -o client

clean:
	rm server client
