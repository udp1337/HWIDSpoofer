# Build Instructions - HWIDSpoofer Educational Project

## 🎯 Project Structure

This solution contains **4 projects** with different requirements:

| Project | Type | Requires WDK? | Purpose |
|---------|------|---------------|---------|
| **ControlPanel** | User-mode app | ❌ NO | Console UI (can build standalone) |
| **HWIDSpoofer** | Kernel driver | ✅ YES | Legacy driver (educational analysis) |
| **HVCIDriver** | Kernel driver | ✅ YES | Modern HVCI-compatible driver |
| **ARPMonitor** | Kernel driver | ✅ YES | ARP detection driver |

## 🚀 Quick Start Options

### Option 1: Build Only ControlPanel (No WDK Required)

If you just want the console application **without** the drivers:

#### Prerequisites:
- Visual Studio 2019/2022 (Community Edition or higher)
- Windows SDK
- C++ Desktop Development workload

#### Build Steps:

1. **Open Command Prompt** (VS Developer Command Prompt):
```cmd
cd /d "%~dp0"
msbuild ControlPanel\ControlPanel.vcxproj /p:Configuration=Release /p:Platform=x64
```

2. **Output location**:
```
bin\x64\Release\ControlPanel.exe
```

3. **Run**:
```cmd
cd bin\x64\Release
ControlPanel.exe
```

**Note**: Without drivers, ControlPanel will show "NOT LOADED" status but still provides educational information and comparisons.

---

### Option 2: Build Full Solution (Requires WDK)

To build **all components** including kernel drivers:

#### Prerequisites:

1. **Visual Studio 2019/2022**
   - Download: https://visualstudio.microsoft.com/downloads/
   - Workloads required:
     - Desktop development with C++
     - Windows SDK (10.0.19041.0 or later)

2. **Windows Driver Kit (WDK)**
   - Download: https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk
   - **IMPORTANT**: WDK version MUST match your Visual Studio version

   For Visual Studio 2022:
   - WDK for Windows 11, version 22H2: https://go.microsoft.com/fwlink/?linkid=2196230

   For Visual Studio 2019:
   - WDK for Windows 10, version 2004: https://go.microsoft.com/fwlink/?linkid=2128854

3. **WDK Extension**
   - Install via Visual Studio Installer → Individual Components
   - Search for "Windows Driver Kit"
   - Check "Windows Driver Kit Visual Studio Extension"

#### Installation Steps:

1. **Install Visual Studio** with C++ workload
2. **Install Windows SDK** (usually comes with VS)
3. **Install WDK** matching your VS version
4. **Restart Visual Studio**

#### Verify Installation:

Open Visual Studio → Extensions → Manage Extensions
- You should see "WDK Extension" listed

Or check registry:
```cmd
reg query "HKLM\SOFTWARE\Microsoft\Windows Kits\Installed Roots" /v KitsRoot10
```

#### Build All Projects:

```cmd
# Open VS Developer Command Prompt (x64 Native Tools)
cd /d "C:\path\to\HWIDSpoofer"

# Build entire solution
msbuild HWIDSpoofer.sln /p:Configuration=Release /p:Platform=x64

# Or build specific project
msbuild HWIDSpoofer\HWIDSpoofer.vcxproj /p:Configuration=Release /p:Platform=x64
msbuild ModernDriver\HVCIDriver.vcxproj /p:Configuration=Release /p:Platform=x64
msbuild ARPMonitor\ARPMonitor.vcxproj /p:Configuration=Release /p:Platform=x64
msbuild ControlPanel\ControlPanel.vcxproj /p:Configuration=Release /p:Platform=x64
```

#### Output Locations:

```
bin\x64\Release\
├── ControlPanel.exe        (User-mode application)
├── HWIDSpoofer.sys         (Legacy driver - educational only)
├── HVCIDriver.sys          (HVCI-compatible driver)
└── ARPMonitor.sys          (ARP detection driver)
```

---

## 🔧 Troubleshooting Build Errors

### Error: "cannot open include file: ntifs.h"

**Cause**: Windows Driver Kit (WDK) not installed

**Solutions**:

1. **If you only need ControlPanel**: Use Option 1 above (build only ControlPanel)

