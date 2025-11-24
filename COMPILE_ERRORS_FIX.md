# 🔧 Fixing Compilation Errors

## Problem: "cannot open include file: ntifs.h"

You're seeing errors like:
```
не удается открыть источник файл "ntifs.h"
не удается открыть источник файл "ntstrsafe.h"
идентификатор "NTSTATUS" не определен
идентификатор "PDEVICE_OBJECT" не определен
...
```

### Root Cause

These errors occur because **kernel driver projects** require **Windows Driver Kit (WDK)**, which is not installed on your system.

The solution contains 4 projects:
- ✅ **ControlPanel** - User-mode app (NO WDK needed)
- ❌ **HWIDSpoofer** - Kernel driver (WDK required)
- ❌ **HVCIDriver** - Kernel driver (WDK required)
- ❌ **ARPMonitor** - Kernel driver (WDK required)

---

## Solution 1: Build Only ControlPanel (Quick & Easy)

### For Windows Users:

**Option A: Use the build script**
```cmd
build_controlpanel.cmd
```

**Option B: Manual build**
```cmd
msbuild ControlPanel\ControlPanel.vcxproj /p:Configuration=Release /p:Platform=x64
```

### For Visual Studio Users:

1. Open `HWIDSpoofer.sln`
2. In Solution Explorer, **right-click** each driver project:
   - Right-click `HWIDSpoofer` → **Unload Project**
   - Right-click `HVCIDriver` → **Unload Project**
   - Right-click `ARPMonitor` → **Unload Project**
3. Right-click `ControlPanel` → **Build**

Output will be in: `bin\x64\Release\ControlPanel.exe`

### Result:

ControlPanel will run and show:
```
  1. Legacy Spoofer Analysis       [NOT LOADED]
  2. HVCI-Compatible Monitor       [NOT LOADED]
  3. ARP Spoofing Detection        [NOT LOADED]
```

You can still:
- ✅ Read educational information about legacy techniques
- ✅ See technical comparisons (Legacy vs. Modern)
- ✅ Understand why HVCI blocks legacy methods
- ✅ View component status explanations

---

## Solution 2: Install WDK (Full Experience)

If you want to build and test the actual drivers:

### Step 1: Install Visual Studio

Download and install Visual Studio 2019 or 2022:
- https://visualstudio.microsoft.com/downloads/

**Required workload**: Desktop development with C++

### Step 2: Install Windows Driver Kit (WDK)

**IMPORTANT**: WDK version MUST match Visual Studio version!

#### For Visual Studio 2022:
Download: **WDK for Windows 11, version 22H2**
- https://go.microsoft.com/fwlink/?linkid=2196230

#### For Visual Studio 2019:
Download: **WDK for Windows 10, version 2004**
- https://go.microsoft.com/fwlink/?linkid=2128854

### Step 3: Verify Installation

After installing WDK, verify:

```cmd
# Check WDK registry key
reg query "HKLM\SOFTWARE\Microsoft\Windows Kits\Installed Roots" /v KitsRoot10

# Should show something like:
# KitsRoot10    REG_SZ    C:\Program Files (x86)\Windows Kits\10\
```

Or check in Visual Studio:
- Extensions → Manage Extensions
- Look for "Windows Driver Kit" or "WDK"

### Step 4: Restart Visual Studio

**Important**: Close and reopen Visual Studio after installing WDK!

### Step 5: Build Solution

```cmd
msbuild HWIDSpoofer.sln /p:Configuration=Release /p:Platform=x64
```

Or in Visual Studio:
- Build → Build Solution (Ctrl+Shift+B)

---

## Solution 3: Linux/WSL Users

### For ControlPanel Only:

You can cross-compile the user-mode application using MinGW:

```bash
# Install MinGW-w64
sudo apt-get update
sudo apt-get install mingw-w64

# Compile ControlPanel
cd ControlPanel
x86_64-w64-mingw32-g++ -o ControlPanel.exe Main.cpp \
    -static -std=c++17 -lkernel32 -luser32 -ladvapi32 \
    -D_WIN32_WINNT=0x0A00
```

