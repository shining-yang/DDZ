#pragma once

#include "resource.h"

// 用于加载窗口类字符串
#define MAX_LOADSTRING 32


// 此代码模块中包含的函数的前向声明:
LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);

ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);

BOOL CreateChildWindows(void);
void DoPokerSelCombo(HWND hWnd, int notify);
BOOL CALLBACK EnumChildWndProc(HWND hWnd, LPARAM lParam);

void OnLogOptions(HWND hWnd, int id, int nEvent);

