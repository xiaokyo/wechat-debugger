// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "resource.h"
#include <stdio.h>
#include <atlimage.h>

// 全局变量
BYTE backCode[5] = {0};
HWND global_hDlg;

// 声明
DWORD getWechatWin();
void showPic();
void saveImg(DWORD QR,DWORD ECX);
void startHook(DWORD hookA, LPVOID funAdd);
void unHook(DWORD hookA);
INT_PTR CALLBACK dialogCallBack(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{   
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DialogBox(hModule, MAKEINTRESOURCE(MAIN), NULL, &dialogCallBack);
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}


// 消息处理程序。
INT_PTR CALLBACK dialogCallBack(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{

    global_hDlg = hDlg;

    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (wParam == HOOK) {
            // start hook wechat QRCode
            startHook(0x5717BA, showPic);
            SetDlgItemText(global_hDlg, HOOK_STATUS, "已经hook");
        }

        if (wParam == UN_HOOK) {
            // unmount hook
            unHook(0x5717BA);
            SetDlgItemText(global_hDlg, HOOK_STATUS, "已卸载hook");
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// get WeChatWin.dll ADD
DWORD getWechatWin()
{
    return (DWORD)LoadLibrary("WeChatWin.dll");
}

void saveImg(DWORD QR,DWORD qrEcx) {
    errno_t err;
    DWORD picLen = qrEcx + 0x4;
    char PicData[0xFFF] = { 0 };
    size_t cpyLen = (size_t) *((LPVOID*)picLen); // 获取地址里值 
    
    //MessageBox(global_hDlg, "hook 图片到了", "hook tips" , MB_OKCANCEL);

    memcpy(PicData, *((LPVOID*)qrEcx), cpyLen); // 拷贝文件
    FILE * pFile;

    fopen_s(&pFile, "qrcode1.png", "wb");
    fwrite(PicData, sizeof(char), sizeof(PicData), pFile);
    fclose(pFile);

    CImage img;
    CRect rect;
    HWND PicHan = GetDlgItem(global_hDlg, QR_CODE);
    GetClientRect(PicHan, &rect);

    img.Load("qrcode1.png");
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
    retAdd = getWechatWin() + 0x5717BF;

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

// start hook
void startHook(DWORD hookA, LPVOID funAdd) {
    DWORD WeAdd = getWechatWin();
    DWORD hookAdd = WeAdd + hookA;

    // jmp code
    BYTE jmpCode[5] = {0};
    jmpCode[0] = 0xE9;
    *(DWORD*)&jmpCode[1] = (DWORD)funAdd - hookAdd - 5;

    // 备份数据
    HANDLE cHandle = OpenProcess(PROCESS_ALL_ACCESS, NULL, GetCurrentProcessId());
    if (ReadProcessMemory(cHandle, (LPCVOID)hookAdd, backCode, 5, NULL) == 0)
    {
        MessageBox(NULL, "读取内存数据失败", "错误", 0);
        return;
    }

    // 写入我们组好的数据
    if (WriteProcessMemory(cHandle, (LPVOID)hookAdd, jmpCode, 5, NULL) == 0)
    {
        MessageBox(NULL, "写入内存数据失败", "错误", 0);
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
        MessageBox(NULL, "写入内存数据失败", "错误", 0);
        return;
    }
}