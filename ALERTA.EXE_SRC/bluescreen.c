#include "bluescreen.h"

#include "errmsgbox.h"

/* https://gitlab.winehq.org/wine/wine/-/blob/master/include/winternl.h */
typedef enum _HARDERROR_RESPONSE_OPTION {
    OptionAbortRetryIgnore,
    OptionOk,
    OptionOkCancel,
    OptionRetryCancel,
    OptionYesNo,
    OptionYesNoCancel,
    OptionShutdownSystem
} HARDERROR_RESPONSE_OPTION, *PHARDERROR_RESPONSE_OPTION;

/* https://gitlab.winehq.org/wine/wine/-/blob/master/include/winternl.h */
typedef enum _HARDERROR_RESPONSE {
    ResponseReturnToCaller,
    ResponseNotHandled,
    ResponseAbort,
    ResponseCancel,
    ResponseIgnore,
    ResponseNo,
    ResponseOk,
    ResponseRetry,
    ResponseYes
} HARDERROR_RESPONSE, *PHARDERROR_RESPONSE;

/* https://gitlab.winehq.org/wine/wine/-/blob/master/include/winternl.h */
typedef NTSTATUS (WINAPI *RtlAdjustPrivilege_t)(ULONG, BOOLEAN, BOOLEAN,
                                                PBOOLEAN);

/* https://gitlab.winehq.org/wine/wine/-/blob/master/include/winternl.h */
typedef NTSTATUS (WINAPI *NtRaiseHardError_t)(NTSTATUS, ULONG, ULONG,
                                              PVOID*,
                                              HARDERROR_RESPONSE_OPTION,
                                              PHARDERROR_RESPONSE);

static HMODULE GetNtDllHandle(VOID)
{
    HMODULE hNtDll = GetModuleHandle(TEXT("ntdll.dll"));

    if (hNtDll == NULL) {
        ErrorMessageBox(TEXT("Failed to load ntdll.dll"));
        return NULL;
    }

    return hNtDll;
}

static NTSTATUS WINAPI RtlAdjustPrivilege(ULONG uPrivilege, BOOLEAN bEnable,
                                          BOOLEAN bCurrentThread,
                                          PBOOLEAN pbEnabled)
{
    FARPROC fpFuncAddress;
    
    fpFuncAddress = GetProcAddress(GetNtDllHandle(), "RtlAdjustPrivilege");

    if (fpFuncAddress == NULL) {
        ErrorMessageBox(TEXT("Failed to locate RtlAdjustPrivilege"));
        return STATUS_DLL_INIT_FAILED;
    }

    return ((RtlAdjustPrivilege_t) fpFuncAddress)(uPrivilege, bEnable,
                                                  bCurrentThread, pbEnabled);
}

static NTSTATUS WINAPI NtRaiseHardError(NTSTATUS ntErrorStatus,
                                        ULONG uNumberOfParameters,
                                        ULONG uUnicodeStringParameterMask,
                                        PVOID *ppParameters,
                                        HARDERROR_RESPONSE_OPTION options,
                                        PHARDERROR_RESPONSE pResponse)
{
    FARPROC fpFuncAddress;
    
    fpFuncAddress = GetProcAddress(GetNtDllHandle(), "NtRaiseHardError");

    if (fpFuncAddress == NULL) {
        ErrorMessageBox(TEXT("Failed to locate NtRaiseHardError"));
        return STATUS_DLL_INIT_FAILED;
    }

    return ((NtRaiseHardError_t) fpFuncAddress)(ntErrorStatus,
                                                uNumberOfParameters,
                                                uUnicodeStringParameterMask,
                                                ppParameters, options,
                                                pResponse);
}

VOID BlueScreen(NTSTATUS ntErrorStatus)
{
    BOOLEAN bEnabled = FALSE;
    HARDERROR_RESPONSE response = 0;
    NTSTATUS status;

    status = RtlAdjustPrivilege(SE_SHUTDOWN_PRIVILEGE, TRUE, FALSE, &bEnabled);

    if (!NT_SUCCESS(status)) {
        ErrorMessageBox(TEXT("Failed to adjust privilege. Status: 0x%X\n"),
                        status);
        return;
    }

    status = NtRaiseHardError(ntErrorStatus, 0, 0, NULL, OptionShutdownSystem,
                              &response);

    if (!NT_SUCCESS(status)) {
        ErrorMessageBox(TEXT("Failed to raise hard error. Status: 0x%X\n"),
                        status);
        return;
    }
}