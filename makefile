main:
	gcc -Iheaders src/server.c src/handlers.c src/dbh.c src/socks.c src/interdb.c -o server -lmysqlclient -lpthread

client:
	gcc -Iheaders src/socks.c src/client.c -o client

clean:
	rm server client
