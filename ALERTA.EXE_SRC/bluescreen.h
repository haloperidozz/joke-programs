#ifndef __BLUESCREEN_H
#define __BLUESCREEN_H

#include <windows.h>
#include <ntdef.h>

VOID BlueScreen(NTSTATUS ntErrorStatus);

#endif /* __BLUESCREEN_H */