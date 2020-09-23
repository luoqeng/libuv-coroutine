# libuv-coroutine
libuv coroutine

build
```
$ git clone https://github.com/libuv/libuv.git
$ cd libuv
$ sh autogen.sh
$ ./configure
$ make
$ make check
$ sudo make install

$ git clone https://github.com/luoqeng/libuv-coroutine.git
$ cd libuv-coroutine
$ gcc -o tcp_callback tcp_callback.c -luv
$ gcc -o tcp_coroutine acosw.S aco.c tcp_coroutine.c -luv
```

reference:
 - https://github.com/hnes/libaco
