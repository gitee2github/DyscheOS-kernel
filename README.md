# DyscheOS-kernel

#### 介绍

##### 概述

操作系统是用于管理和控制计算机硬件的程序，为上层软件或用户提供硬件或底层软件的使用接口，最大限度利用计算机系统的资源。因此操作系统的发展与硬件演进息息相关，当前不同领域（如计算，IoT，大数据，AI等）各有其独特的硬件诉求；同时处理器也在不断升级，指令集生态愈加丰富，核数更多且单体计算能力更强；硬件设备也更加多样性以及更加智能化，设备本身具备计算和管理的能力，部分设备如GPU，NPU和FPGA等在特定领域计算能力远超CPU。

基于这些新型的硬件和使用场景，操作系统要如何演进？宏内核使用广泛但是在众核场景下存在扩展性问题；微内核设计简单且更加轻量，但是性能无法满足要求；学术界和业界也在积极探索新的操作系统架构如multi-kernel，multiOS等，以适配新的硬件平台。

关于操作系统的探索方向很多，有基于现有系统的微创新，也有颠覆性的设计；我们希望能够利用现有的成熟系统，通过一定的技术方案，在众核异构的大型系统上提供一种可动态伸缩的异构操作系统（DYnamic SCalable HEterogeneous Operating System） - DyscheOS。

##### sig-DyscheOS范围

DyscheOS兴趣小组希望能够在openEuler系统上，实现一些技术方案，将大型系统灵活划分为一系列可独立运行又互联的小型子系统，子系统之间未必相同架构，可以借此为复杂的硬件平台提供更多的灵活性、更好的性能及扩展性；我们需要基于openEuler平台探索并实现以下方案：

* 硬件资源的灵活划分

* 部署多OS的能力，基于划分好的资源部署运行任意OS或baremetal程序

* 多OS间资源隔离，OS独立运行，相互隔离

* OS之间的高效通信，通过共享内存和中断提供一套跨OS的高性能通信机制

* 共有资源的有限共享，系统中存在一些不可划分的特殊设备，需要能够实现此类设备在各OS间的共享

* 多OS协同，借鉴distributed OS相关技术，实现大型业务在多OS之间的高效协同工作

#### 软件架构
软件架构说明


#### 安装教程

1.  xxxx
2.  xxxx
3.  xxxx

#### 使用说明

1.  xxxx
2.  xxxx
3.  xxxx

#### 参与贡献

1.  Fork 本仓库
2.  新建 Feat_xxx 分支
3.  提交代码
4.  新建 Pull Request


#### 特技

1.  使用 Readme\_XXX.md 来支持不同的语言，例如 Readme\_en.md, Readme\_zh.md
2.  Gitee 官方博客 [blog.gitee.com](https://blog.gitee.com)
3.  你可以 [https://gitee.com/explore](https://gitee.com/explore) 这个地址来了解 Gitee 上的优秀开源项目
4.  [GVP](https://gitee.com/gvp) 全称是 Gitee 最有价值开源项目，是综合评定出的优秀开源项目
5.  Gitee 官方提供的使用手册 [https://gitee.com/help](https://gitee.com/help)
6.  Gitee 封面人物是一档用来展示 Gitee 会员风采的栏目 [https://gitee.com/gitee-stars/](https://gitee.com/gitee-stars/)
