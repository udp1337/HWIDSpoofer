# Security Research Control Panel

## Overview

The Control Panel is a unified console application that integrates all components of the HWID Security Research project, demonstrating the evolution from legacy techniques to modern HVCI-compatible approaches.

## Educational Purpose

This application is designed for **educational cybersecurity research** and demonstrates:

1. **Legacy Techniques Analysis** - Understanding outdated methods that no longer work on modern systems
2. **HVCI-Compatible Monitoring** - Modern approaches that work with Hypervisor-protected Code Integrity
3. **Network Security** - ARP spoofing detection for defensive security

## Components

### 1. Legacy Spoofer Analysis

**Status**: Read-only demonstration (NOT executable)

**Purpose**: Educational analysis of legacy HWID spoofing techniques

**Features**:
- Analysis of outdated pattern scanning methods
- Explanation of IRP hooking techniques
- Discussion of physical memory access approaches
- **Why they fail on HVCI-enabled systems**

**Key Learning Points**:
- Direct kernel memory writes are blocked by EPT (Extended Page Tables)
- IRP hooking violates W^X (Write XOR Execute) policy
- Physical memory access is protected by VTL 1 isolation
- Pattern scanning is unreliable across Windows versions

### 2. HVCI-Compatible Monitor

**Status**: Fully functional monitoring driver

**Purpose**: Demonstrate HVCI-compatible techniques for HWID collection

**Features**:
- System hardware ID overview
- Disk information via Storage Query API
- Network adapter enumeration
- SMBIOS data via Registry access

**Technical Approach**:
```
✓ Official Microsoft APIs (IOCTL_STORAGE_QUERY_PROPERTY)
✓ Registry-based information retrieval
✓ NonPagedPoolNx memory allocations
✓ No hooking or memory patching
✓ Fully compatible with HVCI, Secure Boot, and TPM
```

### 3. ARP Spoofing Detection

**Status**: Fully functional network security tool

**Purpose**: Detect and alert on ARP spoofing attacks

**Features**:
- Real-time ARP packet monitoring
- IP-MAC binding cache
- Suspicious activity detection
- Detailed alert reporting
- Statistics tracking

**Detection Capabilities**:
- MAC address conflicts
- Gratuitous ARP abuse
- Rapid IP-MAC changes
- Duplicate IP detection

## Installation

### Prerequisites

- Windows 10/11 (version 1607 or later)
- Administrator privileges
- Visual Studio 2019/2022 with Windows Driver Kit (WDK)
- Test signing enabled for driver installation

### Building

#### Using Visual Studio:

1. Open `HWIDSpoofer.sln`
2. Select configuration (Debug/Release)
3. Build solution (Ctrl+Shift+B)
4. Output: `bin\x64\[Debug|Release]\ControlPanel.exe`

#### Using MSBuild:

```cmd
msbuild HWIDSpoofer.sln /p:Configuration=Release /p:Platform=x64
```

### Driver Installation

Before running the Control Panel, install the required drivers:

#### HVCI Driver:

```cmd
cd ModernDriver
sc create HVCIDriver type= kernel binPath= "%CD%\HVCIDriver.sys"
sc start HVCIDriver
```

#### ARP Monitor:

```cmd
cd ARPMonitor
sc create ARPMonitor type= kernel binPath= "%CD%\ARPMonitor.sys"
sc start ARPMonitor
```

> **Note**: On systems with Secure Boot enabled, drivers must be properly signed. Use test signing mode for development.

## Usage

### Running the Control Panel

```cmd
ControlPanel.exe
```

### Main Menu Options

```
=======================================================================
                   Security Research Control Panel
=======================================================================

  1. Legacy Spoofer Analysis       [NOT LOADED]
  2. HVCI-Compatible Monitor       [LOADED]
  3. ARP Spoofing Detection        [LOADED]

  4. View Technical Comparison
  5. Component Status

  0. Exit
```

### Example Workflow

#### 1. Analyze Legacy Techniques

```
Main Menu → 1 (Legacy Spoofer) → 1 (Analyze Legacy Techniques)
```

This shows detailed analysis of why legacy methods fail on HVCI-enabled systems.

#### 2. Collect HVID Information

```
Main Menu → 2 (HVCI Monitor) → 1 (Collect System HWID Overview)
```

Example output:
```
Computer Name: RESEARCH-PC
Disks Found: 2
Network Adapters: 3
SMBIOS Available: Yes
GPU Detection: Yes

✓ All information retrieved using HVCI-compatible methods:
  • Storage Query API (official Microsoft API)
  • Registry access (no physical memory access)
  • IoGetDeviceInterfaces (no NDIS structure walking)
  • NonPagedPoolNx allocations (W^X compliant)
```

#### 3. Monitor Network Security

```
Main Menu → 3 (ARP Monitor) → 1 (Show ARP Statistics)
```

