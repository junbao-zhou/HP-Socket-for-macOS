# HP-Socket for macOS

#### 项目介绍
由于平台限制某些机制某些机制会无法正常提供。

己知无法正常提供机制：

1、TCP协议中设置Keep live 时间:`TCP_KEEPALIVE`、`TCP_KEEPINTVL`和`TCP_KEEPCNT`。

2、对平台版本(macOS系统版本)有所要求，必需`10.12`之上的版本(包含)。

对平台版本的要求主要是其中组件`RWLock.h`中依赖`shared_timed_mutex`，而根据`_LIBCPP_AVAILABILITY_SHARED_MUTEX`定义对平台版本有所要求(若引入boost进行替代也可解决此问题)。

HP-Socket for macOS版本只是提供与其他平台上相同的回调接口，内部使用其他一些机制与组件替换相应不兼容的组件。

机制：

`epoll` => `kqueue`

组件:

`MessagePipe.h`

HP-Socket for macOS版本(改自HP-Socket for linux 5.3.2)。

暂时只还原`TcpServer`、`TcpClient`、'UdpServer'和`UdpClient`