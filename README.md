# cpp-tcp-server-examples
Some fake C++ tcp/http server examples

## Compile and Run server
```sh
make
./server.out
```

## Test server performance
```
curl localhost:8080/

ab -n10000 -c10 localhost:8080/
```

## Note
epoll is only available on Linux
