#include "ARPMonitor.h"

//=============================================================================
// Global Context
//=============================================================================

ARP_MONITOR_CONTEXT g_ArpContext = { 0 };

//=============================================================================
// Driver Entry Point
//=============================================================================

_Use_decl_annotations_
NTSTATUS DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
)
{
    NTSTATUS status = STATUS_SUCCESS;
    UNICODE_STRING deviceName = RTL_CONSTANT_STRING(L"\\Device\\ARPMonitor");
    UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\DosDevices\\ARPMonitor");

    UNREFERENCED_PARAMETER(RegistryPath);

    ARP_LOG("DriverEntry called - ARP Spoofing Detection v%d.%d",
            ARP_MONITOR_VERSION_MAJOR, ARP_MONITOR_VERSION_MINOR);
    ARP_LOG("Educational Network Security Tool");

    // Initialize global context
    RtlZeroMemory(&g_ArpContext, sizeof(ARP_MONITOR_CONTEXT));
    InitializeListHead(&g_ArpContext.ArpCacheHead);
    KeInitializeSpinLock(&g_ArpContext.CacheLock);
    KeInitializeSpinLock(&g_ArpContext.AlertLock);
    KeInitializeSpinLock(&g_ArpContext.StatsLock);

    // Set default configuration
    g_ArpContext.MonitoringEnabled = TRUE;
    g_ArpContext.LogAllPackets = FALSE;
    g_ArpContext.MacChangeThreshold = 5000;  // 5 seconds
    g_ArpContext.GratuitousArpThreshold = 10;

    // Set dispatch routines
    DriverObject->DriverUnload = DriverUnload;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = DeviceCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = DeviceClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceControl;

    // Create device
    status = IoCreateDevice(
        DriverObject,
        0,
        &deviceName,
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &g_ArpContext.DeviceObject
    );

    if (!NT_SUCCESS(status)) {
        ARP_ERROR("IoCreateDevice failed: 0x%08X", status);
        return status;
    }

    ARP_LOG("Device created: %wZ", &deviceName);

    // Create symbolic link
    status = IoCreateSymbolicLink(&symLink, &deviceName);
    if (!NT_SUCCESS(status)) {
        ARP_ERROR("IoCreateSymbolicLink failed: 0x%08X", status);
        IoDeleteDevice(g_ArpContext.DeviceObject);
        return status;
    }

    ARP_LOG("Symbolic link created: %wZ", &symLink);

    g_ArpContext.DeviceName = deviceName;
    g_ArpContext.SymbolicLink = symLink;

    // Set device flags
    g_ArpContext.DeviceObject->Flags |= DO_BUFFERED_IO;
    g_ArpContext.DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    // Register NDIS protocol for packet capturing
    // Note: This is a simplified version. Full implementation would need
    // proper NDIS protocol driver registration
    ARP_LOG("Note: Full NDIS protocol registration requires additional code");
    ARP_LOG("This is a demonstration framework for educational purposes");

    ARP_LOG("ARP Monitor initialized successfully");
    ARP_LOG("Ready to detect ARP spoofing attacks");

    return STATUS_SUCCESS;
}

//=============================================================================
// Driver Unload
//=============================================================================

_Use_decl_annotations_
VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);

    ARP_LOG("DriverUnload called");

    // Clean up cache
    ClearCache();

    // Delete symbolic link
    IoDeleteSymbolicLink(&g_ArpContext.SymbolicLink);
    ARP_LOG("Symbolic link deleted");

    // Delete device
    if (g_ArpContext.DeviceObject) {
        IoDeleteDevice(g_ArpContext.DeviceObject);
        ARP_LOG("Device deleted");
    }

    ARP_LOG("ARP Monitor unloaded. Final statistics:");
    ARP_LOG("  Total packets: %llu", g_ArpContext.Stats.TotalPackets);
    ARP_LOG("  Suspicious packets: %llu", g_ArpContext.Stats.SuspiciousPackets);
    ARP_LOG("  Active alerts: %lu", g_ArpContext.AlertCount);
}

//=============================================================================
// IRP Handlers
//=============================================================================

