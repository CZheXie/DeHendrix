# SATURN-style deobfuscation report

- Binary: `ACE-BASE.sys`
- Entry: `0x14043ec4e`
- Method: angr VEX-IR symbolic eval, RSP concretized, others symbolic
- Blocks recovered: 6

## Recovered Control Flow

| # | Block VA | Size | Insns | Last insn | Successor |
|---|---|---|---|---|---|
| 0 | `0x14043ec4e` | 82 | 23 | `jmp rcx` | `0x14043f340` |
| 1 | `0x14043f340` | 100 | 29 | `jmp rdx` | `0x14043f2ea` |
| 2 | `0x14043f2ea` | 85 | 24 | `jmp r8` | `0x14043f097` |
| 3 | `0x14043f097` | 92 | 26 | `jmp r9` | `0x14043f3a5` |
| 4 | `0x14043f3a5` | 70 | 15 | `call 0x1404cc11c` | `0x14043f3eb` |
| 5 | `0x14043f3eb` | 1 | 1 | `int3 ` | `0x14043f3ec` |

## Detailed Block Listings

### Block #0 @ `0x14043ec4e`

```
0x14043ec4e  488da42480fdffff          lea        rsp, [rsp - 0x280]
0x14043ec56  48896c2410                mov        qword ptr [rsp + 0x10], rbp
0x14043ec5b  4889e5                    mov        rbp, rsp
0x14043ec5e  9c                        pushfq     
0x14043ec5f  8f4500                    pop        qword ptr [rbp]
0x14043ec62  4c897578                  mov        qword ptr [rbp + 0x78], r14
0x14043ec66  49f7d0                    not        r8
0x14043ec69  4c87c2                    xchg       rdx, r8
0x14043ec6c  4c894530                  mov        qword ptr [rbp + 0x30], r8
0x14043ec70  48f7d2                    not        rdx
0x14043ec73  4c87c2                    xchg       rdx, r8
0x14043ec76  49f7d0                    not        r8
0x14043ec79  4d87c1                    xchg       r9, r8
0x14043ec7c  4c894550                  mov        qword ptr [rbp + 0x50], r8
0x14043ec80  49f7d1                    not        r9
0x14043ec83  4d87c1                    xchg       r9, r8
0x14043ec86  49f7d1                    not        r9
0x14043ec89  4991                      xchg       r9, rax
0x14043ec8b  4c894d08                  mov        qword ptr [rbp + 8], r9
0x14043ec8f  51                        push       rcx
0x14043ec90  488d0d17b29cfd            lea        rcx, [rip - 0x2634de9]
0x14043ec97  488d8992546302            lea        rcx, [rcx + 0x2635492]
0x14043ec9e  ffe1                      jmp        rcx
```

### Block #1 @ `0x14043f340`

```
0x14043f340  59                        pop        rcx
0x14043f341  48f7d0                    not        rax
0x14043f344  4991                      xchg       r9, rax
0x14043f346  49f7d2                    not        r10
0x14043f349  4d87d3                    xchg       r11, r10
0x14043f34c  4c895560                  mov        qword ptr [rbp + 0x60], r10
0x14043f350  49f7d3                    not        r11
0x14043f353  4d87d3                    xchg       r11, r10
0x14043f356  48895d18                  mov        qword ptr [rbp + 0x18], rbx
0x14043f35a  48896540                  mov        qword ptr [rbp + 0x40], rsp
0x14043f35e  48897d28                  mov        qword ptr [rbp + 0x28], rdi
0x14043f362  4c896d70                  mov        qword ptr [rbp + 0x70], r13
0x14043f366  49f7d3                    not        r11
0x14043f369  4d87d8                    xchg       r8, r11
0x14043f36c  4c895d48                  mov        qword ptr [rbp + 0x48], r11
0x14043f370  49f7d0                    not        r8
0x14043f373  4d87d8                    xchg       r8, r11
0x14043f376  4c89bd80000000            mov        qword ptr [rbp + 0x80], r15
0x14043f37d  48f7d0                    not        rax
0x14043f380  4992                      xchg       r10, rax
0x14043f382  48894558                  mov        qword ptr [rbp + 0x58], rax
0x14043f386  49f7d2                    not        r10
0x14043f389  4992                      xchg       r10, rax
0x14043f38b  48897538                  mov        qword ptr [rbp + 0x38], rsi
0x14043f38f  4c896568                  mov        qword ptr [rbp + 0x68], r12
0x14043f393  52                        push       rdx
0x14043f394  488d15dc849f58            lea        rdx, [rip + 0x589f84dc]
0x14043f39b  488d92737a60a7            lea        rdx, [rdx - 0x589f858d]
0x14043f3a2  ffe2                      jmp        rdx
```

### Block #2 @ `0x14043f2ea`

