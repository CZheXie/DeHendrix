# Cross-section disasm @ 0x1403b7000

- Binary: `ACE-BASE.sys`
- Entry: `0x1403b7000` (section `INIT`)
- BBs: 7
- Total insns: 99

## IAT call inventory

Distinct IAT functions: **4** | total call sites: **7**

| Site | Slot VA | DLL | Function |
|---|---|---|---|
| `0x140044f6a` (`.text`) | `0x1400ce718` | ntoskrnl.exe | **RtlInitUnicodeString** |
| `0x140044f7e` (`.text`) | `0x1400ce718` | ntoskrnl.exe | **RtlInitUnicodeString** |
| `0x140044f92` (`.text`) | `0x1400ce718` | ntoskrnl.exe | **RtlInitUnicodeString** |
| `0x140044fba` (`.text`) | `0x1400ce718` | ntoskrnl.exe | **RtlInitUnicodeString** |
| `0x14004500d` (`.text`) | `0x1400ce2c0` | ntoskrnl.exe | **IoCreateSymbolicLink** |
| `0x14004504e` (`.text`) | `0x1400ce288` | ntoskrnl.exe | **IoDeleteSymbolicLink** |
| `0x140045061` (`.text`) | `0x1400ce280` | ntoskrnl.exe | **IoDeleteDevice** |

### IAT call frequency

| Function | Count |
|---|---|
| `RtlInitUnicodeString` | 4 |
| `IoCreateSymbolicLink` | 1 |
| `IoDeleteDevice` | 1 |
| `IoDeleteSymbolicLink` | 1 |

## Direct call inventory

Distinct targets: **5** | total call sites: **5**

| Site | Section | Target | Target section |
|---|---|---|---|
| `0x140044fa9` | `.text` | `0x140044450` | `.text` |
| `0x140044ff6` | `.text` | `0x1403b506c` | `PAGE` |
| `0x14004501f` | `.text` | `0x1400448a0` | `.text` |
| `0x140045077` | `.text` | `0x140014f10` | `.text` |
| `0x1403b7010` | `INIT` | `0x1403b702c` | `INIT` |

## Indirect calls / jumps

- Indirect calls: 0
- Indirect jumps: 0

## Anti-debug indicators

- `rdtsc` sites: **0** -> []
- DR0-DR7 accesses: **0** -> []
- CR0-CR4 accesses: **0** -> []

## BB listing (asm)

