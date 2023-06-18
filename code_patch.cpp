#include <Windows.h>
#include <iostream>
#include "code_patch.h"

using namespace std;

typedef DWORD(__stdcall *MyGetFileAttributes)(LPCWSTR lpFileName);
MyGetFileAttributes originMyGetFileAttributes;
DWORD __stdcall __GetFileAttributes(LPCWSTR lpFileName) {
    cout << "__GetFileAttributes" << endl;
    return originMyGetFileAttributes(lpFileName);;
}


int main() {

    originMyGetFileAttributes =  hookCodePatch<MyGetFileAttributes>(::GetFileAttributes, __GetFileAttributes);

    ::GetFileAttributes(LR"(C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Accessories\Notepad.lnk)");

}



template<typename T>
 T hookCodePatch(T originFun, T hookFuntion)
{
    const int patchSize = 10; // ������Ҫ patch ���ֽ���
    const int pageSize = 5;
    unsigned char* patchedMemory = new unsigned char[patchSize];
    unsigned char* originFunPtr = reinterpret_cast<unsigned char*>(originFun);
    
    DWORD oldProtect;
    VirtualProtect(patchedMemory, patchSize, PAGE_EXECUTE_READWRITE, &oldProtect);

    if (originFunPtr[0] == 0xFF &&originFunPtr[1] == 0x25) {
        originFunPtr = reinterpret_cast<unsigned char*>(**(int **)(originFunPtr + 2));
    }
    if (originFunPtr[0] == 0xEB ) {
        originFunPtr = originFunPtr + originFunPtr[1] + 2;
        if (originFunPtr[0] == 0xFF && originFunPtr[1] == 0x25) {
            originFunPtr = reinterpret_cast<unsigned char*>(* (int*)(originFunPtr + 2));
        }
        if (originFunPtr[0] == 0xE9) {
            originFunPtr = originFunPtr +*(int*)(originFunPtr + 1);
        }
    }
    
    // ���� originFun ��ǰ����ֽڵ� patchedMemory
    memcpy(patchedMemory, originFunPtr, 5);

    intptr_t hookOffset = (reinterpret_cast<intptr_t>(originFunPtr) + 5) - reinterpret_cast<intptr_t>(patchedMemory + 0xA);

    // �޸� patchedMemory �ĺ�����ֽ�Ϊ��ת�� originFun + 5 ��ָ��
    patchedMemory[5] = 0xE9; // x86 ��תָ��Ĳ�����
    memcpy(patchedMemory + 6, &hookOffset, sizeof(hookOffset));
    // ������תƫ����
    hookOffset = reinterpret_cast<intptr_t>(hookFuntion) - (reinterpret_cast<intptr_t>(originFunPtr) + 5);
    
    
    // �޸� originFun ��ǰ����ֽ�Ϊ��ת�� hookFunction ��ָ��
   
    VirtualProtect(originFunPtr, pageSize, PAGE_EXECUTE_READWRITE, &oldProtect);
    originFunPtr[0] = 0xE9; // x86 ��תָ��Ĳ�����
    memcpy(originFunPtr + 1, &hookOffset, sizeof(hookOffset));
    VirtualProtect(originFunPtr, pageSize, oldProtect, &oldProtect);
    return (T)patchedMemory;
}

