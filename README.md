# cpp-tcp-server-examples
Some fake C++ tcp/http server examples

## Compile
```sh
make
```
To show debug messages:
```sh
make epoll_debug
```

## Run server
```
./server.out [-h <host>] [-p<port>]
```
    Default host: 127.0.0.1
    Default port: 8080

## Test server performance
```
curl <host>:<port>/

ab -n10000 -c10 <host>:<port>/
```

## Note
epoll is only available on Linux
