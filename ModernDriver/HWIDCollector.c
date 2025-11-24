#include "HVCIDriver.h"
#include <ntddscsi.h>
#include <ntdddisk.h>

//=============================================================================
// Main HWID Collection Function (HVCI-Safe)
//=============================================================================

_Use_decl_annotations_
NTSTATUS CollectSystemHWID(PSYSTEM_HWID_INFO HwidInfo)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (HwidInfo == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    LOG_INFO("Starting system HWID collection (READ-ONLY)");

    // Обнуление структуры
    RtlZeroMemory(HwidInfo, sizeof(SYSTEM_HWID_INFO));

    // Установка версии данных
    HwidInfo->DataVersion = 1;
    KeQuerySystemTime(&HwidInfo->CollectionTime);

    // Сбор информации о дисках (только чтение)
    status = CollectDiskInfo(
        HwidInfo->Disks,
        ARRAYSIZE(HwidInfo->Disks),
        &HwidInfo->DiskCount
    );

    if (!NT_SUCCESS(status)) {
        LOG_WARNING("Failed to collect disk info: 0x%08X", status);
        // Продолжаем сбор остальных данных
    } else {
        LOG_INFO("Collected info for %lu disks", HwidInfo->DiskCount);
    }

    // Сбор информации о сетевых адаптерах
    status = CollectNicInfo(
        HwidInfo->Adapters,
        ARRAYSIZE(HwidInfo->Adapters),
        &HwidInfo->AdapterCount
    );

    if (!NT_SUCCESS(status)) {
        LOG_WARNING("Failed to collect NIC info: 0x%08X", status);
    } else {
        LOG_INFO("Collected info for %lu network adapters", HwidInfo->AdapterCount);
    }

    // Сбор SMBIOS информации (только чтение!)
    status = CollectSmbiosInfo(&HwidInfo->Smbios);
    if (!NT_SUCCESS(status)) {
        LOG_WARNING("Failed to collect SMBIOS info: 0x%08X", status);
    } else {
        LOG_INFO("SMBIOS info collected");
    }

    // Проверка наличия TPM
    status = CheckTPMPresence(&HwidInfo->TpmPresent, &HwidInfo->TpmVersion);
    if (!NT_SUCCESS(status)) {
        LOG_WARNING("Failed to check TPM: 0x%08X", status);
        HwidInfo->TpmPresent = FALSE;
    } else {
        LOG_INFO("TPM present: %s (version %lu)",
                HwidInfo->TpmPresent ? "YES" : "NO",
                HwidInfo->TpmVersion);
    }

    LOG_INFO("System HWID collection completed");

    return STATUS_SUCCESS;
}

//=============================================================================
// Disk Information Collection (READ-ONLY)
//=============================================================================

