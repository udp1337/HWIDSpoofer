/*
 * Main.cpp - Unified Control Panel for HWID Security Research
 *
 * Purpose: Educational demonstration of legacy vs. modern security approaches
 *
 * Components:
 *   1. Legacy Spoofer Analysis (Read-Only Demonstration)
 *   2. HVCI-Compatible Monitoring Driver
 *   3. ARP Spoofing Detection Monitor
 *
 * Educational Use Only - Cybersecurity Research
 */

#include "Common.h"
#include <windows.h>
#include <stdio.h>

//=============================================================================
// HVCI Driver Structures
//=============================================================================

typedef struct _SYSTEM_HWID_INFO {
    ULONG DiskCount;
    ULONG NicCount;
    BOOLEAN SmbiosAvailable;
    BOOLEAN GpuAvailable;
    WCHAR ComputerName[64];
} SYSTEM_HWID_INFO;

typedef struct _DISK_HWID_INFO {
    WCHAR DevicePath[260];
    CHAR SerialNumber[64];
    CHAR Model[128];
    ULONGLONG SizeInBytes;
    BOOLEAN IsRemovable;
} DISK_HWID_INFO;

typedef struct _NIC_HWID_INFO {
    WCHAR AdapterName[256];
    UCHAR MacAddress[6];
    WCHAR Description[256];
    BOOLEAN IsPhysical;
} NIC_HWID_INFO;

typedef struct _SMBIOS_HWID_INFO {
    CHAR SystemManufacturer[64];
    CHAR SystemProductName[64];
    CHAR SystemSerialNumber[64];
    CHAR SystemUUID[37];
    CHAR BiosVendor[64];
    CHAR BiosVersion[64];
    CHAR BaseboardManufacturer[64];
    CHAR BaseboardProduct[64];
    CHAR BaseboardSerialNumber[64];
} SMBIOS_HWID_INFO;

// HVCI Driver IOCTL codes
#define IOCTL_HVCI_GET_HWID_INFO \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_HVCI_GET_DISK_INFO \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_HVCI_GET_NIC_INFO \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_HVCI_GET_SMBIOS_INFO \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_READ_ACCESS)

//=============================================================================
// ARP Monitor Structures
//=============================================================================

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

// ARP Monitor IOCTL codes
#define IOCTL_ARP_GET_STATISTICS \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x900, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_ARP_GET_ALERTS \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x902, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_ARP_CLEAR_CACHE \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x903, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_ARP_ENABLE_MONITOR \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x904, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define MAX_ALERTS 100

//=============================================================================
// Component Status Tracking
//=============================================================================

COMPONENT_STATUS g_LegacyStatus = STATUS_UNKNOWN;
COMPONENT_STATUS g_HVCIStatus = STATUS_UNKNOWN;
COMPONENT_STATUS g_ARPStatus = STATUS_UNKNOWN;

HANDLE g_hHVCIDriver = INVALID_HANDLE_VALUE;
HANDLE g_hARPMonitor = INVALID_HANDLE_VALUE;

//=============================================================================
// Forward Declarations
//=============================================================================

void ShowMainMenu();
void ShowComponentStatus();
void LegacySpooferMenu();
void HVCIDriverMenu();
void ARPMonitorMenu();
void ShowComparison();

BOOL InitializeHVCIDriver();
BOOL InitializeARPMonitor();
void CleanupDrivers();

void AnalyzeLegacyTechniques();
void ShowHVCIInformation();
void CollectHWIDInfo();
void ShowDiskInfo();
void ShowNICInfo();
void ShowSMBIOSInfo();

void ShowARPStatistics();
void ShowARPAlerts();
void EnableARPMonitoring(BOOL enable);
void ClearARPCache();

//=============================================================================
// Main Entry Point
//=============================================================================