Example output:
```
=== ARP Monitor Statistics ===
Total packets:       15234
ARP requests:        8421
ARP replies:         6813
Gratuitous ARP:      142

Security Status:
  Suspicious packets:  0
  MAC conflicts:       0
  Active alerts:       0

✓ No suspicious activity detected
```

#### 4. Compare Legacy vs. Modern

```
Main Menu → 4 (View Technical Comparison)
```

Shows side-by-side comparison of techniques with HVCI compatibility status.

## Technical Details

### Architecture

```
┌─────────────────────────────────────────────────────┐
│              Control Panel (User Mode)              │
│  - Main menu system                                 │
│  - Component status tracking                        │
│  - Educational information display                  │
└─────────────────────────────────────────────────────┘
                        │
        ┌───────────────┼───────────────┐
        │               │               │
        ▼               ▼               ▼
┌──────────────┐ ┌──────────────┐ ┌──────────────┐
│   Legacy     │ │ HVCI Driver  │ │ ARP Monitor  │
│  (Analysis)  │ │ (Monitoring) │ │ (Detection)  │
└──────────────┘ └──────────────┘ └──────────────┘
     Analysis    DeviceIoControl  DeviceIoControl
                       │               │
                       ▼               ▼
                ┌─────────────┐ ┌─────────────┐
                │ \\.\\HVCI    │ │ \\.\\ARP     │
                │  Driver     │ │  Monitor    │
                └─────────────┘ └─────────────┘
```

### Driver Communication

All driver communication uses standard Windows IOCTL (I/O Control) mechanism:

```cpp
BOOL SendDriverIOCTL(
    HANDLE hDriver,
    DWORD ioctl,
    PVOID inputBuffer,
    DWORD inputSize,
    PVOID outputBuffer,
    DWORD outputSize,
    PDWORD bytesReturned
);
```

### IOCTL Codes

#### HVCI Driver:

| IOCTL | Purpose | Access |
|-------|---------|--------|
| `IOCTL_HVCI_GET_HWID_INFO` | Get system overview | Read-only |
| `IOCTL_HVCI_GET_DISK_INFO` | Get disk details | Read-only |
| `IOCTL_HVCI_GET_NIC_INFO` | Get network adapters | Read-only |
| `IOCTL_HVCI_GET_SMBIOS_INFO` | Get SMBIOS data | Read-only |

#### ARP Monitor:

| IOCTL | Purpose | Access |
|-------|---------|--------|
| `IOCTL_ARP_GET_STATISTICS` | Get packet statistics | Read-only |
| `IOCTL_ARP_GET_ALERTS` | Get spoofing alerts | Read-only |
| `IOCTL_ARP_ENABLE_MONITOR` | Enable/disable monitoring | Write |
| `IOCTL_ARP_CLEAR_CACHE` | Clear ARP cache | Write |

## Security Considerations

### This is NOT a bypass tool

The Control Panel demonstrates **why legacy bypass techniques no longer work** on modern Windows systems. It does NOT:

- ❌ Bypass HVCI protection
- ❌ Circumvent Secure Boot
- ❌ Spoof TPM attestation
- ❌ Modify system hardware IDs
- ❌ Perform ARP spoofing attacks

### What it DOES do

- ✅ Educate about security mechanisms
- ✅ Demonstrate HVCI compatibility
- ✅ Provide read-only monitoring
- ✅ Detect network attacks (defensive)
- ✅ Show technical comparisons

### Authorized Use Cases

This tool is intended for:

1. **Academic coursework** in cybersecurity programs
2. **Security research** in controlled environments
3. **Authorized penetration testing** engagements
4. **Defensive security training**
5. **Understanding Windows security architecture**

## HVCI Compatibility

### What is HVCI?

Hypervisor-protected Code Integrity is a Windows security feature that uses hardware virtualization to protect kernel-mode processes and drivers from being compromised.

**Key protections**:
- **W^X (Write XOR Execute)**: Memory cannot be both writable and executable
- **EPT (Extended Page Tables)**: Hypervisor controls actual memory permissions
- **Code Integrity**: All kernel code must be Microsoft-signed
- **VTL 1 Isolation**: Security functions run in separate virtual trust level

### Checking HVCI Status

The Control Panel automatically detects HVCI status:

```cpp
// Registry check
HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\DeviceGuard\
  Scenarios\HypervisorEnforcedCodeIntegrity
  Enabled = 1  (HVCI is ON)
```

### Why Legacy Techniques Fail

| Technique | HVCI Block Mechanism |
|-----------|---------------------|
| **IRP Hooking** | W^X prevents MajorFunction table modification |
| **Pattern Scanning + Write** | EPT blocks kernel memory writes |
| **Physical Memory Access** | VTL 1 protects physical memory mappings |
| **Direct Structure Modification** | Page permissions enforced by hypervisor |

## Troubleshooting

### Control Panel starts but shows "NOT LOADED" for drivers

**Cause**: Drivers not installed or not running

**Solution**:
```cmd
# Check driver status
sc query HVCIDriver
sc query ARPMonitor

# Start drivers
sc start HVCIDriver
sc start ARPMonitor
```