_Use_decl_annotations_
NTSTATUS CollectDiskInfo(
    PDISK_HWID_INFO DiskArray,
    ULONG MaxCount,
    PULONG ActualCount
)
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG diskIndex = 0;
    WCHAR deviceNameBuffer[64];
    UNICODE_STRING deviceName;
    PFILE_OBJECT fileObject = NULL;
    PDEVICE_OBJECT deviceObject = NULL;

    if (DiskArray == NULL || ActualCount == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    *ActualCount = 0;

    LOG_INFO("Collecting disk information (max %lu devices)", MaxCount);

    // Перебор физических дисков (обычно PhysicalDrive0-7)
    for (diskIndex = 0; diskIndex < MaxCount && diskIndex < 8; diskIndex++) {
        STORAGE_PROPERTY_QUERY query;
        STORAGE_DESCRIPTOR_HEADER header;
        PSTORAGE_DEVICE_DESCRIPTOR descriptor = NULL;
        IO_STATUS_BLOCK ioStatus;
        ULONG descriptorSize = 0;
        KEVENT event;

        // Формирование имени устройства
        status = RtlStringCbPrintfW(
            deviceNameBuffer,
            sizeof(deviceNameBuffer),
            L"\\Device\\Harddisk%lu\\Partition0",
            diskIndex
        );

        if (!NT_SUCCESS(status)) {
            continue;
        }

        RtlInitUnicodeString(&deviceName, deviceNameBuffer);

        // Открытие устройства (READ-ONLY!)
        status = IoGetDeviceObjectPointer(
            &deviceName,
            FILE_READ_ACCESS,  // Только чтение!
            &fileObject,
            &deviceObject
        );

        if (!NT_SUCCESS(status)) {
            // Устройство не существует или недоступно
            continue;
        }

        LOG_INFO("Opened disk device: %wZ", &deviceName);

        // Подготовка запроса дескриптора устройства
        RtlZeroMemory(&query, sizeof(query));
        query.PropertyId = StorageDeviceProperty;
        query.QueryType = PropertyStandardQuery;

        // Первый запрос: получение размера дескриптора
        KeInitializeEvent(&event, NotificationEvent, FALSE);

        PIRP irp = IoBuildDeviceIoControlRequest(
            IOCTL_STORAGE_QUERY_PROPERTY,
            deviceObject,
            &query,
            sizeof(query),
            &header,
            sizeof(header),
            FALSE,
            &event,
            &ioStatus
        );

        if (irp == NULL) {
            ObDereferenceObject(fileObject);
            continue;
        }

        status = IoCallDriver(deviceObject, irp);
        if (status == STATUS_PENDING) {
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            status = ioStatus.Status;
        }

        if (!NT_SUCCESS(status)) {
            ObDereferenceObject(fileObject);
            continue;
        }

        descriptorSize = header.Size;
        if (descriptorSize == 0) {
            ObDereferenceObject(fileObject);
            continue;
        }

        // Выделение памяти для дескриптора (NonPagedPoolNx - важно для HVCI!)
        descriptor = (PSTORAGE_DEVICE_DESCRIPTOR)ExAllocatePoolWithTag(
            NonPagedPoolNx,  // NX = Non-Executable (требуется для HVCI)
            descriptorSize,
            DRIVER_TAG
        );

        if (descriptor == NULL) {
            ObDereferenceObject(fileObject);
            continue;
        }

        // Второй запрос: получение полного дескриптора
        KeInitializeEvent(&event, NotificationEvent, FALSE);

        irp = IoBuildDeviceIoControlRequest(
            IOCTL_STORAGE_QUERY_PROPERTY,
            deviceObject,
            &query,
            sizeof(query),
            descriptor,
            descriptorSize,
            FALSE,
            &event,
            &ioStatus
        );

        if (irp == NULL) {
            ExFreePoolWithTag(descriptor, DRIVER_TAG);
            ObDereferenceObject(fileObject);
            continue;
        }

        status = IoCallDriver(deviceObject, irp);
        if (status == STATUS_PENDING) {
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            status = ioStatus.Status;
        }

        if (NT_SUCCESS(status)) {
            PDISK_HWID_INFO diskInfo = &DiskArray[*ActualCount];

            // Заполнение информации
            SafeCopyWString(
                diskInfo->DeviceName,
                deviceNameBuffer,
                ARRAYSIZE(diskInfo->DeviceName)
            );

            // Копирование серийного номера (если есть)
            if (descriptor->SerialNumberOffset > 0) {
                PCHAR serialNumber = (PCHAR)descriptor + descriptor->SerialNumberOffset;
                SafeCopyString(
                    diskInfo->SerialNumber,
                    serialNumber,
                    sizeof(diskInfo->SerialNumber)
                );
            } else {
                RtlStringCbCopyA(diskInfo->SerialNumber,
                                sizeof(diskInfo->SerialNumber),
                                "N/A");
            }

            diskInfo->DeviceNumber = diskIndex;
            diskInfo->IsValid = TRUE;

            LOG_INFO("Disk %lu: Serial=%s", diskIndex, diskInfo->SerialNumber);

            (*ActualCount)++;
        }

        // Освобождение ресурсов
        ExFreePoolWithTag(descriptor, DRIVER_TAG);
        ObDereferenceObject(fileObject);
    }

    LOG_INFO("Collected info for %lu disks", *ActualCount);

    return STATUS_SUCCESS;
}

