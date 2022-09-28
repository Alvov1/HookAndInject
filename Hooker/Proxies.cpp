#include "pch.h"
#include "Proxies.h"

typedef LONG_PTR(WINAPI* tSetWindowLongPtrA) (
    HWND     hWnd,
    int      nIndex,
    LONG_PTR dwNewLong
    );

typedef LONG_PTR(WINAPI* tSetWindowLongPtrW) (
    HWND     hWnd,
    int      nIndex,
    LONG_PTR dwNewLong
    );

tSetWindowLongPtrA pSetWindowLongPtrA;
tSetWindowLongPtrW pSetWindowLongPtrW;

void init() {
    pSetWindowLongPtrA = (tSetWindowLongPtrA)((PCHAR)SetWindowLongPtrA + 5);
    pSetWindowLongPtrW = (tSetWindowLongPtrW)((PCHAR)SetWindowLongPtrW + 5);
}

extern "C" __declspec(dllexport)
LONG_PTR OurSetWindowLongPtrA(
    HWND     hWnd,
    int      nIndex,
    LONG_PTR dwNewLong
) {
    if (nIndex != GWLP_WNDPROC)
        return pSetWindowLongPtrA(hWnd, nIndex, dwNewLong);
    return 0;
};

extern "C" __declspec(dllexport)
LONG_PTR OurSetWindowLongPtrW(
    HWND     hWnd,
    int      nIndex,
    LONG_PTR dwNewLong
) {
    if (nIndex != GWLP_WNDPROC)
        return pSetWindowLongPtrW(hWnd, nIndex, dwNewLong);
    return 0;
};