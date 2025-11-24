#include "HVCIDriver.h"

//=============================================================================
// Глобальный контекст драйвера
//=============================================================================

DRIVER_CONTEXT g_DriverContext = { 0 };

//=============================================================================
// Driver Entry Point (HVCI-compatible)
//=============================================================================

_Use_decl_annotations_
NTSTATUS DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
)
{
    NTSTATUS status = STATUS_SUCCESS;
    UNICODE_STRING deviceName = RTL_CONSTANT_STRING(L"\\Device\\HVCIDriver");
    UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\DosDevices\\HVCIDriver");

    UNREFERENCED_PARAMETER(RegistryPath);

    LOG_INFO("DriverEntry called - HVCI-Compatible Driver v%d.%d",
             DRIVER_VERSION_MAJOR, DRIVER_VERSION_MINOR);

    // Инициализация глобального контекста
    RtlZeroMemory(&g_DriverContext, sizeof(DRIVER_CONTEXT));
    KeInitializeSpinLock(&g_DriverContext.DataLock);

    // Установка обработчиков
    DriverObject->DriverUnload = DriverUnload;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = DeviceCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = DeviceClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceControl;

    // Создание устройства (HVCI-safe)
    status = IoCreateDevice(
        DriverObject,
        0,  // No device extension needed
        &deviceName,
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN,  // Важно для безопасности
        FALSE,
        &g_DriverContext.DeviceObject
    );

    if (!NT_SUCCESS(status)) {
        LOG_ERROR("IoCreateDevice failed: 0x%08X", status);
        return status;
    }

    LOG_INFO("Device created successfully: %wZ", &deviceName);

    // Создание символической ссылки для user-mode доступа
    status = IoCreateSymbolicLink(&symLink, &deviceName);
    if (!NT_SUCCESS(status)) {
        LOG_ERROR("IoCreateSymbolicLink failed: 0x%08X", status);
        IoDeleteDevice(g_DriverContext.DeviceObject);
        return status;
    }

    LOG_INFO("Symbolic link created: %wZ", &symLink);

    // Сохранение имен для cleanup
    g_DriverContext.DeviceName = deviceName;
    g_DriverContext.SymbolicLink = symLink;
    g_DriverContext.IsInitialized = TRUE;

    // Установка флагов устройства
    g_DriverContext.DeviceObject->Flags |= DO_BUFFERED_IO;
    g_DriverContext.DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    LOG_INFO("Driver initialization completed successfully");

    return STATUS_SUCCESS;
}

//=============================================================================
// Driver Unload (Clean Shutdown)
//=============================================================================

_Use_decl_annotations_
VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);

    LOG_INFO("DriverUnload called");

    if (g_DriverContext.IsInitialized) {
        // Удаление символической ссылки
        IoDeleteSymbolicLink(&g_DriverContext.SymbolicLink);
        LOG_INFO("Symbolic link deleted");

        // Удаление устройства
        if (g_DriverContext.DeviceObject) {
            IoDeleteDevice(g_DriverContext.DeviceObject);
            LOG_INFO("Device deleted");
        }

        g_DriverContext.IsInitialized = FALSE;
    }

    LOG_INFO("Driver unloaded successfully");
}

//=============================================================================
// IRP Handlers
//=============================================================================