int main(int argc, char* argv[])
{
    // Show disclaimer first
    ShowDisclaimer();

    // Clear screen and show title
    PrintHeader(APP_TITLE " v" APP_VERSION);

    // Check admin rights
    CheckAdminRights();

    // Initialize components
    PrintInfo("Initializing components...");

    g_LegacyStatus = STATUS_NOT_LOADED;  // Legacy is for analysis only

    if (InitializeHVCIDriver()) {
        g_HVCIStatus = STATUS_LOADED;
        PrintSuccess("HVCI Driver connected");
    } else {
        g_HVCIStatus = STATUS_NOT_LOADED;
        PrintWarning("HVCI Driver not available");
    }

    if (InitializeARPMonitor()) {
        g_ARPStatus = STATUS_LOADED;
        PrintSuccess("ARP Monitor connected");
    } else {
        g_ARPStatus = STATUS_NOT_LOADED;
        PrintWarning("ARP Monitor not available");
    }

    printf("\n");
    PressAnyKey();

    // Main menu loop
    BOOL running = TRUE;
    while (running) {
        ShowMainMenu();

        int choice = GetMenuChoice(0, 5);

        switch (choice) {
            case 1:
                LegacySpooferMenu();
                break;

            case 2:
                HVCIDriverMenu();
                break;

            case 3:
                ARPMonitorMenu();
                break;

            case 4:
                ShowComparison();
                break;

            case 5:
                ShowComponentStatus();
                PressAnyKey();
                break;

            case 0:
                running = FALSE;
                break;
        }
    }

    // Cleanup
    CleanupDrivers();

    PrintInfo("Exiting Control Panel...");
    return 0;
}

//=============================================================================
// Main Menu
//=============================================================================

void ShowMainMenu()
{
    PrintHeader("MAIN MENU");

    printf("  %s1.%s Legacy Spoofer Analysis       [%s]\n",
           COLOR_CYAN, COLOR_RESET, GetStatusString(g_LegacyStatus));
    printf("  %s2.%s HVCI-Compatible Monitor       [%s]\n",
           COLOR_CYAN, COLOR_RESET, GetStatusString(g_HVCIStatus));
    printf("  %s3.%s ARP Spoofing Detection        [%s]\n",
           COLOR_CYAN, COLOR_RESET, GetStatusString(g_ARPStatus));
    printf("\n");
    printf("  %s4.%s View Technical Comparison\n", COLOR_YELLOW, COLOR_RESET);
    printf("  %s5.%s Component Status\n", COLOR_YELLOW, COLOR_RESET);
    printf("\n");
    printf("  %s0.%s Exit\n", COLOR_RED, COLOR_RESET);
}

//=============================================================================
// Legacy Spoofer Menu
//=============================================================================

void LegacySpooferMenu()
{
    PrintHeader("LEGACY SPOOFER ANALYSIS");

    PrintWarning("This section is for EDUCATIONAL ANALYSIS ONLY");
    PrintWarning("Legacy techniques DO NOT work on HVCI-enabled systems");
    printf("\n");

    printf("  %s1.%s Analyze Legacy Techniques\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s2.%s Show HVCI Incompatibilities\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s0.%s Back to Main Menu\n", COLOR_YELLOW, COLOR_RESET);

    int choice = GetMenuChoice(0, 2);

    switch (choice) {
        case 1:
            AnalyzeLegacyTechniques();
            break;

        case 2:
            ShowHVCIInformation();
            break;

        case 0:
            return;
    }

    PressAnyKey();
}

void AnalyzeLegacyTechniques()
{
    PrintHeader("LEGACY TECHNIQUES ANALYSIS");

    ShowComponentInfo(
        "Disk Serial Spoofing (Legacy)",
        "Modify disk serial numbers by patching kernel structures",
        "Direct memory modification of RAID_UNIT_EXTENSION structure"
    );

    printf("%s[×] HVCI Incompatibility:%s\n", COLOR_RED, COLOR_RESET);
    printf("  • Direct kernel memory writes blocked by EPT\n");
    printf("  • Pattern scanning unreliable across Windows versions\n");
    printf("  • No write protection bypass in VTL 1\n\n");

    ShowComponentInfo(
        "NIC MAC Spoofing (Legacy)",
        "Change network adapter MAC addresses",
        "IRP_MJ_DEVICE_CONTROL hooking in NDIS driver stack"
    );

    printf("%s[×] HVCI Incompatibility:%s\n", COLOR_RED, COLOR_RESET);
    printf("  • MajorFunction table modification blocked\n");
    printf("  • All code pages must be signed\n");
    printf("  • W^X policy prevents runtime code modification\n\n");

    ShowComponentInfo(
        "SMBIOS Spoofing (Legacy)",
        "Modify system BIOS information",
        "Direct physical memory access via MmMapIoSpace"
    );

    printf("%s[×] HVCI Incompatibility:%s\n", COLOR_RED, COLOR_RESET);
    printf("  • Physical memory writes blocked by hypervisor\n");
    printf("  • SMBIOS tables protected in VTL 1\n");
    printf("  • Secure Boot verifies firmware integrity\n\n");

    PrintSeparator("-", 70);
    printf("\n%sConclusion:%s These techniques are 5-7 years outdated\n",
           COLOR_BRIGHT_YELLOW, COLOR_RESET);
    printf("Modern systems with HVCI enabled block ALL of these methods.\n");
}