2. **If you need full solution**: Install WDK (see Option 2 prerequisites)

3. **Temporary workaround**: Unload driver projects in Visual Studio:
   - Right-click on `HWIDSpoofer` project → Unload Project
   - Right-click on `HVCIDriver` project → Unload Project
   - Right-click on `ARPMonitor` project → Unload Project
   - Build only `ControlPanel` project

### Error: "error: No Target Architecture"

**Cause**: Not building for x64 platform

**Solution**:
```cmd
msbuild /p:Platform=x64
```
Or in Visual Studio: Select "x64" from platform dropdown

### Error: LNK1181 - Cannot open input file

**Cause**: Missing WDK libraries

**Solution**: Install WDK and ensure environment variables are set:
```cmd
set WindowsSdkDir=C:\Program Files (x86)\Windows Kits\10\
set WindowsSdkVerBinPath=C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\
```

### Error: MSB8036 - Windows SDK not found

**Solution**: Install Windows SDK via Visual Studio Installer

---

## 🧪 Testing the Build

### Test ControlPanel (without drivers):

```cmd
cd bin\x64\Release
ControlPanel.exe
```

Expected output:
```
========================================================================
                    Security Research Control Panel
========================================================================

⚠ WARNING: Educational Use Only
This software is for cybersecurity research and education.

Press Y to agree and continue, N to exit:
```

### Install and Test Drivers (requires admin + test signing):

#### Enable Test Signing:

```cmd
# Run as Administrator
bcdedit /set testsigning on
bcdedit /set nointegritychecks on
# Reboot required
shutdown /r /t 0
```

#### Install Drivers:

```cmd
# Run as Administrator in driver directory
cd bin\x64\Release

# Install HVCI Driver
sc create HVCIDriver type= kernel binPath= "%CD%\HVCIDriver.sys"
sc start HVCIDriver

# Install ARP Monitor
sc create ARPMonitor type= kernel binPath= "%CD%\ARPMonitor.sys"
sc start ARPMonitor

# Verify
sc query HVCIDriver
sc query ARPMonitor
```

#### Run ControlPanel:

```cmd
ControlPanel.exe
```

Now should show:
```
  1. Legacy Spoofer Analysis       [NOT LOADED]
  2. HVCI-Compatible Monitor       [LOADED]      ← ✓
  3. ARP Spoofing Detection        [LOADED]      ← ✓
```

---

## 📦 Build Configurations

### Debug vs. Release

| Configuration | Optimization | Symbols | Size | Use Case |
|---------------|-------------|---------|------|----------|
| **Debug** | Disabled | Full | Large | Development, debugging |
| **Release** | Enabled | Minimal | Small | Production, testing |

### Build for Debug:

```cmd
msbuild HWIDSpoofer.sln /p:Configuration=Debug /p:Platform=x64
```

### Build for Multiple Platforms:

```cmd
# x64 (Intel/AMD 64-bit)
msbuild HWIDSpoofer.sln /p:Platform=x64

# ARM64 (Surface Pro X, Qualcomm)
msbuild HWIDSpoofer.sln /p:Platform=ARM64
```

---

## 🐧 Building on Linux (via CMake - ControlPanel only)

For the user-mode ControlPanel application, you can cross-compile:

### Using MinGW-w64:

```bash
# Install MinGW
sudo apt-get install mingw-w64

# Compile
cd ControlPanel
x86_64-w64-mingw32-g++ -o ControlPanel.exe Main.cpp \
    -static -std=c++17 -lkernel32 -luser32 -ladvapi32
```

**Note**: Drivers CANNOT be built on Linux (requires WDK + Windows build environment)

---

## 🎓 For Educational/Coursework Use

### Recommended Build Strategy:

1. **Install WDK** (full setup) - demonstrates professional driver development environment
2. **Build all projects** - shows complete understanding of architecture
3. **Test on VM** - use Windows 10/11 VM with test signing enabled

### What to Include in Coursework Submission:

