#ifndef __ERRMSGBOX_H
#define __ERRMSGBOX_H

#include <windows.h>
#include <tchar.h>

static VOID ErrorMessageBox(LPCTSTR lpszMessage, ...)
{
    TCHAR szBuffer[4096];
    va_list args;

    va_start(args, lpszMessage);

    _vsntprintf_s(szBuffer, sizeof(szBuffer) / sizeof(TCHAR), _TRUNCATE,
                  lpszMessage, args);

    va_end(args);

    MessageBox(NULL, szBuffer, TEXT("Error"), MB_ICONERROR | MB_OK);
}

#endif /* __ERRMSGBOX_H */