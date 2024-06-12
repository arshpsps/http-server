help:
	@ echo "usage: server"
	@ echo "       client [host=<n>| -n <ipaddr/hostname>]"

client: ./src/client.c | builds
	@ gcc -o builds/CLIENToutput ./src/client.c
	@ ./builds/CLIENToutput $(host)

server: ./src/server.c | builds
	@ gcc -o builds/HTTPoutput ./src/server.c
	@ ./builds/HTTPoutput

builds:
	mkdir -p $@
