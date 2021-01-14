// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "resource.h"
#include <stdio.h>
#include <atlimage.h>
#include <string>
#include <tchar.h>
#include <iostream>

using namespace std;

// 全局变量
BYTE backCode[5] = {0};
HWND global_hDlg;
DWORD recvMessageCall = { 0 };
DWORD jumpBackAdd = { 0 };

// 声明
DWORD getWechatWin();
void startHook(DWORD hookAdd,DWORD recvAdd, LPVOID funAdd);
void unHook(DWORD hookA);
DWORD WINAPI ThreadProc(HMODULE hDlg);
INT_PTR CALLBACK dialogCallBack(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void messageListen();

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{   
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        // 创建了一个线程防止卡住主线程
        CreateThread(NULL,0, (LPTHREAD_START_ROUTINE)ThreadProc, hModule,0,NULL);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

DWORD WINAPI ThreadProc(HMODULE hDlg) {
    DialogBox(hDlg, MAKEINTRESOURCE(MAIN), NULL, &dialogCallBack);
    return FALSE;
}

// 消息处理程序。
INT_PTR CALLBACK dialogCallBack(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    global_hDlg = hDlg;

    switch (message)
    {
    case WM_INITDIALOG:
        break;
    case WM_COMMAND:
        if (wParam == HOOK_LISTEN) {
            // hook 监听消息的call
            startHook(0x3A89DB, 0x3A48B0, messageListen);
        }

        if (wParam == UNHOOK_LISTEN) {
            // 卸载监听hook按钮
            unHook(0x3A89DB);
        }

        break;
    }
    return (INT_PTR)FALSE;
}

// get WeChatWin.dll ADD
DWORD getWechatWin()
{
    return (DWORD)LoadLibrary(L"WeChatWin.dll");
}

// 处理监听获取的消息
void handleMessage(DWORD eax) {
    /*
    wxid  [eax] + 0x40
    消息内容  [eax] + 0x68
    群消息时候的 [eax] + 0x12C
    */
    wchar_t *wxid = *(wchar_t**)(*(DWORD*)eax + 0x40);
    wchar_t *msg = *(wchar_t**)(*(DWORD*)eax + 0x68);
    WCHAR wxidText[0x8192] = { 0 };
    swprintf_s(wxidText, L"wxid: %s => 消息内容:%s", wxid, msg);
    SetDlgItemText(global_hDlg, RECIEVE_INPUT, wxidText);
}

DWORD eax_i = 0;
void __declspec(naked) messageListen() {
    __asm {
        mov eax_i,eax
        pushad
    }

    // 处理消息
    handleMessage(eax_i);

    __asm {
        popad
        push eax
        call recvMessageCall
        jmp jumpBackAdd
    }

}

// start hook
void startHook(DWORD hookAddress, DWORD recvAdd, LPVOID funAdd){
    DWORD WeAdd = getWechatWin();
    DWORD hookAdd = WeAdd + hookAddress;
    recvMessageCall = WeAdd + recvAdd;
    jumpBackAdd = hookAdd + 5;

    // jmp code
    BYTE jmpCode[5] = {0};
    jmpCode[0] = 0xE9;
    *(DWORD*)&jmpCode[1] = (DWORD)funAdd - hookAdd - 5;

    // 备份数据
    HANDLE cHandle = OpenProcess(PROCESS_ALL_ACCESS, NULL, GetCurrentProcessId());
    if (ReadProcessMemory(cHandle, (LPCVOID)hookAdd, backCode, 5, NULL) == 0)
    {
        MessageBox(NULL, L"读取内存数据失败", L"错误", 0);
        return;
    }

    // 写入我们组好的数据
    if (WriteProcessMemory(cHandle, (LPVOID)hookAdd, jmpCode, 5, NULL) == 0)
    {
        MessageBox(NULL, L"写入内存数据失败", L"错误", 0);
        return;
    }
}

// unhook
void unHook(DWORD hookA) {
    DWORD WeAdd = getWechatWin();
    DWORD hookAdd = WeAdd + hookA;
    // 提升权限
    HANDLE cHandle = OpenProcess(PROCESS_ALL_ACCESS, NULL, GetCurrentProcessId());
    if (WriteProcessMemory(cHandle, (LPVOID)hookAdd, backCode, 5, NULL) == 0)
    {
        MessageBox(NULL, L"写入内存数据失败", L"错误", 0);
        return;
    }
}