//=============================================================================
// Network Adapter Information Collection (READ-ONLY)
//=============================================================================

_Use_decl_annotations_
NTSTATUS CollectNicInfo(
    PNIC_HWID_INFO NicArray,
    ULONG MaxCount,
    PULONG ActualCount
)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (NicArray == NULL || ActualCount == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    *ActualCount = 0;

    LOG_INFO("Collecting network adapter information");

    // HVCI-Safe метод: использование IoGetDeviceInterfaces
    // Вместо прямого обращения к NDIS структурам

    GUID nicGuid = { 0xad498944, 0x762f, 0x11d0,
                    { 0x8d, 0xcb, 0x00, 0xc0, 0x4f, 0xc3, 0x35, 0x8c } };
    PWSTR deviceList = NULL;
    ULONG deviceListLength = 0;

    // Получение списка сетевых устройств
    status = IoGetDeviceInterfaces(
        &nicGuid,
        NULL,
        0,
        &deviceList
    );

    if (!NT_SUCCESS(status) || deviceList == NULL) {
        LOG_WARNING("Failed to get device interfaces: 0x%08X", status);
        return status;
    }

    // Парсинг списка устройств
    PWSTR currentDevice = deviceList;
    ULONG nicIndex = 0;

    while (*currentDevice != L'\0' && nicIndex < MaxCount) {
        UNICODE_STRING deviceName;
        PFILE_OBJECT fileObject = NULL;
        PDEVICE_OBJECT deviceObject = NULL;

        RtlInitUnicodeString(&deviceName, currentDevice);

        // Попытка открыть устройство (READ-ONLY)
        status = IoGetDeviceObjectPointer(
            &deviceName,
            FILE_READ_ACCESS,
            &fileObject,
            &deviceObject
        );

        if (NT_SUCCESS(status)) {
            PNIC_HWID_INFO nicInfo = &NicArray[nicIndex];

            // Сохранение имени адаптера
            SafeCopyWString(
                nicInfo->AdapterName,
                currentDevice,
                ARRAYSIZE(nicInfo->AdapterName)
            );

            // В реальном коде здесь можно сделать IOCTL запрос
            // для получения MAC адреса через стандартные API
            // Пример: OID_802_3_CURRENT_ADDRESS

            nicInfo->IsPhysical = TRUE;
            nicInfo->IsValid = TRUE;

            LOG_INFO("NIC %lu: %wZ", nicIndex, &deviceName);

            nicIndex++;
            ObDereferenceObject(fileObject);
        }

        // Переход к следующей строке в списке
        currentDevice += wcslen(currentDevice) + 1;
    }

    *ActualCount = nicIndex;

    // Освобождение памяти
    if (deviceList) {
        ExFreePool(deviceList);
    }

    LOG_INFO("Collected info for %lu network adapters", *ActualCount);

    return STATUS_SUCCESS;
}

//=============================================================================
// SMBIOS Information Collection (READ-ONLY)
//=============================================================================

