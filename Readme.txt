Something to say...
===================
This project was first created and written during Mar.-June in 2009, when I was
unemployed, just for fun. But with almost no updates later.
Though it's developed on Visual Studio 2005 originally, it can easily be 
auto-upgraded to Visual Studio 2008, 2010, 2013. Actually, the latest version,
that is, the version uploaded to GitHub, is VS2013 formatted.
Previously, this project was fallen into a rather long sleep on Visual Studio 
Online. VSO is an excellent and powerful online collaborative platform. 
But by comparison to github, I think the latter one is easier to share and
collaborate. So I decide to host it on github now.
Maybe in the future, I'd like to add some new features such as user registration,
user records, etc. just for personal interests and fun.


简介

本软件为斗地主游戏软件，包括服务器与客户端，二者均为Windows应用程序，并且是在
Windows XP环境下使用Visual Studio 2005开发完成。目前，可以确认运行的平台为Windows XP。

服务器程序使用 Win32 C 编写。服务器支持最多300个客户端连接，支持100张
游戏桌，支持玩家托管游戏，支持游戏旁观功能（每张椅子限制最多允许2个旁观者），
支持基本聊天功能。目前，服务器没有与数据库连接，因此，不支持用户认证，不支持保存
玩家游戏积分等。

客户端程序使用 Win32 C/C++ 编写。支持服务器定制（添加、删除、编辑所选服务器参数），
支持游戏大厅功能，支持常规的斗地主游戏及旁观功能，支持基本聊天功能。不支持的功能
很多，只要您能想到。


声明

1.  作者开发并发布本程序的目的仅为交流与分享软件开发与设计经验所用。作者允许您
    以任何目的使用本软件及其代码，您可以修改、传播，只要您不声明其代码原作者是您。
    此外，本软件使用到的每三方软件代码，以及一些资源素材都是从网上搜集，不保证
    它们没有私有版权。您若以商业目的使用这些代码或资源，引起任何纠纷将与作者无关。
    
2.  虽然本软件在通常情况下，能够很好的运行，但它并没有经过严格的测试，因此，作者
    不保证软件没有错误或漏洞。您若坚持使用，则您自己将沉担一切风险与后果。


源码目录简介

_bin        编译输出目录
_obj        编译中间目录
CWebPage    从网上找到的一个支持Win32应用程序中使用IE的模块。该项目将生成一个DLL。
DdzClient   斗地主软件客户端项目。代码主要由C/C++编写。
DdzServer   斗地主软件服务器项目。代码主要由C编写。
Docs        一些简单的软件开发设计文档。
Include     服务器与客户端程序都可能使用到的头文件。（ResImage、ResPoker、ResSound 
            资源项目生成的头文件将置于该目录中）。
PokerLib    斗地主游戏中扑克牌的算法逻辑。它是DLL项目，主要用于扑克牌的识别与提示。
ResImage    客户端软件中使用到图标资源DLL项目。
ResPoker    客户端软件中使用到的扑克牌相关的资源DLL项目。
ResSound    客户端软件中使用到的游戏声音资源DLL项目。
SetupClient 客户端软件安装项目，用于制作客户端软件安装程序。
SetupServer 服务器软件安装项目，用于制作服务吕软件安装程序。
ThirdPartySoftware 软件中使用到的第三方源代码的原版本。


软件使用简介

1.  服务器

在具备网络功能的主机上运行服务器程序即可。程序将使用从主机搜索到的第一个IP，并默认
使用端口26008，除非您修改其配置文件以指定某端口。

2.  客户端

1)  游戏背景图片设置。您可以不使用程序默认的游戏背景图片，在应用程序主目录放置一个
Game_bg.bmp的文件，或在配置文件中指定一个位图文件。

2)  使用客户端连接服务器，首先需要确保将要连接的服务器已经运行，并且其参数设置正确。
同时，允许您手工添加、删除、编辑服务器。

3)  Debug版本的程序允许运行多个实例，Release版本的程序将限制仅运行一个实例。


第三方软件

[win32 项目中嵌入IE]
CWebPage    http://www.codeproject.com/KB/COM/cwebpage.aspx

[GIF动画支持]
PictureEx   http://www.codeproject.com/KB/graphics/pictureex.aspx


后记

欢迎您使用本软件及其代码，有任何建议或想法都可以与作者交流：shining.ysn@gmail.com