void ShowHVCIInformation()
{
    PrintHeader("HVCI PROTECTION MECHANISMS");

    printf("%sWhat is HVCI?%s\n", COLOR_BRIGHT_CYAN, COLOR_RESET);
    printf("Hypervisor-protected Code Integrity runs kernel integrity checks\n");
    printf("in a secure virtual trust level (VTL 1) isolated from the OS.\n\n");

    printf("%sKey Protections:%s\n", COLOR_BRIGHT_YELLOW, COLOR_RESET);
    printf("  %s[1]%s W^X (Write XOR Execute)\n", COLOR_GREEN, COLOR_RESET);
    printf("      • Memory cannot be both writable and executable\n");
    printf("      • Prevents runtime code patching\n\n");

    printf("  %s[2]%s EPT (Extended Page Tables)\n", COLOR_GREEN, COLOR_RESET);
    printf("      • Hypervisor controls actual memory permissions\n");
    printf("      • OS cannot override EPT restrictions\n\n");

    printf("  %s[3]%s Code Signing Enforcement\n", COLOR_GREEN, COLOR_RESET);
    printf("      • All kernel code must be Microsoft-signed\n");
    printf("      • Prevents loading of unsigned drivers\n\n");

    printf("  %s[4]%s Control Flow Guard (CFG)\n", COLOR_GREEN, COLOR_RESET);
    printf("      • Validates indirect function calls\n");
    printf("      • Prevents ROP/JOP exploits\n\n");

    PrintSeparator("-", 70);
    printf("\n%sImpact on Legacy Techniques:%s\n", COLOR_BRIGHT_RED, COLOR_RESET);
    printf("  × IRP Hooking: BLOCKED (W^X violation)\n");
    printf("  × Memory Patching: BLOCKED (EPT protection)\n");
    printf("  × Physical Memory Access: BLOCKED (VTL isolation)\n");
    printf("  × Pattern Scanning: Still works, but writes blocked\n");
}

//=============================================================================
// HVCI Driver Menu
//=============================================================================

void HVCIDriverMenu()
{
    if (g_HVCIStatus != STATUS_LOADED) {
        PrintHeader("HVCI DRIVER");
        PrintError("HVCI Driver is not loaded!");
        PrintInfo("Please ensure the driver is installed and started.");
        PressAnyKey();
        return;
    }

    PrintHeader("HVCI-COMPATIBLE HWID MONITOR");

    PrintInfo("This driver demonstrates HVCI-compatible techniques");
    PrintInfo("READ-ONLY monitoring without system modification");
    printf("\n");

    printf("  %s1.%s Collect System HWID Overview\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s2.%s Show Disk Information\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s3.%s Show Network Adapter Information\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s4.%s Show SMBIOS Information\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s0.%s Back to Main Menu\n", COLOR_YELLOW, COLOR_RESET);

    int choice = GetMenuChoice(0, 4);

    switch (choice) {
        case 1:
            CollectHWIDInfo();
            break;

        case 2:
            ShowDiskInfo();
            break;

        case 3:
            ShowNICInfo();
            break;

        case 4:
            ShowSMBIOSInfo();
            break;

        case 0:
            return;
    }

    PressAnyKey();
}

