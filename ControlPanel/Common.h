#pragma once

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>

//=============================================================================
// Common Definitions
//=============================================================================

#define APP_TITLE "Security Research Control Panel"
#define APP_VERSION "1.0"

// Console colors
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_WHITE   "\033[37m"
#define COLOR_BRIGHT_RED     "\033[91m"
#define COLOR_BRIGHT_GREEN   "\033[92m"
#define COLOR_BRIGHT_YELLOW  "\033[93m"
#define COLOR_BRIGHT_BLUE    "\033[94m"
#define COLOR_BRIGHT_CYAN    "\033[96m"

//=============================================================================
// Utility Functions
//=============================================================================

inline void ClearScreen() {
    system("cls");
}

inline void PressAnyKey() {
    printf("\nPress any key to continue...");
    _getch();
}

inline void PrintSeparator(const char* symbol = "=", int length = 70) {
    for (int i = 0; i < length; i++) {
        printf("%s", symbol);
    }
    printf("\n");
}

inline void PrintHeader(const char* title) {
    ClearScreen();
    printf("%s", COLOR_BRIGHT_CYAN);
    PrintSeparator("=", 70);
    printf("%s%s%s\n", COLOR_BRIGHT_YELLOW, title, COLOR_RESET);
    PrintSeparator("=", 70);
    printf("\n");
}

inline void PrintSuccess(const char* message) {
    printf("%s[+] %s%s\n", COLOR_BRIGHT_GREEN, message, COLOR_RESET);
}

inline void PrintError(const char* message) {
    printf("%s[!] %s%s\n", COLOR_BRIGHT_RED, message, COLOR_RESET);
}

inline void PrintWarning(const char* message) {
    printf("%s[*] %s%s\n", COLOR_BRIGHT_YELLOW, message, COLOR_RESET);
}

inline void PrintInfo(const char* message) {
    printf("%s[i] %s%s\n", COLOR_BRIGHT_BLUE, message, COLOR_RESET);
}

inline BOOL IsAdmin() {
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

    return isAdmin;
}

inline void CheckAdminRights() {
    if (!IsAdmin()) {
        PrintWarning("Not running as Administrator!");
        PrintWarning("Some features may not work properly.");
        printf("\n");
    }
}

inline void ShowDisclaimer() {
    ClearScreen();
    printf("%s", COLOR_BRIGHT_RED);
    PrintSeparator("!", 70);
    printf("                         EDUCATIONAL USE ONLY\n");
    PrintSeparator("!", 70);
    printf("%s", COLOR_RESET);

    printf("\n");
    printf("This software is provided for %sEDUCATIONAL PURPOSES ONLY%s.\n\n",
           COLOR_BRIGHT_YELLOW, COLOR_RESET);

    printf("Authorized uses:\n");
    printf("  %s[+]%s Learning cybersecurity concepts\n", COLOR_GREEN, COLOR_RESET);
    printf("  %s[+]%s Authorized penetration testing\n", COLOR_GREEN, COLOR_RESET);
    printf("  %s[+]%s Academic research and coursework\n", COLOR_GREEN, COLOR_RESET);
    printf("  %s[+]%s Security training and certification\n", COLOR_GREEN, COLOR_RESET);

    printf("\n");
    printf("Prohibited uses:\n");
    printf("  %s[-]%s Bypassing anti-cheat systems\n", COLOR_RED, COLOR_RESET);
    printf("  %s[-]%s Unauthorized system modification\n", COLOR_RED, COLOR_RESET);
    printf("  %s[-]%s Any illegal activities\n", COLOR_RED, COLOR_RESET);

    printf("\n");
    printf("%sBy proceeding, you acknowledge that:%s\n", COLOR_BRIGHT_CYAN, COLOR_RESET);
    printf("  1. You have proper authorization\n");
    printf("  2. You understand the legal implications\n");
    printf("  3. You accept full responsibility for your actions\n");

    printf("\n");
    PrintSeparator("-", 70);
    printf("\n");

    printf("Do you agree to use this software responsibly? (Y/N): ");

    char response = _getch();
    printf("%c\n", response);

    if (response != 'Y' && response != 'y') {
        printf("\n%sExiting...%s\n", COLOR_RED, COLOR_RESET);
        exit(0);
    }
}

//=============================================================================
// Driver Communication Helper
//=============================================================================

inline BOOL OpenDriver(const wchar_t* driverPath, HANDLE* phDriver) {
    *phDriver = CreateFileW(
        driverPath,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    return (*phDriver != INVALID_HANDLE_VALUE);
}

inline void CloseDriver(HANDLE hDriver) {
    if (hDriver != INVALID_HANDLE_VALUE) {
        CloseHandle(hDriver);
    }
}

inline BOOL SendDriverIOCTL(
    HANDLE hDriver,
    DWORD ioctl,
    PVOID inputBuffer,
    DWORD inputSize,
    PVOID outputBuffer,
    DWORD outputSize,
    PDWORD bytesReturned
) {
    return DeviceIoControl(
        hDriver,
        ioctl,
        inputBuffer,
        inputSize,
        outputBuffer,
        outputSize,
        bytesReturned,
        NULL
    );
}

//=============================================================================
// Menu Helper
//=============================================================================

inline int GetMenuChoice(int min, int max) {
    int choice;

    while (1) {
        printf("\n%sEnter choice (%d-%d): %s", COLOR_CYAN, min, max, COLOR_RESET);

        if (scanf_s("%d", &choice) == 1) {
            // Clear input buffer
            int c;
            while ((c = getchar()) != '\n' && c != EOF);

            if (choice >= min && choice <= max) {
                return choice;
            }
        } else {
            // Clear input buffer on invalid input
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
        }

        PrintError("Invalid choice. Please try again.");
    }
}

//=============================================================================
// Component Status
//=============================================================================

typedef enum _COMPONENT_STATUS {
    STATUS_UNKNOWN,
    STATUS_NOT_LOADED,
    STATUS_LOADED,
    STATUS_RUNNING,
    STATUS_ERROR
} COMPONENT_STATUS;

inline const char* GetStatusString(COMPONENT_STATUS status) {
    switch (status) {
        case STATUS_LOADED:
        case STATUS_RUNNING:
            return COLOR_BRIGHT_GREEN "LOADED" COLOR_RESET;
        case STATUS_NOT_LOADED:
            return COLOR_YELLOW "NOT LOADED" COLOR_RESET;
        case STATUS_ERROR:
            return COLOR_BRIGHT_RED "ERROR" COLOR_RESET;
        default:
            return COLOR_CYAN "UNKNOWN" COLOR_RESET;
    }
}

//=============================================================================
// Educational Information
//=============================================================================

inline void ShowComponentInfo(const char* componentName, const char* purpose, const char* approach) {
    printf("\n%s=== %s ===%s\n", COLOR_BRIGHT_CYAN, componentName, COLOR_RESET);
    printf("%sPurpose:%s %s\n", COLOR_BRIGHT_YELLOW, COLOR_RESET, purpose);
    printf("%sApproach:%s %s\n", COLOR_BRIGHT_YELLOW, COLOR_RESET, approach);
    printf("\n");
}
