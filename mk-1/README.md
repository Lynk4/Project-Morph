# 🧬 Polymorph.

---

## ⚡ 5-Engine Polymorphic Shellcode Launcher – Pure C

### 🧨 What it does.
- Loads **any** raw 64-bit shellcode (`payload-64.bin`)  
- Randomly picks **1 of 5 real polymorphic decryption engines**  
- Sprays **heavy junk code** before & after decryption  
- Executes your payload **perfectly** every time  
- **Every execution has a different hash, timing, and behavior**

---

## 🧪 The 5 Engines.


| # | Engine   | Technique Used                       |
|---|----------|---------------------------------------|
| 1 | XOR      | Classic XOR with random key           |
| 2 | ADD/SUB  | Incremental add + reverse subtract    |
| 3 | ROL      | Rotating left each byte               |
| 4 | Reverse  | Full payload reversal                 |
| 5 | NOT      | Bitwise invert (NOT)                  |


---

## 🧯 Generate a harmless MessageBox payload

```bash
msfvenom -p windows/x64/messagebox TEXT="HI......!!;)" TITLE="From Lynk4" -f raw > payload-64.bin
```
---


## 🏗️ Compilation

```cmd
cl mk-1-polymorph.c /O2 /link user32.lib
```

---

## 🔧 Results.

<img width="988" height="518" alt="Screenshot 2025-11-23 at 6 50 44 PM" src="https://github.com/user-attachments/assets/b389df6e-ec1f-49ce-9a9b-28db636968c8" />

---

<img width="981" height="407" alt="Screenshot 2025-11-25 at 9 30 16 AM" src="https://github.com/user-attachments/assets/1cf5dbe6-3a25-491a-9a08-678e37a88eec" />

---

## 🛡️ AV Detection.............

<img width="1285" height="730" alt="Screenshot 2025-11-23 at 5 01 57 PM" src="https://github.com/user-attachments/assets/3d09de9f-8c81-4881-b7c0-435bcafd3b96" />

---