_Use_decl_annotations_
NTSTATUS DeviceCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    ARP_LOG("DeviceCreate called");

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS DeviceClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    ARP_LOG("DeviceClose called");

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
    PVOID outputBuffer = NULL;
    ULONG outputBufferLength = 0;
    ULONG bytesReturned = 0;
    KIRQL oldIrql;

    UNREFERENCED_PARAMETER(DeviceObject);

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    ioControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;
    outputBuffer = Irp->AssociatedIrp.SystemBuffer;
    outputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    ARP_LOG("DeviceControl called with IOCTL: 0x%08X", ioControlCode);

    switch (ioControlCode) {

    case IOCTL_ARP_GET_STATISTICS:
    {
        if (outputBufferLength < sizeof(ARP_STATISTICS)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        KeAcquireSpinLock(&g_ArpContext.StatsLock, &oldIrql);
        RtlCopyMemory(outputBuffer, &g_ArpContext.Stats, sizeof(ARP_STATISTICS));
        KeReleaseSpinLock(&g_ArpContext.StatsLock, oldIrql);

        bytesReturned = sizeof(ARP_STATISTICS);
        ARP_LOG("Statistics returned: %llu total packets, %llu suspicious",
                g_ArpContext.Stats.TotalPackets,
                g_ArpContext.Stats.SuspiciousPackets);
        break;
    }

    case IOCTL_ARP_GET_ALERTS:
    {
        ULONG alertsSize = sizeof(ARP_ALERT) * g_ArpContext.AlertCount;

        if (outputBufferLength < alertsSize) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        KeAcquireSpinLock(&g_ArpContext.AlertLock, &oldIrql);
        RtlCopyMemory(outputBuffer, g_ArpContext.Alerts, alertsSize);
        KeReleaseSpinLock(&g_ArpContext.AlertLock, oldIrql);

        bytesReturned = alertsSize;
        ARP_LOG("Returned %lu alerts", g_ArpContext.AlertCount);
        break;
    }

    case IOCTL_ARP_CLEAR_CACHE:
    {
        ClearCache();
        ARP_LOG("ARP cache cleared");
        break;
    }

    case IOCTL_ARP_ENABLE_MONITOR:
    {
        PBOOLEAN enable = (PBOOLEAN)outputBuffer;
        if (outputBufferLength < sizeof(BOOLEAN)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        g_ArpContext.MonitoringEnabled = *enable;
        ARP_LOG("Monitoring %s", *enable ? "ENABLED" : "DISABLED");
        break;
    }

    default:
        ARP_WARNING("Unknown IOCTL: 0x%08X", ioControlCode);
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = bytesReturned;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}

//=============================================================================
// ARP Packet Processing (Demonstration)
//=============================================================================

_Use_decl_annotations_
VOID ProcessArpPacket(
    PETHERNET_HEADER EthHeader,
    PARP_PACKET ArpPacket,
    ULONG PacketLength
)
{
    KIRQL oldIrql;
    PARP_CACHE_ENTRY cacheEntry = NULL;
    CHAR senderMac[18];
    CHAR senderIp[16];
    CHAR targetMac[18];
    CHAR targetIp[16];

    if (!g_ArpContext.MonitoringEnabled) {
        return;
    }

    if (PacketLength < sizeof(ETHERNET_HEADER) + sizeof(ARP_PACKET)) {
        return;
    }

    // Update statistics
    KeAcquireSpinLock(&g_ArpContext.StatsLock, &oldIrql);
    g_ArpContext.Stats.TotalPackets++;

    if (ArpPacket->Operation == ARP_OP_REQUEST) {
        g_ArpContext.Stats.ArpRequests++;
    } else if (ArpPacket->Operation == ARP_OP_REPLY) {
        g_ArpContext.Stats.ArpReplies++;
    }

    KeReleaseSpinLock(&g_ArpContext.StatsLock, oldIrql);

    // Format addresses for logging
    FormatMacAddress(ArpPacket->SenderMac, senderMac);
    FormatIpAddress(ArpPacket->SenderIP, senderIp);
    FormatMacAddress(ArpPacket->TargetMac, targetMac);
    FormatIpAddress(ArpPacket->TargetIP, targetIp);

    if (g_ArpContext.LogAllPackets) {
        ARP_LOG("ARP %s: %s (%s) -> %s (%s)",
                ArpPacket->Operation == ARP_OP_REQUEST ? "REQUEST" : "REPLY",
                senderIp, senderMac,
                targetIp, targetMac);
    }

    // Check for gratuitous ARP (sender IP == target IP)
    if (CompareIP(ArpPacket->SenderIP, ArpPacket->TargetIP)) {
        KeAcquireSpinLock(&g_ArpContext.StatsLock, &oldIrql);
        g_ArpContext.Stats.GratuitousArp++;
        KeReleaseSpinLock(&g_ArpContext.StatsLock, oldIrql);

        ARP_WARNING("Gratuitous ARP detected from %s (%s)",
                    senderIp, senderMac);
    }

    // Look up existing cache entry
    KeAcquireSpinLock(&g_ArpContext.CacheLock, &oldIrql);
    cacheEntry = FindCacheEntry(ArpPacket->SenderIP);

    if (cacheEntry == NULL) {
        // New IP-MAC binding
        cacheEntry = AddCacheEntry(ArpPacket->SenderIP, ArpPacket->SenderMac);
        ARP_LOG("New ARP binding: %s -> %s", senderIp, senderMac);
    } else {
        // Check for MAC address change (potential spoofing!)
        if (!CompareMAC(cacheEntry->MacAddress, ArpPacket->SenderMac)) {
            CHAR oldMac[18];
            FormatMacAddress(cacheEntry->MacAddress, oldMac);

            ARP_ALERT("ARP SPOOFING DETECTED!");
            ARP_ALERT("IP %s changed from %s to %s",
                      senderIp, oldMac, senderMac);

            // Add alert
            AddAlert(
                ArpPacket->SenderIP,
                cacheEntry->MacAddress,
                ArpPacket->SenderMac,
                ArpPacket->TargetIP,
                8  // High severity
            );

            KeAcquireSpinLock(&g_ArpContext.StatsLock, &oldIrql);
            g_ArpContext.Stats.MacConflicts++;
            g_ArpContext.Stats.SuspiciousPackets++;
            KeReleaseSpinLock(&g_ArpContext.StatsLock, oldIrql);

            // Update cache with new MAC
            UpdateCacheEntry(cacheEntry, ArpPacket->SenderMac);
            cacheEntry->IsSuspicious = TRUE;
        } else {
            // Same MAC, just update timestamp
            KeQuerySystemTime(&cacheEntry->LastSeen);
            cacheEntry->PacketCount++;
        }
    }

    KeReleaseSpinLock(&g_ArpContext.CacheLock, oldIrql);
}

//=============================================================================
// Cache Management
//=============================================================================

_Use_decl_annotations_
PARP_CACHE_ENTRY FindCacheEntry(PUCHAR IpAddress)
{
    PLIST_ENTRY entry = NULL;
    PARP_CACHE_ENTRY cacheEntry = NULL;

    // Caller must hold CacheLock!

    for (entry = g_ArpContext.ArpCacheHead.Flink;
         entry != &g_ArpContext.ArpCacheHead;
         entry = entry->Flink)
    {
        cacheEntry = CONTAINING_RECORD(entry, ARP_CACHE_ENTRY, ListEntry);

        if (CompareIP(cacheEntry->IpAddress, IpAddress)) {
            return cacheEntry;
        }
    }

    return NULL;
}

_Use_decl_annotations_
PARP_CACHE_ENTRY AddCacheEntry(PUCHAR IpAddress, PUCHAR MacAddress)
{
    PARP_CACHE_ENTRY newEntry = NULL;

    // Caller must hold CacheLock!

    newEntry = (PARP_CACHE_ENTRY)ExAllocatePoolWithTag(
        NonPagedPoolNx,
        sizeof(ARP_CACHE_ENTRY),
        ARP_MONITOR_TAG
    );

    if (newEntry == NULL) {
        return NULL;
    }

    RtlZeroMemory(newEntry, sizeof(ARP_CACHE_ENTRY));

    RtlCopyMemory(newEntry->IpAddress, IpAddress, 4);
    RtlCopyMemory(newEntry->MacAddress, MacAddress, 6);

    KeQuerySystemTime(&newEntry->FirstSeen);
    newEntry->LastSeen = newEntry->FirstSeen;
    newEntry->PacketCount = 1;
    newEntry->IsSuspicious = FALSE;
    newEntry->IsGateway = FALSE;

    InsertTailList(&g_ArpContext.ArpCacheHead, &newEntry->ListEntry);
    g_ArpContext.CacheEntryCount++;

    return newEntry;
}

_Use_decl_annotations_
VOID UpdateCacheEntry(PARP_CACHE_ENTRY Entry, PUCHAR MacAddress)
{
    // Caller must hold CacheLock!

    RtlCopyMemory(Entry->MacAddress, MacAddress, 6);
    KeQuerySystemTime(&Entry->LastSeen);
    Entry->PacketCount++;
}

_Use_decl_annotations_
VOID ClearCache(VOID)
{
    KIRQL oldIrql;
    PLIST_ENTRY entry = NULL;
    PARP_CACHE_ENTRY cacheEntry = NULL;

    KeAcquireSpinLock(&g_ArpContext.CacheLock, &oldIrql);

    while (!IsListEmpty(&g_ArpContext.ArpCacheHead)) {
        entry = RemoveHeadList(&g_ArpContext.ArpCacheHead);
        cacheEntry = CONTAINING_RECORD(entry, ARP_CACHE_ENTRY, ListEntry);

        ExFreePoolWithTag(cacheEntry, ARP_MONITOR_TAG);
        g_ArpContext.CacheEntryCount--;
    }

    KeReleaseSpinLock(&g_ArpContext.CacheLock, oldIrql);

    ARP_LOG("Cache cleared: %lu entries removed", g_ArpContext.CacheEntryCount);
}

//=============================================================================
// Alert Management
//=============================================================================

_Use_decl_annotations_
VOID AddAlert(
    PUCHAR AttackerIP,
    PUCHAR OldMac,
    PUCHAR NewMac,
    PUCHAR VictimIP,
    ULONG Severity
)
{
    KIRQL oldIrql;
    PARP_ALERT alert = NULL;

    KeAcquireSpinLock(&g_ArpContext.AlertLock, &oldIrql);

    if (g_ArpContext.AlertCount >= MAX_ARP_ALERTS) {
        // Shift array to make room
        RtlMoveMemory(
            &g_ArpContext.Alerts[0],
            &g_ArpContext.Alerts[1],
            sizeof(ARP_ALERT) * (MAX_ARP_ALERTS - 1)
        );
        g_ArpContext.AlertCount = MAX_ARP_ALERTS - 1;
    }

    alert = &g_ArpContext.Alerts[g_ArpContext.AlertCount];

    RtlCopyMemory(alert->AttackerIP, AttackerIP, 4);
    RtlCopyMemory(alert->OldMac, OldMac, 6);
    RtlCopyMemory(alert->NewMac, NewMac, 6);
    RtlCopyMemory(alert->VictimIP, VictimIP, 4);

    KeQuerySystemTime(&alert->DetectionTime);
    alert->Severity = Severity;

    g_ArpContext.AlertCount++;
    g_ArpContext.Stats.ActiveAlerts = g_ArpContext.AlertCount;

    KeReleaseSpinLock(&g_ArpContext.AlertLock, oldIrql);
}

//=============================================================================
// Utility Functions
//=============================================================================

_Use_decl_annotations_
BOOLEAN CompareMAC(PUCHAR Mac1, PUCHAR Mac2)
{
    return RtlCompareMemory(Mac1, Mac2, 6) == 6;
}

_Use_decl_annotations_
BOOLEAN CompareIP(PUCHAR Ip1, PUCHAR Ip2)
{
    return RtlCompareMemory(Ip1, Ip2, 4) == 4;
}

_Use_decl_annotations_
VOID FormatMacAddress(PUCHAR Mac, PCHAR Buffer)
{
    RtlStringCbPrintfA(
        Buffer,
        18,
        "%02X:%02X:%02X:%02X:%02X:%02X",
        Mac[0], Mac[1], Mac[2],
        Mac[3], Mac[4], Mac[5]
    );
}

_Use_decl_annotations_
VOID FormatIpAddress(PUCHAR Ip, PCHAR Buffer)
{
    RtlStringCbPrintfA(
        Buffer,
        16,
        "%d.%d.%d.%d",
        Ip[0], Ip[1], Ip[2], Ip[3]
    );
}