```
submission/
├── source/                     (Full source code)
│   └── HWIDSpoofer/
├── documentation/
│   ├── BUILD.md               (This file)
│   ├── ControlPanel/README.md
│   ├── ModernDriver/README_HVCI.md
│   └── ARPMonitor/README_ARP.md
├── binaries/                  (Compiled output)
│   ├── x64/
│   │   ├── ControlPanel.exe
│   │   ├── HVCIDriver.sys
│   │   └── ARPMonitor.sys
│   └── screenshots/
│       ├── menu.png
│       ├── hvci-output.png
│       └── arp-detection.png
└── report.pdf                 (Technical report)
```

---

## 🔐 Security Notes

### Driver Signing for Production:

**Test environment** (as shown above):
```cmd
bcdedit /set testsigning on
```

**Production environment** (requires):
- Extended Validation (EV) Code Signing Certificate (~$300-500/year)
- Hardware Security Module (HSM) or USB token
- Microsoft Hardware Dev Center submission
- WHQL certification

### HVCI Compatibility Testing:

Check if HVCI is enabled:
```cmd
msinfo32
# Look for: "Virtualization-based security" → "Running"
```

Or via registry:
```cmd
reg query "HKLM\SYSTEM\CurrentControlSet\Control\DeviceGuard\Scenarios\HypervisorEnforcedCodeIntegrity" /v Enabled
```

---

## 📚 Additional Resources

### Microsoft Documentation:

- WDK Download: https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk
- Driver Development: https://learn.microsoft.com/en-us/windows-hardware/drivers/
- HVCI Requirements: https://learn.microsoft.com/en-us/windows-hardware/design/device-experiences/oem-hvci-enablement

### Tutorials:

- Setting up WDK: https://learn.microsoft.com/en-us/windows-hardware/drivers/develop/building-a-driver
- Test Signing: https://learn.microsoft.com/en-us/windows-hardware/drivers/install/the-testsigning-boot-configuration-option

### Tools:

- WinDbg (Kernel Debugging): https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/
- OSR Online (Driver Dev Community): https://www.osronline.com/
- Driver Verifier: `verifier.exe` (built into Windows)

---

## ❓ FAQ

### Q: Can I build this on macOS?

**A**: No. Kernel drivers require Windows build environment and WDK. ControlPanel (user-mode) could theoretically be cross-compiled but requires Windows-specific APIs.

### Q: Do I need Visual Studio Professional?

**A**: No. Visual Studio Community Edition (free) is sufficient.

### Q: Can I use VS Code instead of Visual Studio?

**A**: For ControlPanel: Yes (with C++ extensions and MSBuild)
For drivers: No (requires full Visual Studio + WDK integration)

### Q: How large is the WDK download?

**A**: ~500-800 MB download, ~2-3 GB installed

### Q: Can I build 32-bit (x86) versions?

**A**: ControlPanel: Yes (but solution is configured for x64)
Drivers: Not recommended (modern Windows is x64-focused)

### Q: Will this work on Windows 7/8?

**A**: Legacy spoofer might work on Windows 7/8
HVCI driver requires Windows 10 1607+ (HVCI was introduced then)
ControlPanel should work on Windows 7+

---

## 📝 Build Checklist

Before building, verify:

- [ ] Visual Studio 2019 or 2022 installed
- [ ] C++ Desktop Development workload installed
- [ ] Windows SDK installed (10.0.19041.0+)
- [ ] (For drivers) Windows Driver Kit (WDK) installed
- [ ] (For drivers) WDK version matches Visual Studio version
- [ ] Platform set to x64
- [ ] Configuration set to Debug or Release
- [ ] All project dependencies available

For driver testing:

- [ ] Administrator privileges available
- [ ] Test signing enabled (bcdedit /set testsigning on)
- [ ] System rebooted after enabling test signing
- [ ] Drivers compiled successfully
- [ ] Driver installation paths correct (absolute paths for binPath)

---

## 🆘 Getting Help

If you encounter build issues:

1. **Check error message** - most errors indicate missing WDK or SDK
2. **Review prerequisites** - ensure all required components installed
3. **Try Option 1** - build only ControlPanel to isolate driver issues
4. **Check VS version** - WDK must match Visual Studio version exactly
5. **Clean solution** - `msbuild /t:Clean` then rebuild

For coursework/educational support, consult with your instructor or TA.

---

**Last Updated**: November 2024
**Compatible With**: Visual Studio 2019/2022, WDK for Windows 10/11
