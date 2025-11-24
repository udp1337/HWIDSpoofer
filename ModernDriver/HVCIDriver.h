#pragma once
#include <ntifs.h>
#include <ntddk.h>
#include <wdf.h>

// Pool tag для отслеживания аллокаций
#define DRIVER_TAG 'DIVH'

// Версия драйвера
#define DRIVER_VERSION_MAJOR 1
#define DRIVER_VERSION_MINOR 0

//=============================================================================
// Структуры данных для HWID информации
//=============================================================================

// Максимальные размеры строк
#define MAX_HWID_STRING 256
#define MAX_SERIAL_LENGTH 64

// Информация о диске
typedef struct _DISK_HWID_INFO {
    WCHAR DeviceName[MAX_HWID_STRING];
    CHAR SerialNumber[MAX_SERIAL_LENGTH];
    ULONG DeviceNumber;
    BOOLEAN IsValid;
} DISK_HWID_INFO, *PDISK_HWID_INFO;

// Информация о сетевом адаптере
typedef struct _NIC_HWID_INFO {
    WCHAR AdapterName[MAX_HWID_STRING];
    UCHAR MacAddress[6];
    BOOLEAN IsPhysical;
    BOOLEAN IsValid;
} NIC_HWID_INFO, *PNIC_HWID_INFO;

// Информация SMBIOS (только чтение!)
typedef struct _SMBIOS_HWID_INFO {
    CHAR Manufacturer[MAX_SERIAL_LENGTH];
    CHAR ProductName[MAX_SERIAL_LENGTH];
    CHAR SerialNumber[MAX_SERIAL_LENGTH];
    UCHAR UUID[16];
    BOOLEAN IsValid;
} SMBIOS_HWID_INFO, *PSMBIOS_HWID_INFO;

// Общая структура HWID данных
typedef struct _SYSTEM_HWID_INFO {
    DISK_HWID_INFO Disks[8];
    ULONG DiskCount;

    NIC_HWID_INFO Adapters[16];
    ULONG AdapterCount;

    SMBIOS_HWID_INFO Smbios;

    // TPM информация (если доступен)
    BOOLEAN TpmPresent;
    ULONG TpmVersion;

    // Метаданные
    LARGE_INTEGER CollectionTime;
    ULONG DataVersion;
} SYSTEM_HWID_INFO, *PSYSTEM_HWID_INFO;

//=============================================================================
// IOCTL коды для коммуникации с user-mode
//=============================================================================

#define IOCTL_HVCI_GET_HWID_INFO \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_HVCI_GET_VERSION \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_HVCI_CHECK_INTEGRITY \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_READ_ACCESS)

//=============================================================================
// Прототипы функций
//=============================================================================

// Driver lifecycle
DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;

// Device operations
DRIVER_DISPATCH DeviceCreate;
DRIVER_DISPATCH DeviceClose;
DRIVER_DISPATCH DeviceControl;

// HWID collection functions (READ-ONLY)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS CollectSystemHWID(_Out_ PSYSTEM_HWID_INFO HwidInfo);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS CollectDiskInfo(_Out_ PDISK_HWID_INFO DiskArray, _In_ ULONG MaxCount, _Out_ PULONG ActualCount);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS CollectNicInfo(_Out_ PNIC_HWID_INFO NicArray, _In_ ULONG MaxCount, _Out_ PULONG ActualCount);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS CollectSmbiosInfo(_Out_ PSMBIOS_HWID_INFO SmbiosInfo);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS CheckTPMPresence(_Out_ PBOOLEAN TpmPresent, _Out_ PULONG TpmVersion);

// Utility functions
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID SafeCopyString(_Out_writes_(DestSize) PCHAR Dest, _In_ PCCHAR Source, _In_ SIZE_T DestSize);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID SafeCopyWString(_Out_writes_(DestSize) PWCHAR Dest, _In_ PCWCHAR Source, _In_ SIZE_T DestSize);

// Integrity checking
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS VerifyDriverIntegrity(_Out_ PBOOLEAN IsValid);

//=============================================================================
// Глобальные переменные
//=============================================================================

typedef struct _DRIVER_CONTEXT {
    PDEVICE_OBJECT DeviceObject;
    UNICODE_STRING DeviceName;
    UNICODE_STRING SymbolicLink;
    BOOLEAN IsInitialized;
    KSPIN_LOCK DataLock;
} DRIVER_CONTEXT, *PDRIVER_CONTEXT;

// Внешнее объявление (определение в .c файле)
extern DRIVER_CONTEXT g_DriverContext;

//=============================================================================
// Logging macros (безопасные для HVCI)
//=============================================================================

#define LOG_INFO(fmt, ...) \
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "[HVCI-Driver] INFO: " fmt "\n", ##__VA_ARGS__)

#define LOG_ERROR(fmt, ...) \
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[HVCI-Driver] ERROR: " fmt "\n", ##__VA_ARGS__)

#define LOG_WARNING(fmt, ...) \
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "[HVCI-Driver] WARNING: " fmt "\n", ##__VA_ARGS__)

//=============================================================================
// SAL annotations для статического анализа
//=============================================================================

// Эти аннотации помогают пройти HVCI проверки и Code Analysis

#ifndef _IRQL_requires_
#define _IRQL_requires_(irql)
#endif

#ifndef _IRQL_requires_max_
#define _IRQL_requires_max_(irql)
#endif

#ifndef _Out_
#define _Out_
#endif

#ifndef _In_
#define _In_
#endif

#ifndef _Out_writes_
#define _Out_writes_(size)
#endif

