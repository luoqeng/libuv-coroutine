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

## event loop
* 服务器做高性能时基本上会落在IO模型这一块上面，因为CPU/内存/IO的制约关系，最弱的一环就是最需要注意的一环。

* IO模型跟编程模型又是密切相关的，最终都关于控制流管理。基本的常识是，IO阻塞的时候，我们应该把CPU拿去做其它事情，也就是控制流的切换。


## coroutine
* coroutine是一种控制流的高级抽象
* 简单理解coroutine只是多了suspend和resume功能的函数而已。
```
co = coroutine.create(function ()
    print("a")
    coroutine.yield()

    print("b")
    coroutine.yield()

    print("c")
end)

coroutine.resume(co)    -- a
coroutine.resume(co)    -- b
coroutine.resume(co)    -- c
```


## callback
 * 注册好回调的函数，在run做主循环，当IO(或者更抽象的概念–事件)可用的时候调用注册好的回调函数。在回调返回的时候，通常还会注册下一次的回调。上下文切换是在回调的返回过程中。状态是保存在一个全局的数据结构中，传递到下一次的回调。

 * 这种控制流的问题是要手动地管理状态，逻辑被打散在callback中了，编程很不友好。开发者的思维必须异步：等到什么样的事件发生后，进行什么样的回调处理，完成之后再做什么样的操作。
```
local uv = require('luv')

local timer = uv.new_timer()
timer:start(1000, 0, function ()
    print ("Awake!")
    timer:close()
end)

print("Sleeping");

uv.run()
```



## coroutine
* coroutine相对于callback的好处是解放思维，用户还是按同步的思维去写代码，代码遇到IO阻塞执行不下去时会自动切换，对用户是透明的。
```
local uv = require('luv')

co = coroutine.create(function (co)
    print("Sleeping")

    local timer = uv.new_timer()
    local str = ""
    timer:start(1000, 0, function ()
        str = "Awake!"
        timer:close()
        coroutine.resume(co)
    end)
    coroutine.yield()

    print(str)
end)

coroutine.resume(co, co);
uv.run()
```

## scheduler

* 实现一个与golang GMP调度器机制一样的调度器，可以得到golang子集的能力。

* golang的协程最大的优势在于它是语言内置的统一并发风格。从内存管理，gc到网络库, syscall以及对应的runtime实现的M:N的调度，都有很深的优化。

* golang的数据库driver都是异步，而c&c++的数据库driver都是同步的，异步需要自己集成异步io复用写回调风格代码。

* 可以对比grpc glang版本与c++异步版本代码, 使用协程grpc c++异步版本也可以改成同步风格代码。

* 让 rpc 调用 request 与 response 在同一个函数内处理。


reference:
 - https://github.com/hnes/libaco
