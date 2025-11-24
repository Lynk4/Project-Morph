# In-memory basic polymorphic virus...............

---

## Features 

- Downloads from remote server (HTTP)
- No disk write — 100% memory
- Native API + direct syscalls 
- (NtAllocate/NtWrite/NtProtect/NtCreateThreadEx)
- 5 polymorphic engines + junk
- No CreateThread, no WinExec, no WriteFile

---

## Cross Compilation...

```bash
cl mk-2-polymorph_remote.c /O2 /link user32.lib wininet.lib
```

---