void CollectHWIDInfo()
{
    PrintHeader("SYSTEM HWID OVERVIEW");

    SYSTEM_HWID_INFO info = { 0 };
    DWORD bytesReturned = 0;

    if (!SendDriverIOCTL(g_hHVCIDriver, IOCTL_HVCI_GET_HWID_INFO,
                         NULL, 0, &info, sizeof(info), &bytesReturned)) {
        PrintError("Failed to retrieve HWID information");
        return;
    }

    printf("%sComputer Name:%s %S\n", COLOR_BRIGHT_CYAN, COLOR_RESET, info.ComputerName);
    printf("%sDisks Found:%s %lu\n", COLOR_BRIGHT_YELLOW, COLOR_RESET, info.DiskCount);
    printf("%sNetwork Adapters:%s %lu\n", COLOR_BRIGHT_YELLOW, COLOR_RESET, info.NicCount);
    printf("%sSMBIOS Available:%s %s\n", COLOR_BRIGHT_YELLOW, COLOR_RESET,
           info.SmbiosAvailable ? "Yes" : "No");
    printf("%sGPU Detection:%s %s\n", COLOR_BRIGHT_YELLOW, COLOR_RESET,
           info.GpuAvailable ? "Yes" : "No");

    printf("\n");
    PrintSuccess("All information retrieved using HVCI-compatible methods:");
    printf("  • Storage Query API (official Microsoft API)\n");
    printf("  • Registry access (no physical memory access)\n");
    printf("  • IoGetDeviceInterfaces (no NDIS structure walking)\n");
    printf("  • NonPagedPoolNx allocations (W^X compliant)\n");
}

void ShowDiskInfo()
{
    PrintHeader("DISK INFORMATION (HVCI-Compatible)");

    DISK_HWID_INFO disks[16] = { 0 };
    DWORD bytesReturned = 0;

    if (!SendDriverIOCTL(g_hHVCIDriver, IOCTL_HVCI_GET_DISK_INFO,
                         NULL, 0, disks, sizeof(disks), &bytesReturned)) {
        PrintError("Failed to retrieve disk information");
        return;
    }

    ULONG diskCount = bytesReturned / sizeof(DISK_HWID_INFO);

    printf("Found %s%lu%s disk(s):\n\n", COLOR_BRIGHT_GREEN, diskCount, COLOR_RESET);

    for (ULONG i = 0; i < diskCount; i++) {
        printf("%sDisk #%lu%s\n", COLOR_BRIGHT_CYAN, i + 1, COLOR_RESET);
        printf("  Device: %S\n", disks[i].DevicePath);
        printf("  Model: %s\n", disks[i].Model);
        printf("  Serial: %s%s%s\n", COLOR_BRIGHT_YELLOW, disks[i].SerialNumber, COLOR_RESET);
        printf("  Size: %.2f GB\n", (double)disks[i].SizeInBytes / (1024 * 1024 * 1024));
        printf("  Removable: %s\n", disks[i].IsRemovable ? "Yes" : "No");
        printf("\n");
    }

    PrintInfo("Method: IOCTL_STORAGE_QUERY_PROPERTY (official API)");
    PrintInfo("No direct RAID_UNIT_EXTENSION access required");
}

void ShowNICInfo()
{
    PrintHeader("NETWORK ADAPTER INFORMATION (HVCI-Compatible)");

    NIC_HWID_INFO nics[32] = { 0 };
    DWORD bytesReturned = 0;

    if (!SendDriverIOCTL(g_hHVCIDriver, IOCTL_HVCI_GET_NIC_INFO,
                         NULL, 0, nics, sizeof(nics), &bytesReturned)) {
        PrintError("Failed to retrieve NIC information");
        return;
    }

    ULONG nicCount = bytesReturned / sizeof(NIC_HWID_INFO);

    printf("Found %s%lu%s network adapter(s):\n\n", COLOR_BRIGHT_GREEN, nicCount, COLOR_RESET);

    for (ULONG i = 0; i < nicCount; i++) {
        printf("%sAdapter #%lu%s\n", COLOR_BRIGHT_CYAN, i + 1, COLOR_RESET);
        printf("  Name: %S\n", nics[i].AdapterName);
        printf("  Description: %S\n", nics[i].Description);
        printf("  MAC Address: %s%02X:%02X:%02X:%02X:%02X:%02X%s\n",
               COLOR_BRIGHT_YELLOW,
               nics[i].MacAddress[0], nics[i].MacAddress[1], nics[i].MacAddress[2],
               nics[i].MacAddress[3], nics[i].MacAddress[4], nics[i].MacAddress[5],
               COLOR_RESET);
        printf("  Physical: %s\n", nics[i].IsPhysical ? "Yes" : "No");
        printf("\n");
    }

    PrintInfo("Method: IoGetDeviceInterfaces + Registry");
    PrintInfo("No NDIS structure traversal or IRP hooking");
}

