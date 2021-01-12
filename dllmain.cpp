// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "resource.h"
#include <stdio.h>
#include <atlimage.h>
#include <string>
#include <tchar.h>
#include <iostream>

// 全局变量
BYTE backCode[5] = {0};
HWND global_hDlg;
DWORD recvMessageCall = { 0 };
DWORD jumpBackAdd = { 0 };

// 声明
DWORD getWechatWin();
void showPic();
void saveImg(DWORD QR,DWORD ECX);
void startHook(DWORD hookAdd,DWORD recvAdd, LPVOID funAdd);
void unHook(DWORD hookA);
DWORD WINAPI ThreadProc(HMODULE hDlg);
INT_PTR CALLBACK dialogCallBack(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LPCWSTR GetMsgByAddress(DWORD memAddress);
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
            startHook(0x46560F, 0x45D5D0, messageListen);
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

void saveImg(DWORD QR,DWORD qrEcx) {
    errno_t err;
    DWORD picLen = qrEcx + 0x4;
    char PicData[0xFFF] = { 0 };
    size_t cpyLen = (size_t) *((LPVOID*)picLen); // 获取地址里值 
    
    memcpy(PicData, *((LPVOID*)qrEcx), cpyLen); // 拷贝文件
    FILE * pFile;

    fopen_s(&pFile, "qrcode.png", "wb");
    fwrite(PicData, sizeof(char), sizeof(PicData), pFile);
    fclose(pFile);

    CImage img;
    CRect rect;
    HWND PicHan = GetDlgItem(global_hDlg, QR_CODE);
    GetClientRect(PicHan, &rect);

    img.Load(L"qrcode.png");
    img.Draw(GetDC(PicHan), rect);
}

// show QRCODE
DWORD pEax = 0;
DWORD pEcx = 0;
DWORD pEdx = 0;
DWORD pEbx = 0;
DWORD pEbp = 0;
DWORD pEsp = 0;
DWORD pEsi = 0;
DWORD pEdi = 0;
DWORD retAdd = 0;
void __declspec(naked) showPic()
{
    // 备份寄存器
    _asm {
        mov pEax,eax
        mov pEcx,ecx
        mov pEdx,edx
        mov pEbx,ebx
        mov pEsp,esp
        mov pEbp,ebp
        mov pEsi,esi
        mov pEdi,edi
    }

    saveImg(pEax, pEcx);

    // 恢复寄存器
    _asm {
        mov  eax,pEax
        mov  ecx,pEcx
        mov  edx,pEdx
        mov  ebx,pEbx
        mov  esp,pEsp
        mov  ebp,pEbp
        mov  esi,pEsi
        mov  edi,pEdi
        jmp retAdd
    }
}


//读取内存中的字符串
//存储格式
//xxxxxxxx:字符串地址（memAddress）
//xxxxxxxx:字符串长度（memAddress +4）
LPCWSTR GetMsgByAddress(DWORD memAddress)
{
    //获取字符串长度
    DWORD msgLength = *(DWORD*)(memAddress + 4);
    if (msgLength == 0)
    {
        WCHAR* msg = new WCHAR[1];
        msg[0] = 0;
        return msg;
    }

    WCHAR* msg = new WCHAR[msgLength + 1];
    ZeroMemory(msg, msgLength + 1);

    //复制内容
    wmemcpy_s(msg, msgLength + 1, (WCHAR*)(*(DWORD*)memAddress), msgLength + 1);
    return msg;
}

void handleMessage(DWORD esp) {
    DWORD* msgAddress = (DWORD*)(esp + 0x8);
    //wstring wxid = GetMsgByAddress(*msgAddress + 0x40);
}

DWORD esp_i = 0;
void __declspec(naked) messageListen() {
    __asm {
        mov esp_i,esp
        call recvMessageCall

        pushad
    }

    // 处理消息
    handleMessage(esp_i);

    __asm {
        popad

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