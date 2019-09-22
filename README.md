# HP-Socket for macOS

#### 项目介绍
由于平台限制某些机制某些机制会无法正常提供。

己知无法正常提供机制：

1、TCP协议中设置Keep live 时间:`TCP_KEEPALIVE`、`TCP_KEEPINTVL`和`TCP_KEEPCNT`（[解决方法](http://www.voidcn.com/article/p-afuibcmg-bqk.html)）。

2、对平台版本(macOS系统版本)有所要求，必需`10.12`之上的版本(包含)。

对平台版本的要求主要是其中组件`RWLock.h`中依赖`shared_timed_mutex`，而根据`_LIBCPP_AVAILABILITY_SHARED_MUTEX`定义对平台版本有所要求(若引入boost进行替代也可解决此问题)。

HP-Socket for macOS版本只是提供与其他平台上相同的回调接口，内部使用其他一些机制与组件替换相应不兼容的组件。

机制：

`epoll` => `kqueue`

部分`Server` 从`ONESHOT`=>`DISPATCH`

```tex
`DISPATCH`等同与epoll中的`edge-triggered (ET)`，在macOS中`ONESHOT`会从内核中删除，而对于大量数据交换就代表有大量删除与添加，而使用ET(DISPATCH)只会重置状态(ENABLE)使其无效。
```

组件:

`MessagePipe.h`

HP-Socket for macOS版本(改自HP-Socket for linux 5.5.3)

#### 支持

- `TcpServer/Client` 
- `UdpServer/Client`
- `TcpPackServer/Client`
- `TcpAgent`
- `TcpPackAgent`
- `HttpClient/Server`
- `ARQ-UdpServer/Client`
- `SSL`

------

#### 文件目录介绍

| 目录名                               | 描述                                                |
| ------------------------------------ | --------------------------------------------------- |
| hpsocket                             | hpsocket网络框架相关的所有文件                      |
| hpsocket\qt-hpsocket                 | 存有qt使用hpsocket的简单例子                        |
| hpsocket\qt-hpsocket\hpsocket.pro    | 生成静态库/qt creator工程配置文件                   |
| hpsocket\qt-hpsocket\qt-hpsocket.pro | 使用hpsocket源文件的简单例子/qt creator工程配置文件 |
| hpsocket\qt-hpsocket\qt-lib-demo     | 使用静态库的简单例子/qt creator工程配置文件         |
| hpsocket\src                         | hpsocket的源文件                                    |
| hpsocket\ssl-cert                    | 测试ssl的相关证书                                   |
| hpsocket\test                        | 一些简单的测试例子                                  |
| hpsocket\CMakeLists.txt              | cmake的配置文件                                     |
| hpsocket\4C                          | hpsocket框架提供的类C接口                           |

提供的例子中请注意依赖头文件与库路径。

> CMakeLists.txt

```
include_directories(/opt/local/include)
link_directories(/opt/local/lib)
#暂时关闭对ssl的支持
#link_libraries(ssl iconv z crypto)
add_definitions(-D_SSL_DISABLED)
```

> *.pro

```tex
#暂时关闭对ssl的支持
DEFINES += _SSL_DISABLED
QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.12

#ssl的依赖库
#macx: LIBS += -L/opt/local/lib -lssl -lcrypto -lglog
INCLUDEPATH += /opt/local/include
```

------

#### 如何使用`4C`接口？

> 将4C文件夹下的文件放置src目录下，删除以下文件

```tex
HPSocket.cpp
HPSocket.h
HPSocket-SSL.cpp
HPSocket-SSL.h
```

并提供了一个官方使用4C接口的http例子`test_server_4c`

后面将完善相关例子与工程配置