```
0x14043f2ea  5a                        pop        rdx
0x14043f2eb  48f7d2                    not        rdx
0x14043f2ee  4887d1                    xchg       rcx, rdx
0x14043f2f1  48895520                  mov        qword ptr [rbp + 0x20], rdx
0x14043f2f5  48f7d1                    not        rcx
0x14043f2f8  4887d1                    xchg       rcx, rdx
0x14043f2fb  9c                        pushfq     
0x14043f2fc  4881454080020000          add        qword ptr [rbp + 0x40], 0x280
0x14043f304  9d                        popfq      
0x14043f305  4c8d9d90000000            lea        r11, [rbp + 0x90]
0x14043f30c  ff7508                    push       qword ptr [rbp + 8]
0x14043f30f  418f03                    pop        qword ptr [r11]
0x14043f312  ff7518                    push       qword ptr [rbp + 0x18]
0x14043f315  418f4308                  pop        qword ptr [r11 + 8]
0x14043f319  ff7520                    push       qword ptr [rbp + 0x20]
0x14043f31c  418f4310                  pop        qword ptr [r11 + 0x10]
0x14043f320  ff7530                    push       qword ptr [rbp + 0x30]
0x14043f323  418f4318                  pop        qword ptr [r11 + 0x18]
0x14043f327  4150                      push       r8
0x14043f329  49b8c86bd2b801000000      movabs     r8, 0x1b8d26bc8
0x14043f333  9c                        pushfq     
0x14043f334  4981c0cf847187            add        r8, -0x788e7b31
0x14043f33b  9d                        popfq      
0x14043f33c  41ffe0                    jmp        r8
```

### Block #3 @ `0x14043f097`

```
0x14043f097  4158                      pop        r8
0x14043f099  ff7540                    push       qword ptr [rbp + 0x40]
0x14043f09c  418f4320                  pop        qword ptr [r11 + 0x20]
0x14043f0a0  ff7510                    push       qword ptr [rbp + 0x10]
0x14043f0a3  418f4328                  pop        qword ptr [r11 + 0x28]
0x14043f0a7  ff7538                    push       qword ptr [rbp + 0x38]
0x14043f0aa  418f4330                  pop        qword ptr [r11 + 0x30]
0x14043f0ae  ff7528                    push       qword ptr [rbp + 0x28]
0x14043f0b1  418f4338                  pop        qword ptr [r11 + 0x38]
0x14043f0b5  ff7548                    push       qword ptr [rbp + 0x48]
0x14043f0b8  418f4340                  pop        qword ptr [r11 + 0x40]
0x14043f0bc  ff7550                    push       qword ptr [rbp + 0x50]
0x14043f0bf  418f4348                  pop        qword ptr [r11 + 0x48]
0x14043f0c3  ff7558                    push       qword ptr [rbp + 0x58]
0x14043f0c6  418f4350                  pop        qword ptr [r11 + 0x50]
0x14043f0ca  ff7560                    push       qword ptr [rbp + 0x60]
0x14043f0cd  418f4358                  pop        qword ptr [r11 + 0x58]
0x14043f0d1  ff7568                    push       qword ptr [rbp + 0x68]
0x14043f0d4  418f4360                  pop        qword ptr [r11 + 0x60]
0x14043f0d8  ff7570                    push       qword ptr [rbp + 0x70]
0x14043f0db  4151                      push       r9
0x14043f0dd  49b96a0c983001000000      movabs     r9, 0x130980c6a
0x14043f0e7  9c                        pushfq     
0x14043f0e8  4981c13be7ab0f            add        r9, 0xfabe73b
0x14043f0ef  9d                        popfq      
0x14043f0f0  41ffe1                    jmp        r9
```

### Block #4 @ `0x14043f3a5`

```
0x14043f3a5  4159                      pop        r9
0x14043f3a7  418f4368                  pop        qword ptr [r11 + 0x68]
0x14043f3ab  ff7578                    push       qword ptr [rbp + 0x78]
0x14043f3ae  418f4370                  pop        qword ptr [r11 + 0x70]
0x14043f3b2  ffb580000000              push       qword ptr [rbp + 0x80]
0x14043f3b8  418f4378                  pop        qword ptr [r11 + 0x78]
0x14043f3bc  ff7500                    push       qword ptr [rbp]
0x14043f3bf  418f8380000000            pop        qword ptr [r11 + 0x80]
0x14043f3c6  4c8d1d82d9f7ff            lea        r11, [rip - 0x8267e]
0x14043f3cd  4c8d9588000000            lea        r10, [rbp + 0x88]
0x14043f3d4  488d6424f8                lea        rsp, [rsp - 8]
0x14043f3d9  4c891424                  mov        qword ptr [rsp], r10
0x14043f3dd  488d6424f8                lea        rsp, [rsp - 8]
0x14043f3e2  4c891c24                  mov        qword ptr [rsp], r11
0x14043f3e6  e831cd0800                call       0x1404cc11c
```

### Block #5 @ `0x14043f3eb`

```
0x14043f3eb  cc                        int3       
```