```x86asm
; --- Block @ 0x140044f00 (.text) ---
0x140044f00  4c8bdc                    mov      r11, rsp
0x140044f03  49895b10                  mov      qword ptr [r11 + 0x10], rbx
0x140044f07  49897318                  mov      qword ptr [r11 + 0x18], rsi
0x140044f0b  57                        push     rdi
0x140044f0c  4881ec80000000            sub      rsp, 0x80
0x140044f13  488d0506f7ffff            lea      rax, [rip - 0x8fa]
0x140044f1a  488bf2                    mov      rsi, rdx
0x140044f1d  48894170                  mov      qword ptr [rcx + 0x70], rax
0x140044f21  488d15486d0800            lea      rdx, [rip + 0x86d48]
0x140044f28  488d0541f6ffff            lea      rax, [rip - 0x9bf]
0x140044f2f  488bf9                    mov      rdi, rcx
0x140044f32  48898180000000            mov      qword ptr [rcx + 0x80], rax
0x140044f39  33db                      xor      ebx, ebx
0x140044f3b  488d05fef7ffff            lea      rax, [rip - 0x802]
0x140044f42  49895b08                  mov      qword ptr [r11 + 8], rbx
0x140044f46  488981e0000000            mov      qword ptr [rcx + 0xe0], rax
0x140044f4d  488d056cffffff            lea      rax, [rip - 0x94]
0x140044f54  488981f0000000            mov      qword ptr [rcx + 0xf0], rax
0x140044f5b  488d05aefdffff            lea      rax, [rip - 0x252]
0x140044f62  48894168                  mov      qword ptr [rcx + 0x68], rax
0x140044f66  498d4bc8                  lea      rcx, [r11 - 0x38]
0x140044f6a  ff15a8970800              call     qword ptr [rip + 0x897a8]
0x140044f70  488d15b96c0800            lea      rdx, [rip + 0x86cb9]
0x140044f77  488d0da23d1b00            lea      rcx, [rip + 0x1b3da2]
0x140044f7e  ff1594970800              call     qword ptr [rip + 0x89794]
0x140044f84  488d15156d0800            lea      rdx, [rip + 0x86d15]
0x140044f8b  488d0d7e3d1b00            lea      rcx, [rip + 0x1b3d7e]
0x140044f92  ff1580970800              call     qword ptr [rip + 0x89780]
0x140044f98  0f1005713d1b00            movups   xmm0, xmmword ptr [rip + 0x1b3d71]
0x140044f9f  488d4c2460                lea      rcx, [rsp + 0x60]
0x140044fa4  0f29442460                movaps   xmmword ptr [rsp + 0x60], xmm0
0x140044fa9  e8a2f4ffff                call     0x140044450
0x140044fae  488d15fb6b0800            lea      rdx, [rip + 0x86bfb]
0x140044fb5  488d4c2470                lea      rcx, [rsp + 0x70]
0x140044fba  ff1558970800              call     qword ptr [rip + 0x89758]
0x140044fc0  488d842490000000          lea      rax, [rsp + 0x90]
0x140044fc8  33d2                      xor      edx, edx
0x140044fca  4889442440                mov      qword ptr [rsp + 0x40], rax
0x140044fcf  448d4b22                  lea      r9d, [rbx + 0x22]
0x140044fd3  48895c2438                mov      qword ptr [rsp + 0x38], rbx
0x140044fd8  488d442470                lea      rax, [rsp + 0x70]
0x140044fdd  4889442430                mov      qword ptr [rsp + 0x30], rax
0x140044fe2  4c8d442450                lea      r8, [rsp + 0x50]
0x140044fe7  885c2428                  mov      byte ptr [rsp + 0x28], bl
0x140044feb  488bcf                    mov      rcx, rdi
0x140044fee  c744242000010000          mov      dword ptr [rsp + 0x20], 0x100
0x140044ff6  e871003700                call     0x1403b506c
0x140044ffb  8bd8                      mov      ebx, eax
0x140044ffd  85c0                      test     eax, eax
0x140044fff  7853                      js       0x140045054
0x140045001  488d542450                lea      rdx, [rsp + 0x50]
0x140045006  488d0d133d1b00            lea      rcx, [rip + 0x1b3d13]
0x14004500d  ff15ad920800              call     qword ptr [rip + 0x892ad]
0x140045013  8bd8                      mov      ebx, eax
0x140045015  85c0                      test     eax, eax
0x140045017  783b                      js       0x140045054
0x140045019  488bd6                    mov      rdx, rsi
0x14004501c  488bcf                    mov      rcx, rdi
0x14004501f  e87cf8ffff                call     0x1400448a0
0x140045024  8bd8                      mov      ebx, eax
0x140045026  85c0                      test     eax, eax
0x140045028  781d                      js       0x140045047
0x14004502a  488b842490000000          mov      rax, qword ptr [rsp + 0x90]
0x140045032  b902080000                mov      ecx, 0x802
0x140045037  488905fa3b1b00            mov      qword ptr [rip + 0x1b3bfa], rax
0x14004503e  48893deb3b1b00            mov      qword ptr [rip + 0x1b3beb], rdi
0x140045045  eb30                      jmp      0x140045077

; --- Block @ 0x140045047 (.text) ---
0x140045047  488d0dd23c1b00            lea      rcx, [rip + 0x1b3cd2]
0x14004504e  ff1534920800              call     qword ptr [rip + 0x89234]

; --- Block @ 0x140045054 (.text) ---
0x140045054  488b8c2490000000          mov      rcx, qword ptr [rsp + 0x90]
0x14004505c  4885c9                    test     rcx, rcx
0x14004505f  7406                      je       0x140045067
0x140045061  ff1519920800              call     qword ptr [rip + 0x89219]
0x140045067  85db                      test     ebx, ebx
0x140045069  7807                      js       0x140045072
0x14004506b  b902080000                mov      ecx, 0x802
0x140045070  eb05                      jmp      0x140045077

; --- Block @ 0x140045067 (.text) ---
0x140045067  85db                      test     ebx, ebx

; --- Block @ 0x140045072 (.text) ---
0x140045072  b903080000                mov      ecx, 0x803

; --- Block @ 0x140045077 (.text) ---
0x140045077  e894fefcff                call     0x140014f10
0x14004507c  4c8d9c2480000000          lea      r11, [rsp + 0x80]
0x140045084  8bc3                      mov      eax, ebx
0x140045086  498b5b18                  mov      rbx, qword ptr [r11 + 0x18]
0x14004508a  498b7320                  mov      rsi, qword ptr [r11 + 0x20]
0x14004508e  498be3                    mov      rsp, r11
0x140045091  5f                        pop      rdi
0x140045092  c3                        ret      

; --- Block @ 0x1403b7000 (INIT) ---
0x1403b7000  48895c2408                mov      qword ptr [rsp + 8], rbx
0x1403b7005  57                        push     rdi
0x1403b7006  4883ec20                  sub      rsp, 0x20
0x1403b700a  488bda                    mov      rbx, rdx
0x1403b700d  488bf9                    mov      rdi, rcx
0x1403b7010  e817000000                call     0x1403b702c
0x1403b7015  488bd3                    mov      rdx, rbx
0x1403b7018  488bcf                    mov      rcx, rdi
0x1403b701b  488b5c2430                mov      rbx, qword ptr [rsp + 0x30]
0x1403b7020  4883c420                  add      rsp, 0x20
0x1403b7024  5f                        pop      rdi
0x1403b7025  e9d6dec8ff                jmp      0x140044f00

```