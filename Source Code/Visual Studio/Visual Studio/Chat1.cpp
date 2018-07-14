#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <winsock.h>
#include "Chat1.h"
#pragma comment(lib, "Ws2_32.lib")

//对话框进程
BOOL CALLBACK DlgProc(HWND hdwnd, UINT Message, WPARAM wParam, LPARAM
	lParam);
BOOL CALLBACK ConnectDlgProc(HWND hdwnd, UINT Message, WPARAM wParam, LPARAM
	lParam);
BOOL CALLBACK ListenDlgProc(HWND hdwnd, UINT Message, WPARAM wParam, LPARAM
	lParam);

int TryConnect(long hostname, int PortNo);
int ListenOnPort(int PortNo);

char Title[] = "聊天室";

HINSTANCE hInst = NULL;
HWND hwnd, hStatus;

SOCKET s;
SOCKADDR_IN from;
int fromlen = sizeof(from);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	hInst = hInstance;

	return DialogBox(hInstance, MAKEINTRESOURCE(DLG_MAIN),
		NULL, DlgProc);

}

void GetTextandAddLine(char Line[], HWND hParent, int IDC)
{
	HWND hEdit = GetDlgItem(hParent, IDC);
	int nTxtLen = GetWindowTextLength(hEdit); // 获得现有文本长度
	SendMessage(hEdit, EM_SETSEL, nTxtLen, nTxtLen);	// 将插入符号移动至末尾
	SendMessage(hEdit, EM_REPLACESEL, 0, (LPARAM)Line);	    // 追加文本
	SendMessage(hEdit, EM_SCROLLCARET, 0, 0);		// 滚动插入符号
} //函数结束   

BOOL CALLBACK DlgProc(HWND hdwnd, UINT Message, WPARAM wParam, LPARAM
	lParam)
{
	switch (Message)
	{

	case WM_INITDIALOG:
	{
		//对话框创建完成
		hwnd = hdwnd;

		hStatus = GetDlgItem(hdwnd, ID_STATUS_MAIN);
	}
	return TRUE;

	//Winsock 的相关信息...
	case 1045:
		switch (lParam)
		{
		case FD_CONNECT: //连接成功
			MessageBeep(MB_OK);
			SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)"连接成功.");
			break;

		case FD_CLOSE: //失去连接
			MessageBeep(MB_ICONERROR);

			//清理
			if (s) closesocket(s);
			WSACleanup();

			SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)"与远端主机的连接丢失.");
			break;

		case FD_READ: //等待接受的传入数据
			char buffer[80];
			memset(buffer, 0, sizeof(buffer)); //清除缓存

			recv(s, buffer, sizeof(buffer) - 1, 0); //获得文本

			GetTextandAddLine(buffer, hwnd, ID_EDIT_DATA); //展示出来
			break;

		case FD_ACCEPT: //连接请求
		{
			MessageBeep(MB_OK);

			SOCKET TempSock = accept(s, (struct sockaddr*)&from, &fromlen);
			s = TempSock; //更新Socket

			char szAcceptAddr[100];
			wsprintf(szAcceptAddr, "接受来自 [%s] 的连接请求.",
				inet_ntoa(from.sin_addr));

			SendMessage(hStatus, SB_SETTEXT, 0,
				(LPARAM)szAcceptAddr);
		}
		break;
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_BTN_CONNECT:
			return DialogBox(hInst, MAKEINTRESOURCE(DLG_CONNECT),
				NULL, ConnectDlgProc);
			break;

		case ID_BTN_LISTEN:
			return DialogBox(hInst, MAKEINTRESOURCE(DLG_LISTEN),
				NULL, ListenDlgProc);
			break;

		case ID_BTN_CLEAR: //清除编辑框并断开连接
		{
			if (s) //如果还有一个连接
			{
				int a = MessageBox(hdwnd, "你确定要断开连接吗?",
					"断开", MB_ICONQUESTION | MB_YESNO);
				if (a == IDYES)
				{
					SendDlgItemMessage(hdwnd, ID_EDIT_DATA, WM_SETTEXT, 0, (LPARAM)"");
					closesocket(s); //关闭Socket
					WSACleanup(); //清理Winsock
				}
			}
		}
		break;

		case ID_BTN_SEND: //传输数据
		{
			int len = GetWindowTextLength(GetDlgItem(hdwnd, ID_EDIT_SEND));

			if (len != 0) //如果回复框还有文本...
			{
				if (s)
				{
					char* Data;
					Data = (char*)GlobalAlloc(GPTR, len + 1);

					GetDlgItemText(hdwnd, ID_EDIT_SEND, Data, len + 1);

					//发送数据前添加回车符和换行符

					char szTemp[1000];

					wsprintf(szTemp, "%s\r\n", Data);

					send(s, szTemp, len + strlen(szTemp) + 1, 0); //传输字符串

					SetDlgItemText(hdwnd, ID_EDIT_SEND, ""); //重置文本框

					wsprintf(szTemp, "[我:] %s\r\n", Data);
					GetTextandAddLine(szTemp, hdwnd, ID_EDIT_DATA); //添加此行至文本框

					GlobalFree((HANDLE)Data); //重要：释放内存!!
				}
				else
				{
					//没有连接!!
					MessageBox(hwnd, "未检测到任何连接.",
						Title, MB_ICONERROR | MB_OK);
				}
			}
		}
		break;

		case IDCANCEL:
			//清除
			if (s) closesocket(s);
			WSACleanup();

			EndDialog(hdwnd, IDOK);
			break;
		} //End switch
	default:
		return FALSE;
		break;
	} //End Message switch
	return TRUE;
}

