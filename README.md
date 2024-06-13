## http-server

### Usage

#### 1. Build and run with Make (gcc)

##### Get a list of the available options:

`make help`

##### Start up the server:

`make server`

> At this point, we could just open up `127.0.0.1:3490` in a browser instead of launching the client (I recommend the browser as the client is an even worse implementation that the server)

##### Launch the client:

`make client [host=<n>| -n <ipaddr/hostname>]`

#### 2. Build without Make (gcc)

##### Clone this repo & cd into it:

`git clone git@github.com:ArshhGill/http-server.git | cd http-server`

##### Compile the server with gcc:

`gcc [-o <l>| -l <output-name-location>] src/server.c`

Run the output directly to get the server up or create a systemd service

##### Compile the client with gcc:

`gcc [-o <l>| -l <output-name-location>] src/client.c`

##### Run the output:

`[<output-location>] [hostname=<n>| -n <ipaddr/hostname>]`
