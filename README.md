# cpp-tcp-server-examples
Some fake C++ tcp/http server examples

## Compile
```sh
make [http|chat]
```
To show debug messages:
```sh
make (http|chat)_debug
```

## Run http server
```
./server.out [-h <host>] [-p<port>]
```
    Default host: 127.0.0.1
    Default port: 8080

## Test http server performance
```
curl <host>:<port>/

ab -n10000 -c10 <host>:<port>/
```

## Test chat server
Run following code in several terminals.
```sh
telnet <host> <port>
```
And chat with each other.

## Note
epoll is only available on Linux