BOOL CALLBACK ConnectDlgProc(HWND hdwnd, UINT Message, WPARAM wParam, LPARAM
	lParam)
{
	switch (Message)
	{
	case WM_INITDIALOG:
	{
		//对话框创建完成
	}
	return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_BTN_GO:
		{
			int len = GetWindowTextLength(GetDlgItem(hdwnd, ID_EDIT_HOST));
			int lenport = GetWindowTextLength(GetDlgItem(hdwnd, ID_EDIT_PORT));

			if (!lenport) return 0; //是否指定了端口号?

			int portno = GetDlgItemInt(hdwnd, ID_EDIT_PORT, 0, 0);

			if (len)
			{
				char* Data;
				Data = (char*)GlobalAlloc(GPTR, len + 1); //申请内存

				GetDlgItemText(hdwnd, ID_EDIT_HOST, Data, len + 1); //把文本放入缓冲区

				if (!gethostbyname(Data))
				{
					//无法取得主机名; 假定这不是一个IP地址
					long hostname = inet_addr(Data);
					if (!TryConnect(hostname, portno))
					{
						MessageBox(hdwnd, "无法连接至远程主机.", Title, MB_ICONERROR | MB_OK);
						if (s) closesocket(s); //关闭Socket
					}
				}

				GlobalFree((HANDLE)Data); //释放内存

				EndDialog(hdwnd, IDOK);
			}
		}
		break;

		case IDCANCEL:
			EndDialog(hdwnd, IDOK);
			break;
		} //End switch
	default:
		return FALSE;
		break;
	} //End Message switch
	return TRUE;
}

BOOL CALLBACK ListenDlgProc(HWND hdwnd, UINT Message, WPARAM wParam, LPARAM
	lParam)
{
	switch (Message)
	{
	case WM_INITDIALOG:
	{
		//对话框创建完成
	}
	return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_BTN_GO:
		{
			int lenport = GetWindowTextLength(GetDlgItem(hdwnd, ID_EDIT_PORT));
			if (!lenport) return 0; //指定端口号了吗?

			int portno = GetDlgItemInt(hdwnd, ID_EDIT_PORT, 0, 0);

			if (!ListenOnPort(portno))
			{
				if (s) closesocket(s);
				MessageBox(hdwnd, "监听端口时发生错误.", Title, MB_ICONERROR | MB_OK);
			}
			EndDialog(hdwnd, IDOK);
		}
		break;

		case IDCANCEL:
			EndDialog(hdwnd, IDOK);
			break;
		} //End switch
	default:
		return FALSE;
		break;
	} //End Message switch
	return TRUE;
}

int TryConnect(long hostname, int PortNo)
{
	WSADATA w; //Winsock 启动信息
	SOCKADDR_IN target; //主机相关信息

	int error = WSAStartup(0x0202, &w);   // 填写WSA

	if (error)
	{ // 如果有错误
		return 0;
	}
	if (w.wVersion != 0x0202)
	{ // WinSock 版本错误
		WSACleanup(); // 卸载 ws2_32.dll
		return 0;
	}

	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // 创建Socket
	if (s == INVALID_SOCKET)
	{
		return 0;
	}

	target.sin_family = AF_INET;           // 地址族
	target.sin_port = htons(PortNo);        // 设置服务器： 端口号
	target.sin_addr.s_addr = hostname;  // 设置服务器：IP地址

										//尝试连接...
	if (connect(s, (SOCKADDR *)&target, sizeof(target)) == SOCKET_ERROR) //尝试绑定
	{ // 错误
		return 0;
	}
	WSAAsyncSelect(s, hwnd, 1045, FD_READ | FD_CONNECT | FD_CLOSE);
	//切换至非阻塞模式

	SendMessage(hStatus, WM_SETTEXT, 0, (LPARAM)"连接至远程主机.");

	return 1; //成功
}

int ListenOnPort(int PortNo)
{
	WSADATA w;

	int error = WSAStartup(0x0202, &w);   // 填写WSA

	if (error)
	{ // 若有错误
		SendMessage(hStatus, WM_SETTEXT, 0, (LPARAM)"无法初始化 Winsock.");
		return 0;
	}
	if (w.wVersion != 0x0202)
	{ // WinSock 版本错误
		WSACleanup(); // 卸载 ws2_32.dll
		SendMessage(hStatus, WM_SETTEXT, 0, (LPARAM)" WinSock 版本错误.");
		return 0;
	}

	SOCKADDR_IN addr; //  TCP socket 的地址结构
	SOCKET client; //Socket句柄

	addr.sin_family = AF_INET;      // 地址族
	addr.sin_port = htons(PortNo);   // 将分配至此Socket
	addr.sin_addr.s_addr = htonl(INADDR_ANY);   // 不限目标

	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // 创建Socket

	if (s == INVALID_SOCKET)
	{
		SendMessage(hStatus, WM_SETTEXT, 0, (LPARAM)"无法创建 socket.");
		return 0;
	}

	if (bind(s, (LPSOCKADDR)&addr, sizeof(addr)) == SOCKET_ERROR) //尝试绑定
	{ // 错误
		SendMessage(hStatus, WM_SETTEXT, 0, (LPARAM)"无法绑定至 IP.");
		return 0;
	}

	listen(s, 10); //开始监听
	WSAAsyncSelect(s, hwnd, 1045, FD_READ | FD_CONNECT | FD_CLOSE | FD_ACCEPT); //切换至非阻塞模式

	char szTemp[100];
	wsprintf(szTemp, "正在监听端口： %d...", PortNo);

	SendMessage(hStatus, WM_SETTEXT, 0, (LPARAM)szTemp);
	return 1;
}


