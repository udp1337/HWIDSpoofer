/*
 * ARPTest.c - User-mode test application for ARP Monitor
 *
 * Purpose: Demonstrates ARP spoofing detection capabilities
 *
 * Educational Use Only - Network Security Research
 */

#include <windows.h>
#include <stdio.h>

// Import structures from driver
typedef struct _ARP_STATISTICS {
    ULONGLONG TotalPackets;
    ULONGLONG ArpRequests;
    ULONGLONG ArpReplies;
    ULONGLONG SuspiciousPackets;
    ULONGLONG GratuitousArp;
    ULONGLONG DuplicateIP;
    ULONGLONG MacConflicts;
    ULONG ActiveAlerts;
} ARP_STATISTICS;

typedef struct _ARP_ALERT {
    UCHAR AttackerIP[4];
    UCHAR OldMac[6];
    UCHAR NewMac[6];
    UCHAR VictimIP[4];
    LARGE_INTEGER DetectionTime;
    ULONG Severity;
} ARP_ALERT;

// IOCTL codes (must match driver)
#define IOCTL_ARP_GET_STATISTICS \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x900, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_ARP_GET_ALERTS \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x902, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_ARP_CLEAR_CACHE \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x903, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_ARP_ENABLE_MONITOR \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x904, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define MAX_ALERTS 100

// Function prototypes
BOOL OpenDriver(HANDLE* phDevice);
void CloseDriver(HANDLE hDevice);
BOOL GetStatistics(HANDLE hDevice);
BOOL GetAlerts(HANDLE hDevice);
BOOL ClearCache(HANDLE hDevice);
BOOL EnableMonitoring(HANDLE hDevice, BOOL enable);
void PrintStatistics(const ARP_STATISTICS* stats);
void PrintAlert(const ARP_ALERT* alert, ULONG index);

//=============================================================================
// Main Entry Point
//=============================================================================

int main(int argc, char* argv[])
{
    HANDLE hDevice = INVALID_HANDLE_VALUE;
    BOOL running = TRUE;
    char command[32];

    printf("========================================================\n");
    printf(" ARP Monitor Test Application\n");
    printf(" Educational Network Security Tool\n");
    printf("========================================================\n\n");

    // Check admin rights
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
    if (!OpenDriver(&hDevice)) {
        printf("[!] Failed to open ARP Monitor driver\n");
        printf("[!] Make sure driver is installed and loaded\n");
        return 1;
    }

    printf("[+] ARP Monitor driver opened successfully\n\n");

    // Main command loop
    printf("Commands:\n");
    printf("  stats  - Show ARP statistics\n");
    printf("  alerts - Show detected ARP spoofing alerts\n");
    printf("  clear  - Clear ARP cache\n");
    printf("  enable - Enable monitoring\n");
    printf("  disable- Disable monitoring\n");
    printf("  help   - Show this help\n");
    printf("  quit   - Exit application\n\n");

    while (running) {
        printf("> ");
        if (fgets(command, sizeof(command), stdin) == NULL) {
            break;
        }

        // Remove newline
        command[strcspn(command, "\n")] = 0;

        if (strcmp(command, "stats") == 0) {
            GetStatistics(hDevice);
        }
        else if (strcmp(command, "alerts") == 0) {
            GetAlerts(hDevice);
        }
        else if (strcmp(command, "clear") == 0) {
            if (ClearCache(hDevice)) {
                printf("[+] ARP cache cleared\n");
            }
        }
        else if (strcmp(command, "enable") == 0) {
            if (EnableMonitoring(hDevice, TRUE)) {
                printf("[+] Monitoring enabled\n");
            }
        }
        else if (strcmp(command, "disable") == 0) {
            if (EnableMonitoring(hDevice, FALSE)) {
                printf("[+] Monitoring disabled\n");
            }
        }
        else if (strcmp(command, "help") == 0) {
            printf("Commands:\n");
            printf("  stats   - Show ARP statistics\n");
            printf("  alerts  - Show detected alerts\n");
            printf("  clear   - Clear ARP cache\n");
            printf("  enable  - Enable monitoring\n");
            printf("  disable - Disable monitoring\n");
            printf("  quit    - Exit\n");
        }
        else if (strcmp(command, "quit") == 0 || strcmp(command, "exit") == 0) {
            running = FALSE;
        }
        else if (strlen(command) > 0) {
            printf("[!] Unknown command: %s\n", command);
            printf("Type 'help' for available commands\n");
        }
    }

    CloseDriver(hDevice);

    printf("\n========================================================\n");
    printf(" ARP Monitor Test Application Exiting\n");
    printf("========================================================\n");

    return 0;
}

//=============================================================================
// Driver Communication Functions
//=============================================================================

