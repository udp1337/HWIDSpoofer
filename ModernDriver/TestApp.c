/*
 * TestApp.c - User-mode test application for HVCI Driver
 *
 * Purpose: Demonstrates safe HWID reading from kernel driver
 *
 * Educational Use Only - Cybersecurity Research
 */

#include <windows.h>
#include <stdio.h>
#include <strsafe.h>

// Import structures from driver header (must match exactly!)
#define MAX_HWID_STRING 256
#define MAX_SERIAL_LENGTH 64

typedef struct _DISK_HWID_INFO {
    WCHAR DeviceName[MAX_HWID_STRING];
    CHAR SerialNumber[MAX_SERIAL_LENGTH];
    ULONG DeviceNumber;
    BOOLEAN IsValid;
} DISK_HWID_INFO, *PDISK_HWID_INFO;

typedef struct _NIC_HWID_INFO {
    WCHAR AdapterName[MAX_HWID_STRING];
    UCHAR MacAddress[6];
    BOOLEAN IsPhysical;
    BOOLEAN IsValid;
} NIC_HWID_INFO, *PNIC_HWID_INFO;

typedef struct _SMBIOS_HWID_INFO {
    CHAR Manufacturer[MAX_SERIAL_LENGTH];
    CHAR ProductName[MAX_SERIAL_LENGTH];
    CHAR SerialNumber[MAX_SERIAL_LENGTH];
    UCHAR UUID[16];
    BOOLEAN IsValid;
} SMBIOS_HWID_INFO, *PSMBIOS_HWID_INFO;

typedef struct _SYSTEM_HWID_INFO {
    DISK_HWID_INFO Disks[8];
    ULONG DiskCount;

    NIC_HWID_INFO Adapters[16];
    ULONG AdapterCount;

    SMBIOS_HWID_INFO Smbios;

    BOOLEAN TpmPresent;
    ULONG TpmVersion;

    LARGE_INTEGER CollectionTime;
    ULONG DataVersion;
} SYSTEM_HWID_INFO, *PSYSTEM_HWID_INFO;

// IOCTL codes (must match driver)
#define IOCTL_HVCI_GET_HWID_INFO \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_HVCI_GET_VERSION \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_HVCI_CHECK_INTEGRITY \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_READ_ACCESS)

// Function prototypes
BOOL OpenDriverHandle(HANDLE* phDevice);
void CloseDriverHandle(HANDLE hDevice);
BOOL GetDriverVersion(HANDLE hDevice);
BOOL CheckDriverIntegrity(HANDLE hDevice);
BOOL GetSystemHWID(HANDLE hDevice);
void PrintHWIDInfo(const SYSTEM_HWID_INFO* pInfo);

//=============================================================================
// Main Entry Point
//=============================================================================

int main(int argc, char* argv[])
{
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    printf("======================================================\n");
    printf(" HVCI-Compatible Driver Test Application\n");
    printf(" Educational Cybersecurity Research Tool\n");
    printf("======================================================\n\n");

    // Check if running as Administrator
    BOOL isAdmin = FALSE;
    HANDLE hToken = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION elevation;
        DWORD size = sizeof(elevation);
        if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &size)) {
            isAdmin = elevation.TokenIsElevated;
        }
        CloseHandle(hToken);
    }

    if (!isAdmin) {
        printf("[!] WARNING: Not running as Administrator\n");
        printf("[!] Driver communication may fail\n\n");
    }

    // Open driver
    if (!OpenDriverHandle(&hDevice)) {
        printf("[!] Failed to open driver\n");
        printf("[!] Make sure driver is installed and loaded\n");
        return 1;
    }

    printf("[+] Driver opened successfully\n\n");

    // Get driver version
    printf("=== Driver Version ===\n");
    GetDriverVersion(hDevice);
    printf("\n");

    // Check integrity
    printf("=== Driver Integrity Check ===\n");
    CheckDriverIntegrity(hDevice);
    printf("\n");

    // Get HWID information
    printf("=== System HWID Information ===\n");
    GetSystemHWID(hDevice);
    printf("\n");

    // Cleanup
    CloseDriverHandle(hDevice);

    printf("======================================================\n");
    printf(" Test completed. Press any key to exit...\n");
    printf("======================================================\n");

    getchar();

    return 0;
}

//=============================================================================
// Driver Communication Functions
//=============================================================================