void ShowSMBIOSInfo()
{
    PrintHeader("SMBIOS INFORMATION (HVCI-Compatible)");

    SMBIOS_HWID_INFO smbios = { 0 };
    DWORD bytesReturned = 0;

    if (!SendDriverIOCTL(g_hHVCIDriver, IOCTL_HVCI_GET_SMBIOS_INFO,
                         NULL, 0, &smbios, sizeof(smbios), &bytesReturned)) {
        PrintError("Failed to retrieve SMBIOS information");
        return;
    }

    printf("%sSystem Information:%s\n", COLOR_BRIGHT_CYAN, COLOR_RESET);
    printf("  Manufacturer: %s\n", smbios.SystemManufacturer);
    printf("  Product: %s\n", smbios.SystemProductName);
    printf("  Serial: %s%s%s\n", COLOR_BRIGHT_YELLOW, smbios.SystemSerialNumber, COLOR_RESET);
    printf("  UUID: %s\n", smbios.SystemUUID);
    printf("\n");

    printf("%sBIOS Information:%s\n", COLOR_BRIGHT_CYAN, COLOR_RESET);
    printf("  Vendor: %s\n", smbios.BiosVendor);
    printf("  Version: %s\n", smbios.BiosVersion);
    printf("\n");

    printf("%sBaseboard Information:%s\n", COLOR_BRIGHT_CYAN, COLOR_RESET);
    printf("  Manufacturer: %s\n", smbios.BaseboardManufacturer);
    printf("  Product: %s\n", smbios.BaseboardProduct);
    printf("  Serial: %s%s%s\n", COLOR_BRIGHT_YELLOW, smbios.BaseboardSerialNumber, COLOR_RESET);
    printf("\n");

    PrintInfo("Method: Registry access (HARDWARE\\DESCRIPTION\\System)");
    PrintInfo("No physical memory access via MmMapIoSpace");
}

//=============================================================================
// ARP Monitor Menu
//=============================================================================

void ARPMonitorMenu()
{
    if (g_ARPStatus != STATUS_LOADED) {
        PrintHeader("ARP MONITOR");
        PrintError("ARP Monitor is not loaded!");
        PrintInfo("Please ensure the driver is installed and started.");
        PressAnyKey();
        return;
    }

    PrintHeader("ARP SPOOFING DETECTION");

    PrintInfo("Network security tool for detecting ARP spoofing attacks");
    printf("\n");

    printf("  %s1.%s Show ARP Statistics\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s2.%s Show Spoofing Alerts\n", COLOR_CYAN, COLOR_RESET);
    printf("  %s3.%s Enable Monitoring\n", COLOR_GREEN, COLOR_RESET);
    printf("  %s4.%s Disable Monitoring\n", COLOR_YELLOW, COLOR_RESET);
    printf("  %s5.%s Clear ARP Cache\n", COLOR_RED, COLOR_RESET);
    printf("  %s0.%s Back to Main Menu\n", COLOR_YELLOW, COLOR_RESET);

    int choice = GetMenuChoice(0, 5);

    switch (choice) {
        case 1:
            ShowARPStatistics();
            PressAnyKey();
            break;

        case 2:
            ShowARPAlerts();
            PressAnyKey();
            break;

        case 3:
            EnableARPMonitoring(TRUE);
            PressAnyKey();
            break;

        case 4:
            EnableARPMonitoring(FALSE);
            PressAnyKey();
            break;

        case 5:
            ClearARPCache();
            PressAnyKey();
            break;

        case 0:
            return;
    }
}