BOOL OpenDriver(HANDLE* phDevice)
{
    *phDevice = CreateFileW(
        L"\\\\.\\ARPMonitor",
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    return (*phDevice != INVALID_HANDLE_VALUE);
}

void CloseDriver(HANDLE hDevice)
{
    if (hDevice != INVALID_HANDLE_VALUE) {
        CloseHandle(hDevice);
    }
}

BOOL GetStatistics(HANDLE hDevice)
{
    ARP_STATISTICS stats = { 0 };
    DWORD bytesReturned = 0;

    BOOL result = DeviceIoControl(
        hDevice,
        IOCTL_ARP_GET_STATISTICS,
        NULL,
        0,
        &stats,
        sizeof(stats),
        &bytesReturned,
        NULL
    );

    if (result) {
        PrintStatistics(&stats);
    } else {
        printf("[-] Failed to get statistics (error %lu)\n", GetLastError());
    }

    return result;
}

BOOL GetAlerts(HANDLE hDevice)
{
    ARP_ALERT alerts[MAX_ALERTS] = { 0 };
    DWORD bytesReturned = 0;

    BOOL result = DeviceIoControl(
        hDevice,
        IOCTL_ARP_GET_ALERTS,
        NULL,
        0,
        alerts,
        sizeof(alerts),
        &bytesReturned,
        NULL
    );

    if (result) {
        ULONG alertCount = bytesReturned / sizeof(ARP_ALERT);

        printf("\n=== ARP Spoofing Alerts ===\n");
        printf("Total alerts: %lu\n\n", alertCount);

        if (alertCount == 0) {
            printf("No alerts detected (good!)\n");
        } else {
            for (ULONG i = 0; i < alertCount; i++) {
                PrintAlert(&alerts[i], i + 1);
            }
        }
    } else {
        printf("[-] Failed to get alerts (error %lu)\n", GetLastError());
    }

    return result;
}

BOOL ClearCache(HANDLE hDevice)
{
    DWORD bytesReturned = 0;

    BOOL result = DeviceIoControl(
        hDevice,
        IOCTL_ARP_CLEAR_CACHE,
        NULL,
        0,
        NULL,
        0,
        &bytesReturned,
        NULL
    );

    if (!result) {
        printf("[-] Failed to clear cache (error %lu)\n", GetLastError());
    }

    return result;
}

BOOL EnableMonitoring(HANDLE hDevice, BOOL enable)
{
    DWORD bytesReturned = 0;

    BOOL result = DeviceIoControl(
        hDevice,
        IOCTL_ARP_ENABLE_MONITOR,
        &enable,
        sizeof(enable),
        NULL,
        0,
        &bytesReturned,
        NULL
    );

    if (!result) {
        printf("[-] Failed to change monitoring state (error %lu)\n", GetLastError());
    }

    return result;
}

//=============================================================================
// Display Functions
//=============================================================================

void PrintStatistics(const ARP_STATISTICS* stats)
{
    printf("\n=== ARP Monitor Statistics ===\n");
    printf("Total packets:       %llu\n", stats->TotalPackets);
    printf("ARP requests:        %llu\n", stats->ArpRequests);
    printf("ARP replies:         %llu\n", stats->ArpReplies);
    printf("Suspicious packets:  %llu\n", stats->SuspiciousPackets);
    printf("Gratuitous ARP:      %llu\n", stats->GratuitousArp);
    printf("MAC conflicts:       %llu\n", stats->MacConflicts);
    printf("Active alerts:       %lu\n", stats->ActiveAlerts);

    if (stats->SuspiciousPackets > 0) {
        printf("\n[!] WARNING: Suspicious activity detected!\n");
        printf("[!] Check alerts for details\n");
    }

    printf("\n");
}

void PrintAlert(const ARP_ALERT* alert, ULONG index)
{
    SYSTEMTIME st;
    FileTimeToSystemTime((FILETIME*)&alert->DetectionTime, &st);

    printf("Alert #%lu (Severity: %lu/10)\n", index, alert->Severity);
    printf("  Time: %04d-%02d-%02d %02d:%02d:%02d\n",
           st.wYear, st.wMonth, st.wDay,
           st.wHour, st.wMinute, st.wSecond);

    printf("  Attacker IP: %d.%d.%d.%d\n",
           alert->AttackerIP[0], alert->AttackerIP[1],
           alert->AttackerIP[2], alert->AttackerIP[3]);

    printf("  Old MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
           alert->OldMac[0], alert->OldMac[1], alert->OldMac[2],
           alert->OldMac[3], alert->OldMac[4], alert->OldMac[5]);

    printf("  New MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
           alert->NewMac[0], alert->NewMac[1], alert->NewMac[2],
           alert->NewMac[3], alert->NewMac[4], alert->NewMac[5]);

    printf("  Victim IP: %d.%d.%d.%d\n",
           alert->VictimIP[0], alert->VictimIP[1],
           alert->VictimIP[2], alert->VictimIP[3]);

    printf("\n");
}
