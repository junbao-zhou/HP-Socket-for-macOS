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

#### 软件架构
软件架构说明


#### 安装教程

1. xxxx
2. xxxx
3. xxxx

#### 使用说明

1. xxxx
2. xxxx
3. xxxx

#### 参与贡献

1. Fork 本项目
2. 新建 Feat_xxx 分支
3. 提交代码
4. 新建 Pull Request


#### 码云特技

1. 使用 Readme\_XXX.md 来支持不同的语言，例如 Readme\_en.md, Readme\_zh.md
2. 码云官方博客 [blog.gitee.com](https://blog.gitee.com)
3. 你可以 [https://gitee.com/explore](https://gitee.com/explore) 这个地址来了解码云上的优秀开源项目
4. [GVP](https://gitee.com/gvp) 全称是码云最有价值开源项目，是码云综合评定出的优秀开源项目
5. 码云官方提供的使用手册 [https://gitee.com/help](https://gitee.com/help)
6. 码云封面人物是一档用来展示码云会员风采的栏目 [https://gitee.com/gitee-stars/](https://gitee.com/gitee-stars/)