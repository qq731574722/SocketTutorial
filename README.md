\# 在C++中使用Windows TCP Sockets

 

[Programming Windows TCP Sockets in C++ for the Beginner](https://www.codeproject.com/Articles/13071/Programming-Windows-TCP-Sockets-in-C-for-the-Begin)

 

\## 介绍

 

开始之前，我们需要include winsock.h，并链接libws2_32.a到项目中，才可以使用TCP/IP必须的API。如果无法做到这一点，请使用在运行时`LoadLibrary()`加载ws2_32.dll或类似的方法

 

本文中的所有代码都是使用“Bloodshed Dev-C ++ 4.9.8.0”编写和测试的; 但一般来说，它应该适用于任何编译器，只需要很少的修改

 

\## 线程、端口、Socket

 

“线程”是你的计算机与远端计算机之间连接的标识名称，一个线程仅连接到一个套接字

 

为了将线程连接到每台计算机，必须有一个接受对象连接到线程，这些对象被称为套接字（Socket）

 

Socket可以打开任意端口，端口有一个唯一的数字，用于区分其他线程，因为同一台计算机可以同时建立多个连接

 

| 端口 | 服务                   |

| :--- | :--------------------- |

| 7    | Ping                   |

| 13   | Time                   |

| 15   | Netstat                |

| 22   | SSH                    |

| 23   | Telnet(默认)           |

| 25   | SMTP(发邮件)           |

| 43   | Whois(查询信息)        |

| 79   | Finger(查询服务端信息) |

| 80   | HTTP(网页)             |

| 110  | POP(收邮件)            |

| 119  | NNTP                   |

| 513  | CLOGIN（用于IP欺骗）   |

 

如果要使用没有分配服务的端口，1000到6535应该都可以

 

\- IE浏览器通过80端口收发数据

\- QQ和其他即时消息软件通常使用更高的未分配端口，以免收到干扰

\- 发送电子邮件时，通过25端口与远程邮件服务器通信

\- 接受邮件时，邮件客户端使用110端口从邮件服务器检索邮件

 

IP地址是分配给网络中每台计算机的身份标识，windows下可以通过ipconfig命令来查看

 

因为网站使用数字标识不利于人的记忆，所以大佬们提出了“域名”这一解决方案，使用[www.baidu.com](www.baidu.com)这样的简易的名称代替IP地址。当我们在浏览器输入这些域名时，将通过路由器查找该域名的IP地址，一旦成功获取（或主机解析完成），浏览器就会连接服务器所在的地址

 

有两个API来完成这项任务,在建立连接之前，您很可能需要将域名转换为IP地址 - 这要求计算机必须连接到Internet

 

\```cpp

//返回域名的IP地址

DECLARE_STDCALL_P(struct hostent *) gethostbyname(const char*);

 

//将一个字符串类型的地址转换为一个IP地址

//这个函数返回一个正确字节顺序的地址，这样我们就不需要做任何转换了（见后文）  

 

unsigned long PASCAL inet_addr(const char*);

\```

 

\## 字节顺序

 

因为Intel的计算机和网络协议使用相反的字节顺序，我们必须在发送请求之前将将每个端口和IP地址转换为网络协议的字节顺序，否则会引起混乱。如果不做反转，端口25最终不会变为端口25。所以当我们与服务器通信时，必须保证我们与服务器使用相同的语言

 

幸运的是，微软提供了一些API用于更改IP或端口的字节顺序

 

\```cpp

u_long PASCAL htonl(u_long); //主机->网络 long

u_long PASCAL ntohl(u_long); //网络->主机 long

 

u_short PASCAL htons(u_short); //主机->网络 short

u_short PASCAL ntohs(u_short); //网络->主机 short

\```

 

**注意**：“主机”计算机指监听并接受连接的计算机，“网络”计算机指连接到主机的访客

 

举例来说，当我们指定我们要监听或连接的端口时，我们必须使用`htons()`函数将数字转换为网络字节顺序。当然，如果使用`inet_addr()`函数将字符串类型的IP地址转换成IP地址，那么获得的这个IP地址就是已经正确的网络字节顺序，就不需要使用`htonl()`函数了

 

\## 开启Winsock

 

使用Windows套接字的第一步是启用Winsock API，Winsock有两个版本,第二版是最新版，也是我们希望指定的版本

 

\```cpp

\#define SCK_VERSION1            0x0101

\#define SCK_VERSION2            0x0202

 

int PASCAL WSAStartup(WORD,LPWSADATA);

int PASCAL WSACleanup(void);

 

//当函数返回时，这个typedef将会写满winsock的版本信息

 

 

typedef struct WSAData

{

​    WORD      wVersion;

​    WORD      wHighVersion;

​    char      szDescription[WSADESCRIPTION_LEN+1];

​    char      szSystemStatus[WSASYS_STATUS_LEN+1];

​    unsigned short      iMaxSockets;

​    unsigned short      iMaxUdpDg;

​    char *       lpVendorInfo;

} 

WSADATA;

 

typedef WSADATA *LPWSADATA;

\```

 

我们只需要调用这两个函数，初始化Winsock时调用`WSAStartup()`，任务完成时调用`WSACleanup()`，但是在任务完成之前不要关闭Winsock，因为这样会取消程序的所有连接和正在监听的端口.

 

\## 初始化Socket

 

要填写正确的参数传递给开启Socket的函数，函数返回这个套接字的句柄。这个句柄非常方便,我们可以通过它随时操作套接字的活动

 

在程序退出前关闭所有开启的Socket是个好习惯。调用`WSACleanup()`会让所拥有套接字和连接被强行关闭，但更优雅的方法是使用`closesocket()`来关闭指定的Socket，你只需要将套接字的句柄传递给这个API

 

\```cpp

//实际比这里定义的东西多很多

//浏览winsock2.h这个头文件

 

\#define SOCK_STREAM      1

\#define SOCK_DGRAM      2

\#define SOCK_RAW      3

 

\#define AF_INET      2 

 

\#define IPPROTO_TCP      6

 

SOCKET PASCAL socket(int,int,int);

int PASCAL closesocket(SOCKET);

\```

 

创建套接字时，需要传递地址族、套接字类型和协议类型。除非你在做一些特殊的工作，通常使用`AF_INET`作为默认地址族传递，这个参数决定如何解释地址

 

实际上不仅只有一种Socket，最常见的三种包括原始套接字（Raw Sockets），流套接字（Stream Sockets）和数据报套接字（Datagram Sockets）。本文使用的是流套接字，因为我们处理TCP协议，我们指定`SOCK_STREAM`作为`socket()`的第二个参数

 

\```cpp

socket(AF_INET,SOCK_STREAM,IPPROTO_TCP)

\```

 

\## 连接远程主机 - 作为客户端

 

接下来尝试一下可以连接到远程计算机的简单程序。

 

我们需要填写有关远程主机的信息，然后将这个结构体指针传给一个神奇的函数`connect()`，这个结构体和API如下。注意`sin_zero`参数是不需要的，因此留空

 

\```cpp

struct sockaddr_in 

{

​      short      sin_family;

​      u_short      sin_port;

​      struct      in_addr sin_addr;

​      char      sin_zero[8];

};

 

int PASCAL connect(SOCKET,const struct sockaddr*,int);

\```

 

在此强烈建议手动输入所有的示例

 

\```cpp

//连接到远端主机（客户端应用）

//包含需要的头文件

//不要忘记链接libws2_32.a

 

\#include <winsock.h>

///不加这个新版vs会报错

\#pragma warning(disable : 4996)

///链接这个就不需要去找libws2_32.a这个文件了

\#pragma comment(lib, "Ws2_32.lib")

///#pragma comment(lib, "libws2_32.a")

using namespace std;

 

SOCKET s; //Socket句柄

 

//CONNECTTOHOST 连接至远端主机

bool ConnectToHost(int PortNo,const char* IPAddress)

{

​    //开启Winsock...

​    WSADATA wsadata;

​    int error = WSAStartup(0x0202, &wsadata);

 

​    //是否有异常发生？

​    if (error)

​        return false;

 

​    //是否获得正确的Winsock版本

​    if (wsadata.wVersion!=0x0202)

​    {

​        WSACleanup(); //清除Winsock

​        return false;

​    }

 

​    //填写初始化Socket必须的信息

​    SOCKADDR_IN target; //Socket地址信息

 

​    target.sin_family = AF_INET;//地址族

​    target.sin_port = htons(PortNo);//连接的端口号

​    target.sin_addr.s_addr=inet_addr(IPAddress);//目的地址

 

​    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);//创建Socket

​    if (s==INVALID_SOCKET)

​    {

​        return false;//无法创建Socket

​    }

 

​    //尝试连接...

​    if (connect(s,(SOCKADDR*)&target,sizeof(target))==SOCKET_ERROR)

​    {

​        return false;//无法连接

​    }

​    else

​    {

​        return true;//连接成功

​    }

}

 

//CLOSECONNECTION - 关闭socket并断开其所有的连接

void CloseConnection()

{

​    //如果Socket存在，则关闭

​    if (s)

​        closesocket(s);

​    WSACleanup();

}

 

\```

 

\## 接收连接 - 作为服务器

 

体验完连接至目的主机后，现在来扮演服务器的角色。为了所有远端计算机都可以连接到你，我们需要“监听”端口以等待连接请求。常用的API如下：

 

\```cpp

int PASCAL bind(SOCKET,const struct sockaddr*,int); //绑定一个socket

int PASCAL listen(SOCKET,int); //监听连接请求

 

//接受一个连接请求

SOCKET PASCAL accept(SOCKET,struct sockaddr*,int*);

\```

 

作为服务器时，你只要监听了这个端口，你就可以接收这个端口的所有请求。例如：远端计算机想要与你的计算机聊天，他将先询问你的服务器是否可以建立一个连接。为了建立连接，你的服务器必须`accept()`连接请求。

 

虽然`listen()`函数是监听端口并充当服务器的最简单方法，但它不是最理想的。我们会发现，在执行它时，程序将冻结，直至建立连接。因为`listen()`是一个“阻塞”功能（一次只能执行一个任务，并在连接挂起前都不会返回）

 

这必然是一个问题，但有一些解决方案。如果对多线程熟悉的话，我们可以将服务器代码放在一个单独的线程上，该线程启动时不会冻结整个程序，父程序不会收到阻碍。

 

但是完全没必要这么麻烦，因为你可以用“异步”Socket函数替换`listen()`。

 

在你打算监听端口前，你必须：

 

\1. 初始化Winsock（前面说过）

\2. 启用一个Socket，确保返回的是一个非0值（代表成功），这个返回值是Socket的句柄

\3. 在`SOCKADDR_IN`结构中写入必要的数据，包括地址族、端口和IP地址

\4. 使用`bind()`函数把socket绑定到IP上（如果`sin_addr`的参数`SOCKADDR_IN`使用inet_addr("0.0.0.0")或`htonl(INADD_ANY)`，你就可以绑定任意IP地址）

 

这时候，如果一切都按照计划进行，你就可以调用`listen()`监听你想的内容

 

`listen()`的第一个参数必须是已经初始化的socket句柄。当然，这个socket连接的端口是我们打算监听的端口。使用next和final函数指定与服务器通信的远程计算机的最大数量。一般情况下，除非只想使用特定的几个连接，我们只需将`SOMAXCONN`(SOcket MAX CONNection)作为参数传递给`listen()`。如果Socket已经启动并正常工作，则一切正常，在收到连接请求时，`listen()`将返回。如果同意建立连接，则调用`accept()`

 

\```cpp

\#include <Windows.h>

\#include <winsock.h>

\#pragma comment(lib, "Ws2_32.lib")

 

SOCKET s;

WSADATA w;

 

//LISTENONPORT - 监听特定的端口，等待连接或数据

bool ListenOnPort(int portno)

{

​    int error = WSAStartup(0x0202, &w);//填写WSA信息

​    if (error)

​    {

​        return false;//因为某些原因无法开启Winsock

​    }

​    if (w.wVersion!=0x0202)//Winsock版本出错？

​    {

​        WSACleanup();

​        return false;

​    }

 

​    SOCKADDR_IN addr; //TCP socket的地址结构

 

​    addr.sin_family = AF_INET; //地址族

​    addr.sin_port = htons(portno); //为socket指定端口

​    

​    //若使用了INADDR_ANY，则接受所有IP地址发出的请求

​    //你也可以传入 inet_addr('0.0.0.0') 完成一样的事

​    //如果你只想监听一个特定IP地址的请求，则指定它

​    addr.sin_addr.s_addr = htonl(INADDR_ANY);

 

​    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //创建socket

 

​    if (s==INVALID_SOCKET)

​    {

​        return false; //如果无法创建socket，就不能继续

​    }

 

​    if (bind(s, (LPSOCKADDR)&addr, sizeof(addr)) == SOCKET_ERROR)

​    {

​        //无法绑定（在我们尝试绑定一个socket多次时会发生）

​        return false;

​    }

 

​    //现在我们开始监听（使用 SOMAXCONN 参数 允许同时建立尽可能多的连接）

​    //你可以根据需要也指定任意整数

​    //这个函数不会 return ，直到建立一个连接

​    listen(s, SOMAXCONN);

 

​    //不要忘记使用CloseConnection

}

\```

 

如果你编译允许这段代码，就像前面说的，你的程序将冻结，直到发出连接请求。你可以通过尝试"telnent"连接来引发此连接请求，当然，连接将不可避免地失败，因为我们连接没有被accept()，但是因为`listen()`返回了，所以程序复活。你可以在终端键入`telnet 127.0.0.0 端口号`来发送请求

 

 

\## 异步Socket

 

使用`listen()`这样的阻塞函数实在太蠢了，来看看“异步Socket”是如何工作的

 

C++有一个其他高级语言所不具备的好处：我们在使用异步Socekt前，不必再去找父窗口的“子类”。C++已经替我们完成了，我们需要做的是把处理代码发送给消息处理程序。你会看到，异步Socket会在你发出连接请求、接受数据时发送你的程序消息。因此，它能够在后台安静地等待，而不会干扰你的父程序或影响性能，因为它只在必要的时候通讯。付出的代价真的很小，因为实际上并不需要增加代码。了解工作原理可能需要一段时间，但非常高兴你愿意花时间了解异步Socket，从长远来看，这会帮你省很多时间。

 

使用异步Socket并不需要修改原有的代码，只需在`listen()`函数后添加一行。当然，你的消息处理程序需要准备好接受以下信息：

 

\- `FD_ACCEPT`:如果你的程序是客户端（使用`connnet()`连接远程主机的一方），在发出连接请求时，你会收到此消息。在发出连接请求时，会发送下面这些消息

\- `FD_CONNECT`:表示已成功建立连接

\- `FD_READ`:我们从远程计算机取得传入的数据。稍后会学习如何处理

\- `FD_CLOSE`:远程主机断开连接，因此连接丢失

 

这些值会在消息处理程序的`lParam`参数中传送。我会在一分钟内告诉你具体位置，但首先，我们需要了解将Socket设为异步模式的API参数：

 

\```cpp

//将socket转换为非阻塞的异步socket

int PASCAL WSAAsyncSelect(SOCKET,HWND,u_int,long);

\```

 

显然，第一个参数要的是我们Socket的句柄，第二个参数要的是我们的父窗口。这是必须条件，否则消息就无法发送到正确的窗口！我们看到第三个参数传入的是一个整型，这是你要指定的唯一通知编号。当消息发送到程序处理程序时，这个编号也会被发送。因此，你可以在消息处理程序中根据编号指定要发送的通知类型。我知道你有点迷糊了，所以看看下面的源码，你应该能够明白一些：

 

\```cpp

\#define MY_MESSAGE_NOTIFICATION      1048 //自定义通知消息

 

//这是我们的消息处理程序/窗口的过程

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)

{

​    switch (message) //处理信息

​    {

​    case MY_MESSAGE_NOTIFICATION: //是否要发送消息?

​        {

​            switch (lParam) //若是，发送哪一个？

​            {

​            case FD_ACCEPT:

​                //连接请求已发送

​                break;

 

​            case FD_CONNECT:

​                //连接成功建立

​                break;

 

​            case FD_READ:

​                //准备接受数据

​                break;

 

​            case FD_CLOSE:

​                //失去连接

​                break;

​            }

​        }

​        break;

 

​        //其他的窗口信息…

 

​    default: //此消息与我们无关

​        return DefWindowProc(hwnd, message, wParam, lParam);

​    }

​    break;

}

\```

 

也不算特别难吧？现在一切准备就绪，我们应该在`ListenOnPort()`函数中的`listen()`后添加一行代码：

 

\```cpp

//socket创建完成

 

//IP地址已绑定

 

//listen()函数刚刚被调用… 

 

//将socket设置为非阻塞的异步模式

//hwnd是一个父窗口的合法句柄

//确保标志间用 或 符号连接

WSAAsyncSelect (s, hwnd, MY_MESSAGE_NOTIFICATION, (FD_ACCEPT | FD_CONNECT |

​     FD_READ | FD_CLOSE);

 

//先这样…

\```

 

\```

PS C:\Users\73157> netstat -an

 

活动连接

 

  协议   本地地址                外部地址                 状态

  TCP    0.0.0.0:135            0.0.0.0:0              LISTENING

  TCP    0.0.0.0:445            0.0.0.0:0              LISTENING

  TCP    0.0.0.0:1234           0.0.0.0:0              LISTENING

  TCP    0.0.0.0:1536           0.0.0.0:0              LISTENING

  TCP    0.0.0.0:1537           0.0.0.0:0              LISTENING

  TCP    10.200.59.149:139      0.0.0.0:0              LISTENING

  TCP    10.200.59.149:14339    14.17.41.210:80        CLOSE_WAIT

  TCP    10.200.59.149:16179    183.61.49.150:8080     ESTABLISHED

  TCP    10.200.59.149:17408    157.55.170.113:5671    ESTABLISHED

  TCP    127.0.0.1:58288        0.0.0.0:0              LISTENING

  TCP    127.0.0.1:65000        0.0.0.0:0              LISTENING

  TCP    [::]:135               [::]:0                 LISTENING

  TCP    [::]:445               [::]:0                 LISTENING

  UDP    0.0.0.0:5050           *:*

  UDP    0.0.0.0:62341          *:*

  UDP    10.200.59.149:137      *:*

  UDP    10.200.59.149:138      *:*

  UDP    127.0.0.1:64961        *:*

  UDP    127.0.0.1:65000        *:*

  UDP    [::]:500               *:*

  UDP    [::]:3702              *:*

\```

 

如果你的服务器正常工作,你应该能在“本地地址”中看到“0.0.0.0:端口#”，端口# 就是你监听的端口号，正处于监听状态。PS：如果你忘记用`htons()`转换端口号，你可能会发现打开了一个新的端口，但是和你预期的完全不同。

 

不用担心，大家都需要多次尝试才会成功，多试几次，你也可以的。（当然，如果你试了几周都没成功，那就把这个教程烧了，忘记是谁写的!）

 

\## 发送数据与接受数据

 

到这一步，你所有的所有服务器都是聋哑的,这肯定不是你想要的结果。所以，来看看如何与其他计算机进行有效的沟通。和往常一样，这几个API调用解决了问题：

 

\```cpp

//传输文字数据给远端计算机

int PASCAL send(SOCKET,const char*,int,int);

 

//从远端计算机接受数据

int PASCAL recv(SOCKET,char*,int,int); 

 

//进阶函数：允许你在多台计算机连接到同一服务器时专门与某台计算机进行通信

int PASCAL sendto(SOCKET,const char*,int,int,const struct   sockaddr*,int);

int PASCAL recvfrom(SOCKET,char*,int,int,struct sockaddr*,int*);

\```

 

如果你使用的不是异步Socket的服务器，你就得把`recv()`函数放在一个定时器函数中——不断地查看传入的数据，这至少也算一个解决方案。当然，如果你机灵地设置了异步Socekt服务器，你就只要将`recv()`代码放到消息处理程序中的`FD_READ`中就可以了。当有数据传入时，你会接到通知。还有比这更简单的吗？

 

当我们监测活动时，必须建立一个缓冲区来持有它，这样指向缓冲区的指针就会传给`recv()`。在程序返回后，文本应该放在我们的缓冲区中，等待被展示出来。源码如下：

 

\```cpp

case FD_READ:

​    {

​        char buffer[80];

​        memset(buffer, 0, sizeof(buffer)); //清除buffer

 

​        //把传入文本放入buffer

​        recv (s, buffer, sizeof(buffer)-1, 0); 

 

​        //用buffer里的文字做一些聪明的事

​        //你可以展示在文本框，或者用：

 

​        //MessageBox(hwnd, Buffer, "Captured Text…", MB_OK);

​    }

​    break;

\```

 

现在你可以接收来自远程计算机或服务器的文本了,只是服务器还没有回复，或“发送”数据至远程计算机的能力。这可能是Winsock编程中最简单的步骤了，但如果你和我一样蠢，每一步都需要教，正确使用`send()`的方法如下。

 

\```cpp

char *szpText;

 

//为文本编辑中的文本分配内存，取回文本

//(详见源码) 然后传一个指向它的指针…

 

send(s, szpText, len_of_text, 0);

\```

 

为了简洁，上面的代码片只是一个骨架，可以让你大致了解`send()`如何使用。要查看完整代码，请下载本教程附带的示例源代码。

 

进阶说明：有时候简单的`send()`和`receive()`函数无法满足你的需要。不同的计算机同时拥有多个连接（我们调用`listen()`时，我们传了`SOMAXCONN`这个参数限制了最大连接数量），并且你需要将数据发送至一台指定的计算机，而非所有计算机。如果你是个小机灵鬼，你可能发现`send()`和`receive()`的另外两个API：`sendto()`和`receivefrom()`（如果你发现了，奖一朵小红花~）。

 

这两个API允许您与任何一台远程计算机进行通信，而不用管其他连接。这些进阶函数中有一个额外的参数，用于接收指向`sockaddr_in`结构体类型的指针，你可以使用这个参数指定要远程计算机的IP地址与其通信。如果你要构建一个完整的聊天程序或是类似的东西，这是个重要的技巧。但是，除了帮你了解这些功能的基本概念以外，我还会让你自己解决这些问题。（你不觉得这话从作者口中说出来很讨厌吗？通常我们自己无迹可寻。。。但如果你真的决定要做的话，也不会让你花费太多时间。）

 

\## 后记

 

到现在，你应该对Windows Socket有一个很好的理解（或是深仇大恨），但无论如何，如果你想找一个更好的解释，请看看这个本文提供的源码。比起理论，实践能让你学到更多

 

另外，如果你复制代码，或是编译你在网上找到的别人的代码，你将无法达到你自己手动输入所有示例所获得的理解水平。我知道这很痛苦，但长远角度来说，你花的时间会为你自己省去很多麻烦

 

希望你有所收获，发布评论让我知道你对本文的看法

 

\## 许可协议

 

本文及其附带源码和文件遵循[ 署名-相同方式共享 2.5 通用 ](https://creativecommons.org/licenses/by-sa/2.5/)许可