_Use_decl_annotations_
NTSTATUS DeviceCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    LOG_INFO("DeviceCreate (IRP_MJ_CREATE) called");

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS DeviceClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    LOG_INFO("DeviceClose (IRP_MJ_CLOSE) called");

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS DeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpStack = NULL;
    ULONG ioControlCode = 0;
    PVOID inputBuffer = NULL;
    PVOID outputBuffer = NULL;
    ULONG inputBufferLength = 0;
    ULONG outputBufferLength = 0;
    ULONG bytesReturned = 0;

    UNREFERENCED_PARAMETER(DeviceObject);

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    ioControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;
    inputBuffer = Irp->AssociatedIrp.SystemBuffer;
    outputBuffer = Irp->AssociatedIrp.SystemBuffer;
    inputBufferLength = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    outputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    LOG_INFO("DeviceControl called with IOCTL: 0x%08X", ioControlCode);

    switch (ioControlCode) {

    case IOCTL_HVCI_GET_HWID_INFO:
    {
        PSYSTEM_HWID_INFO hwidInfo = NULL;

        LOG_INFO("Processing IOCTL_HVCI_GET_HWID_INFO");

        // Проверка размера буфера
        if (outputBufferLength < sizeof(SYSTEM_HWID_INFO)) {
            LOG_ERROR("Output buffer too small: %lu bytes, need %lu",
                     outputBufferLength, (ULONG)sizeof(SYSTEM_HWID_INFO));
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        hwidInfo = (PSYSTEM_HWID_INFO)outputBuffer;
        if (hwidInfo == NULL) {
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        // Сбор информации о системе (READ-ONLY операции)
        status = CollectSystemHWID(hwidInfo);
        if (NT_SUCCESS(status)) {
            bytesReturned = sizeof(SYSTEM_HWID_INFO);
            LOG_INFO("HWID information collected successfully");
        } else {
            LOG_ERROR("Failed to collect HWID: 0x%08X", status);
        }
        break;
    }

    case IOCTL_HVCI_GET_VERSION:
    {
        PULONG version = NULL;

        LOG_INFO("Processing IOCTL_HVCI_GET_VERSION");

        if (outputBufferLength < sizeof(ULONG)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        version = (PULONG)outputBuffer;
        *version = (DRIVER_VERSION_MAJOR << 16) | DRIVER_VERSION_MINOR;
        bytesReturned = sizeof(ULONG);

        LOG_INFO("Version returned: %d.%d", DRIVER_VERSION_MAJOR, DRIVER_VERSION_MINOR);
        break;
    }

    case IOCTL_HVCI_CHECK_INTEGRITY:
    {
        PBOOLEAN isValid = NULL;

        LOG_INFO("Processing IOCTL_HVCI_CHECK_INTEGRITY");

        if (outputBufferLength < sizeof(BOOLEAN)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        isValid = (PBOOLEAN)outputBuffer;
        status = VerifyDriverIntegrity(isValid);

        if (NT_SUCCESS(status)) {
            bytesReturned = sizeof(BOOLEAN);
            LOG_INFO("Integrity check result: %s", *isValid ? "VALID" : "INVALID");
        }
        break;
    }

    default:
        LOG_WARNING("Unknown IOCTL code: 0x%08X", ioControlCode);
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    // Завершение IRP
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = bytesReturned;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}

//=============================================================================
// Utility Functions
//=============================================================================

_Use_decl_annotations_
VOID SafeCopyString(PCHAR Dest, PCCHAR Source, SIZE_T DestSize)
{
    SIZE_T sourceLen = 0;
    SIZE_T copyLen = 0;

    if (Dest == NULL || Source == NULL || DestSize == 0) {
        return;
    }

    // Обнуление целевого буфера
    RtlZeroMemory(Dest, DestSize);

    // Вычисление длины исходной строки (безопасно)
    sourceLen = strnlen(Source, DestSize - 1);
    copyLen = min(sourceLen, DestSize - 1);

    // Копирование
    RtlCopyMemory(Dest, Source, copyLen);
    Dest[copyLen] = '\0';
}

_Use_decl_annotations_
VOID SafeCopyWString(PWCHAR Dest, PCWCHAR Source, SIZE_T DestSize)
{
    SIZE_T sourceLen = 0;
    SIZE_T copyLen = 0;

    if (Dest == NULL || Source == NULL || DestSize == 0) {
        return;
    }

    // Обнуление целевого буфера
    RtlZeroMemory(Dest, DestSize * sizeof(WCHAR));

    // Вычисление длины исходной строки
    sourceLen = wcsnlen(Source, DestSize - 1);
    copyLen = min(sourceLen, DestSize - 1);

    // Копирование
    RtlCopyMemory(Dest, Source, copyLen * sizeof(WCHAR));
    Dest[copyLen] = L'\0';
}

//=============================================================================
// Driver Integrity Verification
//=============================================================================

_Use_decl_annotations_
NTSTATUS VerifyDriverIntegrity(PBOOLEAN IsValid)
{
    if (IsValid == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    // Простая проверка: драйвер инициализирован?
    *IsValid = g_DriverContext.IsInitialized;

    // В реальном драйвере здесь можно добавить:
    // - Проверку цифровой подписи
    // - Проверку контрольных сумм критических секций
    // - Верификацию через CI.dll API

    LOG_INFO("Driver integrity check: %s", *IsValid ? "PASS" : "FAIL");

    return STATUS_SUCCESS;
}