### "Access Denied" when opening driver

**Cause**: Not running as Administrator

**Solution**: Right-click → "Run as Administrator"

### Driver fails to load on Windows 11

**Cause**: HVCI enabled + unsigned driver

**Solution** (test systems only):
```cmd
# Enable test signing
bcdedit /set testsigning on
bcdedit /set nointegritychecks on
# Reboot required
```

### ARP Monitor shows no packets

**Cause**: Monitoring not enabled or no network activity

**Solution**:
```
ARP Monitor Menu → Enable Monitoring
```

## Performance Impact

### Memory Usage

- Control Panel: ~2 MB (user mode)
- HVCI Driver: ~1 MB (kernel mode)
- ARP Monitor: ~500 KB + cache (~100 KB per 1000 packets)

### CPU Usage

- Idle: < 0.1%
- Active monitoring: < 1%
- Packet processing (ARP): < 2% on high-traffic networks

## Comparison: Legacy vs. Modern

### Legacy Approach (2015-2018)

```cpp
// Example: Legacy disk serial spoofing
PVOID raidExtension = PatternScan("\x48\x89\x5c\x24...");  // ❌ Unreliable
*(PULONG64)(raidExtension + 0x120) = newSerial;           // ❌ HVCI blocks
```

**Problems**:
- Pattern scanning breaks across Windows updates
- Direct writes blocked by EPT
- Requires unsigned driver (incompatible with Secure Boot)
- Triggers PatchGuard/KPP violations

### Modern Approach (2023+)

```cpp
// Example: HVCI-compatible disk info
STORAGE_PROPERTY_QUERY query = { StorageDeviceProperty, PropertyStandardQuery };
DeviceIoControl(hDisk, IOCTL_STORAGE_QUERY_PROPERTY,
    &query, sizeof(query), descriptor, size, &returned, NULL);  // ✓ Official API
```

**Benefits**:
- Uses documented Windows APIs
- Works with HVCI, Secure Boot, TPM
- No kernel memory writes
- Read-only, non-intrusive
- Stable across Windows versions

## Educational Value

### What Students Learn

1. **Evolution of Windows Security**
   - How Microsoft responded to kernel exploits
   - Virtualization-based security architecture
   - Hardware-assisted protection (VT-x, EPT)

2. **Attack vs. Defense Trade-offs**
   - Why certain attacks become obsolete
   - How modern hardware prevents tampering
   - Defense-in-depth strategies

3. **Driver Development Best Practices**
   - Proper use of Windows APIs
   - IRQL-aware programming
   - Memory pool types (Nx vs. non-Nx)
   - SAL annotations for security

4. **Network Security Fundamentals**
   - ARP protocol vulnerabilities
   - Man-in-the-Middle attack detection
   - Defensive monitoring techniques

## License and Disclaimer

### Educational Use Only

This software is provided for **EDUCATIONAL PURPOSES ONLY**. By using this software, you agree that:

1. You have proper authorization to run security research tools
2. You understand the legal implications
3. You accept full responsibility for your actions
4. You will NOT use this for:
   - Bypassing anti-cheat systems
   - Unauthorized system modification
   - Any illegal activities

### No Warranty

This software is provided "AS IS" without warranty of any kind. The authors are not responsible for any damages or legal consequences resulting from its use.

## References

### Microsoft Documentation

- [Driver Security Checklist](https://docs.microsoft.com/en-us/windows-hardware/drivers/driversecurity/driver-security-checklist)
- [Hypervisor-protected Code Integrity](https://docs.microsoft.com/en-us/windows/security/threat-protection/device-guard/enable-virtualization-based-protection-of-code-integrity)
- [Memory Management](https://docs.microsoft.com/en-us/windows-hardware/drivers/kernel/managing-memory-for-drivers)
- [Storage Class Drivers](https://docs.microsoft.com/en-us/windows-hardware/drivers/storage/storage-class-drivers)

### Security Research

- [Windows Internals, Part 1 (7th Edition)](https://www.microsoftpressstore.com/store/windows-internals-part-1-system-architecture-processes-9780735684188)
- [Rootkits and Bootkits](https://nostarch.com/rootkits)
- [ARP Spoofing Detection and Prevention](https://www.ietf.org/rfc/rfc5227.txt)

## Contributing

This is an educational project. Contributions that enhance educational value are welcome:

- ✅ Better explanations and documentation
- ✅ Additional defensive security features
- ✅ Improved visualization of concepts
- ❌ Attack/bypass improvements

## Support

For questions about the educational aspects of this project:

1. Review the documentation in each component folder
2. Check the comparison views in the Control Panel
3. Consult Windows security documentation
4. Discuss in educational forums (with instructor approval)

---

**Remember**: The goal is to understand **why** legacy techniques fail, not to create new bypass methods. Modern Windows security is built on hardware-assisted protections that cannot be circumvented through software alone.
