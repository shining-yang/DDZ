//
// File: Logon.h
//
#pragma once

static void InitUserImageCombox(HWND hComboBox);
static void OnLogonDlgCommand(HWND hDlg, int nResult);
static BOOL SaveUserLogonInfo(HWND hDlg, LPCTSTR name, int nImage, BOOL bMale);
static INT_PTR CALLBACK LogonDialogProc(HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam);

// 显示登陆对话框，并将用户登陆信息保存到 lParam 所指结构
BOOL UserLogon(HINSTANCE hInstance, HWND hWndParent, LPARAM lParam);
