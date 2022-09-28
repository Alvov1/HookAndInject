#pragma once

#include <Windows.h>

void init();

extern "C" __declspec(dllexport)
LONG_PTR OurSetWindowLongPtrA(
    HWND     hWnd,
    int      nIndex,
    LONG_PTR dwNewLong
);

extern "C" __declspec(dllexport)
LONG_PTR OurSetWindowLongPtrW(
    HWND     hWnd,
    int      nIndex,
    LONG_PTR dwNewLong
);