BOOL OpenDriverHandle(HANDLE* phDevice)
{
    *phDevice = CreateFileW(
        L"\\\\.\\HVCIDriver",
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    return (*phDevice != INVALID_HANDLE_VALUE);
}

void CloseDriverHandle(HANDLE hDevice)
{
    if (hDevice != INVALID_HANDLE_VALUE) {
        CloseHandle(hDevice);
    }
}

BOOL GetDriverVersion(HANDLE hDevice)
{
    ULONG version = 0;
    DWORD bytesReturned = 0;

    BOOL result = DeviceIoControl(
        hDevice,
        IOCTL_HVCI_GET_VERSION,
        NULL,
        0,
        &version,
        sizeof(version),
        &bytesReturned,
        NULL
    );

    if (result) {
        ULONG major = (version >> 16) & 0xFFFF;
        ULONG minor = version & 0xFFFF;
        printf("[+] Driver Version: %lu.%lu\n", major, minor);
    } else {
        printf("[-] Failed to get version (error %lu)\n", GetLastError());
    }

    return result;
}

BOOL CheckDriverIntegrity(HANDLE hDevice)
{
    BOOLEAN isValid = FALSE;
    DWORD bytesReturned = 0;

    BOOL result = DeviceIoControl(
        hDevice,
        IOCTL_HVCI_CHECK_INTEGRITY,
        NULL,
        0,
        &isValid,
        sizeof(isValid),
        &bytesReturned,
        NULL
    );

    if (result) {
        printf("[+] Driver Integrity: %s\n", isValid ? "VALID" : "INVALID");
    } else {
        printf("[-] Failed to check integrity (error %lu)\n", GetLastError());
    }

    return result;
}

BOOL GetSystemHWID(HANDLE hDevice)
{
    SYSTEM_HWID_INFO hwidInfo = { 0 };
    DWORD bytesReturned = 0;

    BOOL result = DeviceIoControl(
        hDevice,
        IOCTL_HVCI_GET_HWID_INFO,
        NULL,
        0,
        &hwidInfo,
        sizeof(hwidInfo),
        &bytesReturned,
        NULL
    );

    if (result) {
        printf("[+] HWID information retrieved successfully\n");
        PrintHWIDInfo(&hwidInfo);
    } else {
        printf("[-] Failed to get HWID info (error %lu)\n", GetLastError());
    }

    return result;
}

void PrintHWIDInfo(const SYSTEM_HWID_INFO* pInfo)
{
    if (pInfo == NULL) return;

    printf("\n--- Disk Information ---\n");
    printf("Disk Count: %lu\n", pInfo->DiskCount);
    for (ULONG i = 0; i < pInfo->DiskCount; i++) {
        if (pInfo->Disks[i].IsValid) {
            printf("  Disk %lu:\n", i);
            printf("    Device: %ws\n", pInfo->Disks[i].DeviceName);
            printf("    Serial: %s\n", pInfo->Disks[i].SerialNumber);
        }
    }

    printf("\n--- Network Adapter Information ---\n");
    printf("Adapter Count: %lu\n", pInfo->AdapterCount);
    for (ULONG i = 0; i < pInfo->AdapterCount; i++) {
        if (pInfo->Adapters[i].IsValid) {
            printf("  Adapter %lu:\n", i);
            printf("    Name: %ws\n", pInfo->Adapters[i].AdapterName);
            printf("    Physical: %s\n", pInfo->Adapters[i].IsPhysical ? "Yes" : "No");
        }
    }

    printf("\n--- SMBIOS Information ---\n");
    if (pInfo->Smbios.IsValid) {
        printf("  Manufacturer: %s\n", pInfo->Smbios.Manufacturer);
        printf("  Product Name: %s\n", pInfo->Smbios.ProductName);
        printf("  Serial Number: %s\n", pInfo->Smbios.SerialNumber);

        printf("  UUID: ");
        for (int i = 0; i < 16; i++) {
            printf("%02X", pInfo->Smbios.UUID[i]);
            if (i == 3 || i == 5 || i == 7 || i == 9) printf("-");
        }
        printf("\n");
    } else {
        printf("  (Not available)\n");
    }

    printf("\n--- TPM Information ---\n");
    printf("  Present: %s\n", pInfo->TpmPresent ? "Yes" : "No");
    if (pInfo->TpmPresent) {
        printf("  Version: %lu.0\n", pInfo->TpmVersion);
    }

    printf("\n--- Metadata ---\n");
    printf("  Data Version: %lu\n", pInfo->DataVersion);
    printf("  Collection Time: %lld\n", pInfo->CollectionTime.QuadPart);
}