void ShowARPStatistics()
{
    PrintHeader("ARP MONITOR STATISTICS");

    ARP_STATISTICS stats = { 0 };
    DWORD bytesReturned = 0;

    if (!SendDriverIOCTL(g_hARPMonitor, IOCTL_ARP_GET_STATISTICS,
                         NULL, 0, &stats, sizeof(stats), &bytesReturned)) {
        PrintError("Failed to retrieve statistics");
        return;
    }

    printf("%sPacket Statistics:%s\n", COLOR_BRIGHT_CYAN, COLOR_RESET);
    printf("  Total packets:       %llu\n", stats.TotalPackets);
    printf("  ARP requests:        %llu\n", stats.ArpRequests);
    printf("  ARP replies:         %llu\n", stats.ArpReplies);
    printf("  Gratuitous ARP:      %llu\n", stats.GratuitousArp);
    printf("\n");

    printf("%sSecurity Status:%s\n", COLOR_BRIGHT_YELLOW, COLOR_RESET);
    printf("  Suspicious packets:  %s%llu%s\n",
           stats.SuspiciousPackets > 0 ? COLOR_BRIGHT_RED : COLOR_GREEN,
           stats.SuspiciousPackets,
           COLOR_RESET);
    printf("  MAC conflicts:       %s%llu%s\n",
           stats.MacConflicts > 0 ? COLOR_BRIGHT_RED : COLOR_GREEN,
           stats.MacConflicts,
           COLOR_RESET);
    printf("  Active alerts:       %s%lu%s\n",
           stats.ActiveAlerts > 0 ? COLOR_BRIGHT_RED : COLOR_GREEN,
           stats.ActiveAlerts,
           COLOR_RESET);

    if (stats.SuspiciousPackets > 0 || stats.ActiveAlerts > 0) {
        printf("\n");
        PrintWarning("SUSPICIOUS ACTIVITY DETECTED!");
        PrintWarning("Check alerts for details (option 2)");
    } else {
        printf("\n");
        PrintSuccess("No suspicious activity detected");
    }
}

void ShowARPAlerts()
{
    PrintHeader("ARP SPOOFING ALERTS");

    ARP_ALERT alerts[MAX_ALERTS] = { 0 };
    DWORD bytesReturned = 0;

    if (!SendDriverIOCTL(g_hARPMonitor, IOCTL_ARP_GET_ALERTS,
                         NULL, 0, alerts, sizeof(alerts), &bytesReturned)) {
        PrintError("Failed to retrieve alerts");
        return;
    }

    ULONG alertCount = bytesReturned / sizeof(ARP_ALERT);

    printf("Total alerts: %s%lu%s\n\n",
           alertCount > 0 ? COLOR_BRIGHT_RED : COLOR_GREEN,
           alertCount,
           COLOR_RESET);

    if (alertCount == 0) {
        PrintSuccess("No alerts detected - network appears secure");
        return;
    }

    for (ULONG i = 0; i < alertCount; i++) {
        SYSTEMTIME st;
        FileTimeToSystemTime((FILETIME*)&alerts[i].DetectionTime, &st);

        printf("%sAlert #%lu%s (Severity: %s%lu/10%s)\n",
               COLOR_BRIGHT_RED, i + 1, COLOR_RESET,
               COLOR_BRIGHT_YELLOW, alerts[i].Severity, COLOR_RESET);

        printf("  Time: %04d-%02d-%02d %02d:%02d:%02d\n",
               st.wYear, st.wMonth, st.wDay,
               st.wHour, st.wMinute, st.wSecond);

        printf("  Attacker IP: %s%d.%d.%d.%d%s\n",
               COLOR_RED,
               alerts[i].AttackerIP[0], alerts[i].AttackerIP[1],
               alerts[i].AttackerIP[2], alerts[i].AttackerIP[3],
               COLOR_RESET);

        printf("  Old MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
               alerts[i].OldMac[0], alerts[i].OldMac[1], alerts[i].OldMac[2],
               alerts[i].OldMac[3], alerts[i].OldMac[4], alerts[i].OldMac[5]);

        printf("  New MAC: %s%02X:%02X:%02X:%02X:%02X:%02X%s\n",
               COLOR_BRIGHT_RED,
               alerts[i].NewMac[0], alerts[i].NewMac[1], alerts[i].NewMac[2],
               alerts[i].NewMac[3], alerts[i].NewMac[4], alerts[i].NewMac[5],
               COLOR_RESET);

        printf("  Victim IP: %d.%d.%d.%d\n",
               alerts[i].VictimIP[0], alerts[i].VictimIP[1],
               alerts[i].VictimIP[2], alerts[i].VictimIP[3]);

        printf("\n");
    }

    PrintWarning("Possible ARP spoofing attack detected!");
    PrintInfo("Verify MAC address changes with network administrator");
}