_Use_decl_annotations_
NTSTATUS CollectSmbiosInfo(PSMBIOS_HWID_INFO SmbiosInfo)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (SmbiosInfo == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    RtlZeroMemory(SmbiosInfo, sizeof(SMBIOS_HWID_INFO));

    LOG_INFO("Collecting SMBIOS information (READ-ONLY)");

    // ВАЖНО: Мы НЕ модифицируем SMBIOS, только читаем!
    // В HVCI среде прямая модификация невозможна

    // Вместо прямого доступа к физической памяти,
    // используем официальное API через Registry

    HANDLE keyHandle = NULL;
    OBJECT_ATTRIBUTES objAttr;
    UNICODE_STRING keyPath = RTL_CONSTANT_STRING(
        L"\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\System\\BIOS"
    );

    InitializeObjectAttributes(
        &objAttr,
        &keyPath,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        NULL
    );

    status = ZwOpenKey(&keyHandle, KEY_READ, &objAttr);
    if (!NT_SUCCESS(status)) {
        LOG_WARNING("Failed to open BIOS registry key: 0x%08X", status);
        return status;
    }

    // Чтение значений из реестра
    struct {
        PCWSTR ValueName;
        PCHAR DestBuffer;
        SIZE_T DestSize;
    } values[] = {
        { L"SystemManufacturer", SmbiosInfo->Manufacturer, sizeof(SmbiosInfo->Manufacturer) },
        { L"SystemProductName", SmbiosInfo->ProductName, sizeof(SmbiosInfo->ProductName) },
        { L"SystemSerialNumber", SmbiosInfo->SerialNumber, sizeof(SmbiosInfo->SerialNumber) }
    };

    for (ULONG i = 0; i < ARRAYSIZE(values); i++) {
        UNICODE_STRING valueName;
        UCHAR buffer[256];
        PKEY_VALUE_PARTIAL_INFORMATION valueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;
        ULONG resultLength = 0;

        RtlInitUnicodeString(&valueName, values[i].ValueName);

        status = ZwQueryValueKey(
            keyHandle,
            &valueName,
            KeyValuePartialInformation,
            valueInfo,
            sizeof(buffer),
            &resultLength
        );

        if (NT_SUCCESS(status) && valueInfo->Type == REG_SZ) {
            // Конвертация WCHAR в CHAR (простой метод)
            PWCHAR wideStr = (PWCHAR)valueInfo->Data;
            SIZE_T len = min(wcslen(wideStr), values[i].DestSize - 1);

            for (SIZE_T j = 0; j < len; j++) {
                values[i].DestBuffer[j] = (CHAR)wideStr[j];
            }
            values[i].DestBuffer[len] = '\0';

            LOG_INFO("SMBIOS %ws: %s", values[i].ValueName, values[i].DestBuffer);
        }
    }

    ZwClose(keyHandle);

    SmbiosInfo->IsValid = TRUE;

    return STATUS_SUCCESS;
}

//=============================================================================
// TPM Presence Check
//=============================================================================

_Use_decl_annotations_
NTSTATUS CheckTPMPresence(PBOOLEAN TpmPresent, PULONG TpmVersion)
{
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE keyHandle = NULL;
    OBJECT_ATTRIBUTES objAttr;
    UNICODE_STRING keyPath = RTL_CONSTANT_STRING(
        L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Services\\TPM\\WMI\\Admin"
    );

    if (TpmPresent == NULL || TpmVersion == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    *TpmPresent = FALSE;
    *TpmVersion = 0;

    InitializeObjectAttributes(
        &objAttr,
        &keyPath,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        NULL
    );

    status = ZwOpenKey(&keyHandle, KEY_READ, &objAttr);
    if (NT_SUCCESS(status)) {
        // TPM служба существует
        *TpmPresent = TRUE;

        // Попытка определить версию TPM (упрощенно)
        UNICODE_STRING valueName = RTL_CONSTANT_STRING(L"SpecVersion");
        UCHAR buffer[256];
        PKEY_VALUE_PARTIAL_INFORMATION valueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;
        ULONG resultLength = 0;

        status = ZwQueryValueKey(
            keyHandle,
            &valueName,
            KeyValuePartialInformation,
            valueInfo,
            sizeof(buffer),
            &resultLength
        );

        if (NT_SUCCESS(status) && valueInfo->Type == REG_SZ) {
            PWCHAR specVersion = (PWCHAR)valueInfo->Data;
            // Простой парсинг: "2.0" -> версия 2
            if (specVersion[0] == L'2') {
                *TpmVersion = 2;
            } else if (specVersion[0] == L'1') {
                *TpmVersion = 1;
            }
        }

        ZwClose(keyHandle);
        LOG_INFO("TPM detected: version %lu", *TpmVersion);
    } else {
        LOG_INFO("TPM not present or inaccessible");
    }

    return STATUS_SUCCESS;
}
