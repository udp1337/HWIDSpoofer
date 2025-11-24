# HVCI-Compatible Kernel Driver

**Образовательный проект для изучения современной разработки драйверов Windows**

## ⚠️ Disclaimer

Этот проект создан **исключительно для образовательных целей** в рамках изучения:
- Windows Kernel Programming
- HVCI (Hypervisor-protected Code Integrity)
- Современных методов защиты Windows
- Defensive Security

**НЕ используйте** для:
- Обхода систем защиты
- Модификации игровых античитов
- Любой незаконной деятельности

---

## 📋 Содержание

1. [Что это такое](#что-это-такое)
2. [Отличия от старого spoofer'а](#отличия-от-старого-spooferа)
3. [Требования](#требования)
4. [Архитектура](#архитектура)
5. [Сборка проекта](#сборка-проекта)
6. [Тестирование](#тестирование)
7. [HVCI совместимость](#hvci-совместимость)
8. [Образовательные цели](#образовательные-цели)

---

## 🎯 Что это такое

Это **современный kernel-mode драйвер**, совместимый с HVCI, который:

✅ **Читает** информацию о системе (HWID)
✅ **Не модифицирует** защищенные структуры
✅ **Использует** только официальные API
✅ **Следует** всем требованиям HVCI
✅ **Работает** на Windows 11 с включенным HVCI

**Ключевое отличие:** Это драйвер для **мониторинга**, а не модификации.

---

## 🆚 Отличия от старого spoofer'а

| Аспект | Старый spoofer | Новый HVCI driver |
|--------|----------------|-------------------|
| **Цель** | Модификация HWID | Чтение HWID |
| **HVCI** | ❌ Несовместим | ✅ Полностью совместим |
| **Методы** | Pattern scanning, hooking | Официальные API |
| **Память** | Прямая модификация физической | Только чтение |
| **IRP Hooking** | Да (нарушение HVCI) | Нет |
| **SMBIOS** | Модификация таблиц | Чтение через Registry |
| **Загрузка** | kdmapper (exploit) | Официальная установка |
| **Подпись** | Не требуется (exploit) | Test signing / WHQL |
| **Законность** | ⚠️ Сомнительная | ✅ Легальная |

---

## 📦 Требования

### Для разработки:

- **Windows 10/11** (рекомендуется 22H2 или новее)
- **Visual Studio 2022** с компонентами:
  - Desktop development with C++
  - Windows Driver Kit (WDK)
- **Windows SDK** (последняя версия)
- **Windows Driver Kit (WDK)** 10.0.22621.0 или новее

### Для тестирования:

- **Виртуальная машина** (настоятельно рекомендуется!)
  - VMware Workstation / Hyper-V
  - Windows 10/11 guest
- **Test Signing Mode** или
- **Self-signed certificate** для тестирования
- **Права администратора**

### Опционально (для продакшена):

- **EV Code Signing Certificate** (~$500/год)
- **Windows Hardware Lab Kit** (для WHQL сертификации)
- **Microsoft Partner Account**

---

## 🏗️ Архитектура

### Компоненты проекта:

```
ModernDriver/
├── HVCIDriver.h          # Заголовочный файл с структурами
├── HVCIDriver.c          # Основная логика драйвера
├── HWIDCollector.c       # Функции сбора HWID (READ-ONLY)
├── HVCIDriver.inf        # Файл установки драйвера
├── TestApp.c             # User-mode тестовое приложение
└── README_HVCI.md        # Эта документация
```

### Поток данных:

```
User-mode Application (TestApp.exe)
        ↓ IOCTL_HVCI_GET_HWID_INFO
Symbolic Link (\DosDevices\HVCIDriver)
        ↓
Device Object (\Device\HVCIDriver)
        ↓
IRP_MJ_DEVICE_CONTROL Handler
        ↓
CollectSystemHWID()
        ├─> CollectDiskInfo()        [Чтение через IOCTL_STORAGE_QUERY_PROPERTY]
        ├─> CollectNicInfo()         [Чтение через IoGetDeviceInterfaces]
        ├─> CollectSmbiosInfo()      [Чтение через Registry]
        └─> CheckTPMPresence()       [Чтение через Registry]
        ↓
Return SYSTEM_HWID_INFO to user-mode
```

### Принципы HVCI-совместимости:

1. **W^X (Write XOR Execute)**
   - Память либо записываемая, либо исполняемая
   - Используем `NonPagedPoolNx` (Non-Executable)

2. **Только чтение защищенных структур**
   - Нет модификации MajorFunction таблиц
   - Нет прямого доступа к физической памяти для записи
   - Нет pattern scanning в kernel memory

3. **Официальные API**
   - `ZwQuerySystemInformation` вместо прямого доступа
   - `IoGetDeviceObjectPointer` вместо обхода NDIS
   - Registry API вместо чтения SMBIOS из памяти

4. **Статический анализ**
   - SAL annotations (`_Use_decl_annotations_`, `_IRQL_requires_`)
   - Code Analysis (CA) compliant
   - PREfast warnings устранены

---

## 🔨 Сборка проекта

### Шаг 1: Подготовка окружения

1. Установите Visual Studio 2022:
   ```
   - Workload: Desktop development with C++
   - Individual component: Windows 11 SDK
   ```

2. Установите WDK:
   - Скачайте с https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk
   - Установите WDK 10.0.22621.0 или новее

3. Проверьте установку:
   ```cmd
   cd "C:\Program Files (x86)\Windows Kits\10\bin\x64"
   dir inf2cat.exe    # Должен существовать
   dir signtool.exe   # Должен существовать
   ```

### Шаг 2: Создание проекта в Visual Studio

1. Откройте Visual Studio 2022
2. Создайте новый проект:
   - Template: **Kernel Mode Driver, Empty (KMDF)**
   - Name: `HVCIDriver`
   - Location: `/home/user/HWIDSpoofer/ModernDriver/`

3. Добавьте файлы в проект:
   - Add Existing Item → `HVCIDriver.h`
   - Add Existing Item → `HVCIDriver.c`
   - Add Existing Item → `HWIDCollector.c`
   - Add Existing Item → `HVCIDriver.inf`

### Шаг 3: Настройка проекта для HVCI

1. Откройте **Project Properties** (Alt+F7)

2. **Configuration Properties → Driver Settings → General**
   ```
   Target OS Version: Windows 10 or later
   Target Platform: Desktop
   ```

3. **Configuration Properties → C/C++ → Code Generation**
   ```
   Spectre Mitigation: Enabled (/Qspectre)
   Control Flow Guard: Yes (/guard:cf)
   Buffer Security Check: Yes (/GS)
   ```

4. **Configuration Properties → C/C++ → Advanced**
   ```
   Compile As: Compile as C Code (/TC)
   ```

5. **Configuration Properties → Linker → Advanced**
   ```
   Control Flow Guard: Guard Control Flow security checks (/guard:cf)
   ```

6. **Configuration Properties → Inf2Cat → General**
   ```
   Use Local Time: Yes
   ```

### Шаг 4: Сборка

1. Выберите конфигурацию:
   ```
   Configuration: Release (или Debug для отладки)
   Platform: x64
   ```

2. Build → Build Solution (Ctrl+Shift+B)

3. Проверьте вывод:
   ```
   Output: x64\Release\HVCIDriver.sys
           x64\Release\HVCIDriver.inf
           x64\Release\HVCIDriver.cat (если создан catalog)
   ```

### Шаг 5: Подписание для тестирования

#### Вариант A: Test Signing (для разработки)

1. Создайте тестовый сертификат:
   ```cmd
   makecert -r -pe -ss PrivateCertStore -n "CN=TestDriverCert" TestCert.cer
   ```

2. Установите сертификат:
   ```cmd
   certmgr /add TestCert.cer /s /r localMachine root
   certmgr /add TestCert.cer /s /r localMachine trustedpublisher
   ```

3. Подпишите драйвер:
   ```cmd
   signtool sign /v /s PrivateCertStore /n "TestDriverCert" /t http://timestamp.digicert.com HVCIDriver.sys
   ```

4. Проверьте подпись:
   ```cmd
   signtool verify /v /pa HVCIDriver.sys
   ```

#### Вариант B: Self-Signed Certificate

```powershell
# PowerShell (as Administrator)

# Создание сертификата
$cert = New-SelfSignedCertificate -Type CodeSigningCert -Subject "CN=HVCI Driver Test" -CertStoreLocation Cert:\LocalMachine\My

# Экспорт в файл
Export-Certificate -Cert $cert -FilePath "HVCIDriverCert.cer"

# Установка в Trust Stores
Import-Certificate -FilePath "HVCIDriverCert.cer" -CertStoreLocation Cert:\LocalMachine\Root
Import-Certificate -FilePath "HVCIDriverCert.cer" -CertStoreLocation Cert:\LocalMachine\TrustedPublisher

# Подписание
$certThumbprint = $cert.Thumbprint
& "C:\Program Files (x86)\Windows Kits\10\bin\x64\signtool.exe" sign /fd sha256 /sha1 $certThumbprint /t http://timestamp.digicert.com /v "HVCIDriver.sys"
```

---

## 🧪 Тестирование

### Подготовка тестовой системы

**⚠️ ВАЖНО: Используйте виртуальную машину!**

1. Создайте VM с Windows 10/11:
   ```
   VMware:
   - RAM: 4GB+
   - CPU: 2 cores+
   - Enable VT-x/AMD-V
   ```

2. Включите Test Signing Mode:
   ```cmd
   bcdedit /set testsigning on
   bcdedit /set loadoptions DISABLE_INTEGRITY_CHECKS
   ```

3. Перезагрузите VM

4. Проверьте статус:
   ```cmd
   bcdedit /enum {current}
   # Должно быть: testsigning = Yes
   ```

### Установка драйвера

#### Метод 1: Через Device Manager (рекомендуется)

1. Скопируйте файлы на VM:
   ```
   HVCIDriver.sys
   HVCIDriver.inf
   HVCIDriver.cat (если есть)
   ```

2. Откройте Device Manager:
   ```
   devmgmt.msc
   ```

3. Action → Add legacy hardware → Next
4. Install the hardware that I manually select → Next
5. System devices → Next
6. Have Disk → Browse → выберите `HVCIDriver.inf`
7. Next → Install
8. Подтвердите установку неподписанного драйвера (если test signing)

#### Метод 2: Через командную строку

```cmd
:: Копирование в system32\drivers
copy HVCIDriver.sys C:\Windows\System32\drivers\

:: Создание службы
sc create HVCIDriver type= kernel start= demand binPath= C:\Windows\System32\drivers\HVCIDriver.sys

:: Запуск службы
sc start HVCIDriver

:: Проверка статуса
sc query HVCIDriver
```

#### Метод 3: Через devcon

```cmd
devcon install HVCIDriver.inf Root\HVCIDriver
devcon enable Root\HVCIDriver
```

### Запуск тестового приложения

1. Скомпилируйте `TestApp.c`:
   ```cmd
   cl TestApp.c
   ```

2. Запустите от имени администратора:
   ```cmd
   TestApp.exe
   ```

3. Ожидаемый вывод:
   ```
   ======================================================
    HVCI-Compatible Driver Test Application
    Educational Cybersecurity Research Tool
   ======================================================

   [+] Driver opened successfully

   === Driver Version ===
   [+] Driver Version: 1.0

   === Driver Integrity Check ===
   [+] Driver Integrity: VALID

   === System HWID Information ===
   [+] HWID information retrieved successfully

   --- Disk Information ---
   Disk Count: 1
     Disk 0:
       Device: \Device\Harddisk0\Partition0
       Serial: 1234567890

   --- Network Adapter Information ---
   Adapter Count: 1
     Adapter 0:
       Name: \Device\{GUID...}
       Physical: Yes

   --- SMBIOS Information ---
     Manufacturer: Dell Inc.
     Product Name: OptiPlex 7090
     Serial Number: ABCD1234
     UUID: 44454C4C-3400-1048-8052-B4C04F503432

   --- TPM Information ---
     Present: Yes
     Version: 2.0

   --- Metadata ---
     Data Version: 1
     Collection Time: 133518912000000000

   ======================================================
    Test completed. Press any key to exit...
   ======================================================
   ```

### Отладка

#### Просмотр логов драйвера

1. Используйте DebugView (Sysinternals):
   ```
   https://learn.microsoft.com/en-us/sysinternals/downloads/debugview
   ```

2. Включите Capture Kernel
3. Фильтр: `HV CI-Driver`

#### Kernel Debugging

1. Настройка VM для kernel debugging:
   ```cmd
   bcdedit /debug on
   bcdedit /dbgsettings serial debugport:1 baudrate:115200
   ```

2. В VMware добавьте Serial Port:
   ```
   Named pipe: \\.\pipe\com_1
   ```

3. Подключите WinDbg:
   ```
   File → Kernel Debug → COM
   Port: com1
   Baud: 115200
   ```

4. Установите breakpoints:
   ```
   kd> bu HVCIDriver!DriverEntry
   kd> bu HVCIDriver!DeviceControl
   kd> g
   ```

---

## 🛡️ HVCI совместимость

### Проверка совместимости

#### Проверка 1: Загрузка при включенном HVCI

```cmd
:: Включите HVCI
reg add "HKLM\SYSTEM\CurrentControlSet\Control\DeviceGuard\Scenarios\HypervisorEnforcedCodeIntegrity" /v "Enabled" /t REG_DWORD /d 1 /f

:: Перезагрузка
shutdown /r /t 0

:: После загрузки проверьте статус
msinfo32
# System Information → Virtualization-based security → Hypervisor enforced Code Integrity: Running
```

Если драйвер **загружается и работает** при HVCI = Running - он совместим! ✅

#### Проверка 2: Driver Verifier

```cmd
:: Включите Driver Verifier для HVCIDriver
verifier /standard /driver HVCIDriver.sys

:: Перезагрузка
shutdown /r /t 0

:: Тестирование (если не BSOD = хорошо)
TestApp.exe

:: Отключение Verifier
verifier /reset
```

#### Проверка 3: Static Analysis

В Visual Studio:

1. Analyze → Run Code Analysis on Solution
2. Проверьте вывод Error List
3. Устраните все warnings уровня C6XXX и C28XXX

### Типичные проблемы HVCI

| Проблема | Причина | Решение |
|----------|---------|---------|
| `KERNEL_SECURITY_CHECK_FAILURE` | Попытка записи в RO память | Используйте только чтение |
| `STATUS_INVALID_IMAGE_HASH` | Нарушение целостности кода | Не используйте self-modifying code |
| Драйвер не загружается | Не подписан / неправильная подпись | Используйте test signing или WHQL |
| `STATUS_ACCESS_VIOLATION` | Доступ к защищенной памяти | Используйте официальные API |

---

## 📚 Образовательные цели

### Что вы изучите с этим проектом:

1. **Windows Kernel Architecture**
   - IRQL levels и их важность
   - IRP (I/O Request Packets) processing
   - Device Object и Symbolic Link

2. **HVCI и VBS (Virtualization-Based Security)**
   - Архитектура VTL 0 / VTL 1
   - EPT (Extended Page Tables) protection
   - W^X enforcement

3. **Правильное использование API**
   - `ZwQuerySystemInformation`
   - `IoGetDeviceObjectPointer`
   - Storage и Network APIs

4. **Security Best Practices**
   - SAL (Source-code Annotation Language)
   - Static analysis
   - Defensive programming

5. **Driver Development Lifecycle**
   - Build → Sign → Install → Test → Debug

### Рекомендуемая литература:

- **"Windows Internals"** - Mark Russinovich
- **"Windows Kernel Programming"** - Pavel Yosifovich
- **"Rootkits and Bootkits"** - Alex Matrosov
- Microsoft Docs: Windows Driver Kit

### Следующие шаги:

1. **Добавьте функционал:**
   - Мониторинг изменений HWID в real-time
   - Детект аномалий (например, обнуление UUID)
   - Интеграция с ETW (Event Tracing for Windows)

2. **Улучшите безопасность:**
   - Implement Code Integrity checks
   - Add ACL verification для device object
   - Implement rate limiting для IOCTL

3. **Расширьте сбор данных:**
   - CPU information (CPUID, microcode version)
   - Firmware информация (UEFI variables)
   - PCIe device topology

---

## 🤝 Вклад в образовательное сообщество

Этот проект может быть использован в:

- **Курсовых работах** по кибербезопасности
- **Дипломных проектах** по системному программированию
- **Исследовательских работах** на темы:
  - "Современные методы защиты Windows"
  - "HVCI и его влияние на kernel development"
  - "Детектирование аномалий в HWID"

### Идеи для курсовых/дипломных:

1. **"Анализ эффективности HVCI против kernel-mode malware"**
   - Сравнение традиционных и HVCI-защищенных систем
   - Тестирование различных векторов атак

2. **"Разработка системы мониторинга целостности HWID"**
   - Реализация continuous monitoring
   - ML-based anomaly detection

3. **"Исследование TPM 2.0 Attestation для защиты игр"**
   - Интеграция TPM Quote в античит
   - Проверка практической реализуемости

---

## 📞 Поддержка

Если у вас возникли вопросы по образовательному использованию:

1. Изучите [Microsoft Driver Development Docs](https://learn.microsoft.com/en-us/windows-hardware/drivers/)
2. Посетите [OSR Online Forums](https://community.osr.com/)
3. Изучите примеры из [Windows Driver Samples](https://github.com/microsoft/Windows-driver-samples)

---

## ⚖️ Лицензия

Этот проект предназначен **только для образовательных целей**.

- ✅ Можно: изучать, модифицировать, использовать в учебных проектах
- ❌ Нельзя: использовать для нарушения ToS, обхода защит, коммерческих целей

**Автор не несет ответственности за неправомерное использование кода.**

---

## 🎓 Заключение

Вы создали **современный, HVCI-совместимый kernel driver**, который:

✅ Работает на Windows 11 с включенным HVCI
✅ Использует только легальные методы
✅ Следует Microsoft best practices
✅ Подходит для образовательных исследований

**Это именно то, что должно быть в курсовой работе по кибербезопасности!**

Успехов в изучении Windows Kernel Development! 🚀
