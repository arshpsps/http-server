## http-server

### Usage

#### 1. Build and run with Make (gcc)

###### Get a list of the available options:

`make help`

###### Start up the server:

`make server`

###### Launch the client:

`make client [host=<n>| -n <ipaddr/hostname>]`

#### 2. Build without Make (gcc)

###### Clone this repo & cd into it:

`git clone git@github.com:ArshhGill/http-server.git | cd http-server`

###### Compile the server with gcc:

`gcc [-o <l>| -l <output-name-location>] src/server.c`

Run the output directly to get the server up or create a systemd service

###### Compile the client with gcc:

`gcc [-o <l>| -l <output-name-location>] src/client.c`

###### Run the output:

`[<output-location>] [hostname=<n>| -n <ipaddr/hostname>]`

