# Quick Start Guide

Быстрая инструкция для запуска HVCI-compatible драйвера.

## 🚀 За 5 минут

### Шаг 1: Проверьте требования

```cmd
:: Проверьте версию Windows
winver
# Нужно: Windows 10 1607+ или Windows 11

:: Проверьте наличие WDK
dir "C:\Program Files (x86)\Windows Kits\10\bin\x64\inf2cat.exe"
# Если нет - установите WDK: https://go.microsoft.com/fwlink/?linkid=2249371
```

### Шаг 2: Скомпилируйте драйвер

```cmd
:: В Visual Studio 2022
1. File → Open → Project/Solution
2. Выберите HVCIDriver.sln (создайте проект из файлов)
3. Build → Configuration Manager → Release, x64
4. Build → Build Solution (Ctrl+Shift+B)

:: Вывод будет в:
# x64\Release\HVCIDriver.sys
```

### Шаг 3: Подпишите драйвер (test signing)

```powershell
# PowerShell as Administrator

# Создание self-signed cert
$cert = New-SelfSignedCertificate -Type CodeSigningCert -Subject "CN=Test" -CertStoreLocation Cert:\LocalMachine\My

# Установка в trust stores
Export-Certificate -Cert $cert -FilePath "test.cer"
Import-Certificate -FilePath "test.cer" -CertStoreLocation Cert:\LocalMachine\Root
Import-Certificate -FilePath "test.cer" -CertStoreLocation Cert:\LocalMachine\TrustedPublisher

# Подпись
$thumb = $cert.Thumbprint
& "C:\Program Files (x86)\Windows Kits\10\bin\x64\signtool.exe" sign /fd sha256 /sha1 $thumb /v "x64\Release\HVCIDriver.sys"
```

### Шаг 4: Включите test signing

```cmd
:: Administrator Command Prompt
bcdedit /set testsigning on
shutdown /r /t 0
```

### Шаг 5: Установите драйвер

```cmd
:: После перезагрузки, Administrator CMD:

:: Метод A: SC (простой)
sc create HVCIDriver type=kernel start=demand binpath=C:\Path\To\HVCIDriver.sys
sc start HVCIDriver

:: Проверка
sc query HVCIDriver
# STATE = RUNNING ✅
```

### Шаг 6: Скомпилируйте тестовое приложение

```cmd
cl TestApp.c
```

### Шаг 7: Запустите тест

```cmd
TestApp.exe

:: Ожидаемый результат:
# [+] Driver opened successfully
# [+] Driver Version: 1.0
# [+] HWID information retrieved successfully
# ... (полный вывод HWID данных)
```

---

## ⚡ Troubleshooting

### Проблема: "Driver failed to start"

```cmd
:: Проверьте подпись
signtool verify /v /pa HVCIDriver.sys

:: Проверьте test signing
bcdedit /enum {current} | findstr testsigning
# testsigning = Yes ✅

:: Проверьте логи
eventvwr
# Windows Logs → System → фильтр по "HVCIDriver"
```

### Проблема: "TestApp can't open driver"

```cmd
:: Проверьте статус
sc query HVCIDriver
# STATE должен быть RUNNING

:: Проверьте symbolic link
dir \\.\HVCIDriver
# Если ошибка - перезапустите драйвер
```

### Проблема: BSOD при загрузке

```cmd
:: Отключите драйвер в Safe Mode:
1. Загрузитесь в Safe Mode (F8 при загрузке)
2. sc delete HVCIDriver
3. Перезагрузка

:: Проверьте совместимость с HVCI:
# В коде должны быть NonPagedPoolNx (не NonPagedPool)
# Нет модификации MajorFunction таблиц
# Нет прямой записи в kernel structures
```

---

## 📚 Дополнительные ресурсы

**Полная документация:**
- [README_HVCI.md](README_HVCI.md) - детальная инструкция
- [COMPARISON.md](COMPARISON.md) - сравнение со старым подходом

**Исходники:**
- `HVCIDriver.h` - заголовочный файл
- `HVCIDriver.c` - основная логика
- `HWIDCollector.c` - сбор HWID информации
- `TestApp.c` - тестовое приложение

**Официальная документация Microsoft:**
- [Windows Driver Kit](https://learn.microsoft.com/en-us/windows-hardware/drivers/)
- [HVCI and Driver Compatibility](https://learn.microsoft.com/en-us/windows-hardware/drivers/driversecurity/use-wdk-to-develop-hvci-compatible-drivers)

---

## 🎯 Для курсовой работы

### Что включить в отчет:

1. **Теоретическая часть:**
   - Описание HVCI архитектуры
   - Сравнение с традиционными методами
   - Анализ современных защит Windows

2. **Практическая часть:**
   - Схема архитектуры драйвера
   - Скриншоты работы (DebugView, TestApp вывод)
   - Анализ кода (ключевые фрагменты)

3. **Результаты:**
   - Таблица совместимости (HVCI on/off)
   - Бенчмарки производительности
   - Выводы о security implications

4. **Заключение:**
   - Оценка эффективности подхода
   - Ограничения метода
   - Направления развития

### Примеры скриншотов для отчета:

```
1. Статус HVCI в msinfo32
   └─ System Summary → Virtualization-based security

2. Вывод DebugView с логами драйвера
   └─ Capture Kernel, фильтр "HVCI-Driver"

3. Результат работы TestApp.exe
   └─ Полный вывод HWID информации

4. Driver Verifier результаты
   └─ verifier /query

5. Code Analysis в Visual Studio
   └─ 0 errors, 0 warnings
```

---

## ✅ Checklist для защиты курсовой

- [ ] Драйвер компилируется без ошибок
- [ ] Драйвер загружается на чистой Windows 10/11
- [ ] TestApp успешно получает HWID данные
- [ ] Драйвер работает при включенном HVCI
- [ ] Код прошел Code Analysis (0 критичных предупреждений)
- [ ] Документация заполнена (README.md)
- [ ] Скриншоты сделаны
- [ ] Теоретическая часть написана
- [ ] Выводы сформулированы

---

## 🆘 Если что-то не работает

1. **Прочитайте [README_HVCI.md](README_HVCI.md)** - там детальные инструкции
2. **Проверьте [COMPARISON.md](COMPARISON.md)** - возможно, вы используете старые методы
3. **Изучите логи** в DebugView / Event Viewer
4. **Проверьте HVCI статус** через `msinfo32`

---

**Время на выполнение:** ~1-2 часа (при наличии всех инструментов)

**Сложность:** Средняя (требуется опыт с Visual Studio и командной строкой)

**Результат:** Полностью функциональный HVCI-compatible kernel driver ✅

Удачи! 🚀