void EnableARPMonitoring(BOOL enable)
{
    DWORD bytesReturned = 0;

    if (SendDriverIOCTL(g_hARPMonitor, IOCTL_ARP_ENABLE_MONITOR,
                        &enable, sizeof(enable), NULL, 0, &bytesReturned)) {
        if (enable) {
            PrintSuccess("ARP monitoring ENABLED");
            PrintInfo("Now detecting ARP spoofing attacks");
        } else {
            PrintWarning("ARP monitoring DISABLED");
        }
    } else {
        PrintError("Failed to change monitoring state");
    }
}

void ClearARPCache()
{
    PrintHeader("CLEAR ARP CACHE");

    PrintWarning("This will clear the ARP cache and reset statistics");
    printf("Are you sure? (Y/N): ");

    char response = _getch();
    printf("%c\n\n", response);

    if (response != 'Y' && response != 'y') {
        PrintInfo("Operation cancelled");
        return;
    }

    DWORD bytesReturned = 0;

    if (SendDriverIOCTL(g_hARPMonitor, IOCTL_ARP_CLEAR_CACHE,
                        NULL, 0, NULL, 0, &bytesReturned)) {
        PrintSuccess("ARP cache cleared successfully");
    } else {
        PrintError("Failed to clear cache");
    }
}

//=============================================================================
// Comparison View
//=============================================================================

void ShowComparison()
{
    PrintHeader("TECHNICAL COMPARISON: Legacy vs. Modern");

    printf("\n%s=== DISK SERIAL ACCESS ===%s\n\n", COLOR_BRIGHT_CYAN, COLOR_RESET);

    printf("%sLegacy Method:%s\n", COLOR_RED, COLOR_RESET);
    printf("  • Pattern scan for RAID_UNIT_EXTENSION\n");
    printf("  • Direct memory write to structure\n");
    printf("  • Bypasses storage stack\n");
    printf("  %s× HVCI: BLOCKED%s (EPT prevents writes)\n\n", COLOR_BRIGHT_RED, COLOR_RESET);

    printf("%sModern Method:%s\n", COLOR_GREEN, COLOR_RESET);
    printf("  • IOCTL_STORAGE_QUERY_PROPERTY API\n");
    printf("  • Official Microsoft interface\n");
    printf("  • Read-only, no modification\n");
    printf("  %s✓ HVCI: COMPATIBLE%s\n\n", COLOR_BRIGHT_GREEN, COLOR_RESET);

    PrintSeparator("-", 70);

    printf("\n%s=== NETWORK MAC ADDRESS ===%s\n\n", COLOR_BRIGHT_CYAN, COLOR_RESET);

    printf("%sLegacy Method:%s\n", COLOR_RED, COLOR_RESET);
    printf("  • Hook IRP_MJ_DEVICE_CONTROL\n");
    printf("  • Modify MajorFunction table\n");
    printf("  • Filter network OID queries\n");
    printf("  %s× HVCI: BLOCKED%s (W^X prevents code modification)\n\n", COLOR_BRIGHT_RED, COLOR_RESET);

    printf("%sModern Method:%s\n", COLOR_GREEN, COLOR_RESET);
    printf("  • IoGetDeviceInterfaces enumeration\n");
    printf("  • Registry-based MAC reading\n");
    printf("  • No hooking required\n");
    printf("  %s✓ HVCI: COMPATIBLE%s\n\n", COLOR_BRIGHT_GREEN, COLOR_RESET);

    PrintSeparator("-", 70);

    printf("\n%s=== SMBIOS DATA ===%s\n\n", COLOR_BRIGHT_CYAN, COLOR_RESET);

    printf("%sLegacy Method:%s\n", COLOR_RED, COLOR_RESET);
    printf("  • MmMapIoSpace(0xF0000, ...)\n");
    printf("  • Direct physical memory access\n");
    printf("  • Parse and modify tables\n");
    printf("  %s× HVCI: BLOCKED%s (Physical memory protected)\n\n", COLOR_BRIGHT_RED, COLOR_RESET);

    printf("%sModern Method:%s\n", COLOR_GREEN, COLOR_RESET);
    printf("  • Registry: HARDWARE\\DESCRIPTION\\System\n");
    printf("  • Firmware-populated values\n");
    printf("  • Read-only access\n");
    printf("  %s✓ HVCI: COMPATIBLE%s\n\n", COLOR_BRIGHT_GREEN, COLOR_RESET);

    PrintSeparator("=", 70);

    printf("\n%sKey Takeaways:%s\n", COLOR_BRIGHT_YELLOW, COLOR_RESET);
    printf("  1. Modern Windows requires official APIs\n");
    printf("  2. HVCI blocks all legacy hooking/patching techniques\n");
    printf("  3. Read-only monitoring is still possible\n");
    printf("  4. TPM 2.0 + Secure Boot make hardware ID spoofing obsolete\n\n");

    PressAnyKey();
}