**Note**: Kernel drivers CANNOT be compiled on Linux. They require:
- Windows build environment
- Windows Driver Kit (WDK)
- MSVC compiler

---

## Detailed Error Explanations

### Error: `ntifs.h: No such file or directory`

**What it means**: This header is part of WDK and contains kernel-mode definitions.

**Where it's used**:
- `HWIDSpoofer/` - All files (legacy driver)
- `ModernDriver/` - HVCI driver files
- `ARPMonitor/` - ARP detection driver

**Not used in**:
- ✅ `ControlPanel/` - Uses standard Windows SDK headers only

### Error: `идентификатор "NTSTATUS" не определен`

**Translation**: Identifier "NTSTATUS" is not defined

**Cause**: `NTSTATUS` is a kernel-mode type from `ntdef.h`/`ntifs.h`

**Solution**: Install WDK (or build only ControlPanel)

### Error: `директива #error: "No Target Architecture"`

**Cause**: Not building for correct platform (x64)

**Solution**:
```cmd
msbuild /p:Platform=x64    # ← Specify x64 platform
```

---

## Quick Reference: What Needs WDK?

| File/Project | Needs WDK? | Why? |
|--------------|------------|------|
| `ControlPanel/Main.cpp` | ❌ NO | User-mode, uses only Windows SDK |
| `ControlPanel/Common.h` | ❌ NO | User-mode headers |
| `HWIDSpoofer/*.cpp` | ✅ YES | Kernel driver, uses `ntifs.h` |
| `ModernDriver/*.c` | ✅ YES | Kernel driver, uses `ntifs.h` |
| `ARPMonitor/*.c` | ✅ YES | Kernel driver, uses `ntifs.h` |

---

## Recommended Approach for Students

### For Coursework Submission:

**Option 1: Full Setup** (Recommended if you have time)
1. Install Visual Studio 2022
2. Install WDK for Windows 11
3. Build entire solution
4. Test drivers on VM (with test signing enabled)
5. Submit full working project

**Option 2: Documentation-Focused** (If WDK installation is problematic)
1. Build only ControlPanel
2. Document WHY drivers can't be tested without WDK
3. Include screenshots of ControlPanel running
4. Provide technical analysis of driver code (without executing it)
5. Explain HVCI compatibility in written report

### Testing Drivers Without Hardware:

If you build drivers but want to test safely:

1. **Use a Virtual Machine**:
   - VMware Workstation or VirtualBox
   - Install Windows 10/11 in VM
   - Enable test signing in VM
   - Install drivers in VM only

2. **Enable Test Signing** (VM only):
   ```cmd
   bcdedit /set testsigning on
   bcdedit /set nointegritychecks on
   shutdown /r /t 0
   ```

3. **Install Drivers**:
   ```cmd
   sc create HVCIDriver type= kernel binPath= "C:\full\path\to\HVCIDriver.sys"
   sc start HVCIDriver
   ```

---

## Still Having Issues?

### Common Problems:

**Problem**: "MSBuild not found"
**Solution**: Run from "Developer Command Prompt for VS" or "x64 Native Tools Command Prompt"

**Problem**: "Platform toolset v143 not found"
**Solution**: Install Visual Studio 2022 or change toolset in project properties

**Problem**: "Windows SDK not found"
**Solution**: Install via Visual Studio Installer → Individual Components → Windows 10/11 SDK

---

## Summary

### To Build ControlPanel ONLY (No WDK):
```cmd
build_controlpanel.cmd
```
or
```cmd
msbuild ControlPanel\ControlPanel.vcxproj /p:Platform=x64 /p:Configuration=Release
```

### To Build Everything (Requires WDK):
1. Install Visual Studio 2019/2022
2. Install matching WDK version
3. Restart Visual Studio
4. Build solution:
```cmd
msbuild HWIDSpoofer.sln /p:Platform=x64 /p:Configuration=Release
```

---

**See [BUILD.md](BUILD.md) for complete installation instructions and troubleshooting guide.**
