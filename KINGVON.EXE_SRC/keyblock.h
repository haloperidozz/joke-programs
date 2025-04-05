#ifndef __KEYBLOCK_H
#define __KEYBLOCK_H

#include <Windows.h>

BOOL KeyBlock_Block(HINSTANCE hInstance, CONST DWORD* aKeys, SHORT cKeys);

VOID KeyBlock_Release(VOID);

#endif /* __KEYBLOCK_H */