//=============================================================================
// Component Status
//=============================================================================

void ShowComponentStatus()
{
    PrintHeader("COMPONENT STATUS");

    printf("\n");
    printf("%s[1] Legacy Spoofer%s\n", COLOR_BRIGHT_CYAN, COLOR_RESET);
    printf("    Status: %s\n", GetStatusString(g_LegacyStatus));
    printf("    Purpose: Educational analysis of outdated techniques\n");
    printf("    HVCI Compatible: %sNO%s\n", COLOR_BRIGHT_RED, COLOR_RESET);
    printf("\n");

    printf("%s[2] HVCI Driver%s\n", COLOR_BRIGHT_CYAN, COLOR_RESET);
    printf("    Status: %s\n", GetStatusString(g_HVCIStatus));
    printf("    Purpose: Modern HVCI-compatible HWID monitoring\n");
    printf("    HVCI Compatible: %sYES%s\n", COLOR_BRIGHT_GREEN, COLOR_RESET);
    if (g_HVCIStatus == STATUS_LOADED) {
        printf("    Device: \\\\.\\HVCIDriver\n");
    }
    printf("\n");

    printf("%s[3] ARP Monitor%s\n", COLOR_BRIGHT_CYAN, COLOR_RESET);
    printf("    Status: %s\n", GetStatusString(g_ARPStatus));
    printf("    Purpose: ARP spoofing detection and prevention\n");
    printf("    HVCI Compatible: %sYES%s\n", COLOR_BRIGHT_GREEN, COLOR_RESET);
    if (g_ARPStatus == STATUS_LOADED) {
        printf("    Device: \\\\.\\ARPMonitor\n");
    }
    printf("\n");

    PrintSeparator("-", 70);

    printf("\n%sSystem Information:%s\n", COLOR_BRIGHT_YELLOW, COLOR_RESET);
    printf("  Admin Rights: %s\n", IsAdmin() ? "YES" : "NO");

    // Check HVCI status via registry
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                      "SYSTEM\\CurrentControlSet\\Control\\DeviceGuard\\Scenarios\\HypervisorEnforcedCodeIntegrity",
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD enabled = 0;
        DWORD size = sizeof(enabled);
        if (RegQueryValueExA(hKey, "Enabled", NULL, NULL, (LPBYTE)&enabled, &size) == ERROR_SUCCESS) {
            printf("  HVCI Enabled: %s\n", enabled ? "YES" : "NO");
        }
        RegCloseKey(hKey);
    }
}

//=============================================================================
// Driver Initialization
//=============================================================================

BOOL InitializeHVCIDriver()
{
    if (!OpenDriver(L"\\\\.\\HVCIDriver", &g_hHVCIDriver)) {
        return FALSE;
    }

    return TRUE;
}

BOOL InitializeARPMonitor()
{
    if (!OpenDriver(L"\\\\.\\ARPMonitor", &g_hARPMonitor)) {
        return FALSE;
    }

    return TRUE;
}

void CleanupDrivers()
{
    CloseDriver(g_hHVCIDriver);
    CloseDriver(g_hARPMonitor);
}
