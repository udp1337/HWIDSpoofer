#pragma once
#include <ntifs.h>
#include <ntddk.h>
#include <ndis.h>

//=============================================================================
// ARP Monitor - Educational ARP Spoofing Detection Tool
// Purpose: Detect and log suspicious ARP activity in the network
// Educational Use Only - Network Security Research
//=============================================================================

#define ARP_MONITOR_TAG 'MPRA'
#define ARP_MONITOR_VERSION_MAJOR 1
#define ARP_MONITOR_VERSION_MINOR 0

//=============================================================================
// ARP Protocol Structures
//=============================================================================

#pragma pack(push, 1)

// Ethernet header
typedef struct _ETHERNET_HEADER {
    UCHAR DestMac[6];
    UCHAR SrcMac[6];
    USHORT EtherType;  // 0x0806 for ARP
} ETHERNET_HEADER, *PETHERNET_HEADER;

// ARP packet structure
typedef struct _ARP_PACKET {
    USHORT HardwareType;      // 1 = Ethernet
    USHORT ProtocolType;      // 0x0800 = IPv4
    UCHAR HardwareSize;       // 6 for MAC
    UCHAR ProtocolSize;       // 4 for IPv4
    USHORT Operation;         // 1 = Request, 2 = Reply
    UCHAR SenderMac[6];
    UCHAR SenderIP[4];
    UCHAR TargetMac[6];
    UCHAR TargetIP[4];
} ARP_PACKET, *PARP_PACKET;

#pragma pack(pop)

// ARP Operations
#define ARP_OP_REQUEST 0x0100  // Network byte order
#define ARP_OP_REPLY   0x0200

// Ethernet Type
#define ETH_TYPE_ARP   0x0608  // Network byte order (0x0806)

//=============================================================================
// ARP Cache Entry (for tracking IP-MAC bindings)
//=============================================================================

typedef struct _ARP_CACHE_ENTRY {
    LIST_ENTRY ListEntry;
    UCHAR IpAddress[4];
    UCHAR MacAddress[6];
    LARGE_INTEGER FirstSeen;
    LARGE_INTEGER LastSeen;
    ULONG PacketCount;
    BOOLEAN IsSuspicious;
    BOOLEAN IsGateway;
} ARP_CACHE_ENTRY, *PARP_CACHE_ENTRY;

//=============================================================================
// Spoofing Alert Structure
//=============================================================================

typedef struct _ARP_ALERT {
    UCHAR AttackerIP[4];
    UCHAR OldMac[6];
    UCHAR NewMac[6];
    UCHAR VictimIP[4];
    LARGE_INTEGER DetectionTime;
    ULONG Severity;  // 1-10
} ARP_ALERT, *PARP_ALERT;

#define MAX_ARP_ALERTS 100

//=============================================================================
// Statistics
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
} ARP_STATISTICS, *PARP_STATISTICS;

//=============================================================================
// Driver Context
//=============================================================================

typedef struct _ARP_MONITOR_CONTEXT {
    // Device objects
    PDEVICE_OBJECT DeviceObject;
    UNICODE_STRING DeviceName;
    UNICODE_STRING SymbolicLink;

    // NDIS Protocol Handle
    NDIS_HANDLE ProtocolHandle;
    NDIS_HANDLE BindingHandle;

    // ARP cache
    LIST_ENTRY ArpCacheHead;
    KSPIN_LOCK CacheLock;
    ULONG CacheEntryCount;

    // Alerts
    ARP_ALERT Alerts[MAX_ARP_ALERTS];
    ULONG AlertCount;
    KSPIN_LOCK AlertLock;

    // Statistics
    ARP_STATISTICS Stats;
    KSPIN_LOCK StatsLock;

    // Configuration
    BOOLEAN MonitoringEnabled;
    BOOLEAN LogAllPackets;
    UCHAR GatewayIP[4];
    UCHAR GatewayMAC[6];

    // Thresholds for detection
    ULONG MacChangeThreshold;      // ms between MAC changes
    ULONG GratuitousArpThreshold;  // Suspicious if > N per second

} ARP_MONITOR_CONTEXT, *PARP_MONITOR_CONTEXT;

//=============================================================================
// IOCTL Codes
//=============================================================================

#define IOCTL_ARP_GET_STATISTICS \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x900, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_ARP_GET_CACHE \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x901, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_ARP_GET_ALERTS \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x902, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_ARP_CLEAR_CACHE \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x903, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_ARP_ENABLE_MONITOR \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x904, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_ARP_SET_GATEWAY \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x905, METHOD_BUFFERED, FILE_WRITE_ACCESS)

//=============================================================================
// Function Prototypes
//=============================================================================

// Driver lifecycle
DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;

// Device operations
DRIVER_DISPATCH DeviceCreate;
DRIVER_DISPATCH DeviceClose;
DRIVER_DISPATCH DeviceControl;

// NDIS Protocol functions
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS RegisterProtocol(VOID);

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID UnregisterProtocol(VOID);

// ARP packet processing
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID ProcessArpPacket(
    _In_ PETHERNET_HEADER EthHeader,
    _In_ PARP_PACKET ArpPacket,
    _In_ ULONG PacketLength
);

// Cache management
_IRQL_requires_max_(DISPATCH_LEVEL)
PARP_CACHE_ENTRY FindCacheEntry(
    _In_ PUCHAR IpAddress
);

_IRQL_requires_max_(DISPATCH_LEVEL)
PARP_CACHE_ENTRY AddCacheEntry(
    _In_ PUCHAR IpAddress,
    _In_ PUCHAR MacAddress
);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID UpdateCacheEntry(
    _In_ PARP_CACHE_ENTRY Entry,
    _In_ PUCHAR MacAddress
);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID ClearCache(VOID);

// Detection functions
_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN DetectArpSpoofing(
    _In_ PARP_PACKET ArpPacket,
    _In_ PARP_CACHE_ENTRY ExistingEntry
);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID AddAlert(
    _In_ PUCHAR AttackerIP,
    _In_ PUCHAR OldMac,
    _In_ PUCHAR NewMac,
    _In_ PUCHAR VictimIP,
    _In_ ULONG Severity
);

// Utility functions
_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN CompareMAC(
    _In_ PUCHAR Mac1,
    _In_ PUCHAR Mac2
);

_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN CompareIP(
    _In_ PUCHAR Ip1,
    _In_ PUCHAR Ip2
);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID FormatMacAddress(
    _In_ PUCHAR Mac,
    _Out_writes_(18) PCHAR Buffer
);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID FormatIpAddress(
    _In_ PUCHAR Ip,
    _Out_writes_(16) PCHAR Buffer
);

//=============================================================================
// Logging Macros
//=============================================================================

#define ARP_LOG(fmt, ...) \
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, \
               "[ARPMonitor] " fmt "\n", ##__VA_ARGS__)

#define ARP_WARNING(fmt, ...) \
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, \
               "[ARPMonitor] WARNING: " fmt "\n", ##__VA_ARGS__)

#define ARP_ERROR(fmt, ...) \
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, \
               "[ARPMonitor] ERROR: " fmt "\n", ##__VA_ARGS__)

#define ARP_ALERT(fmt, ...) \
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, \
               "[ARPMonitor] *** ALERT *** " fmt "\n", ##__VA_ARGS__)

//=============================================================================
// Global Context
//=============================================================================

extern ARP_MONITOR_CONTEXT g_ArpContext;
