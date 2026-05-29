# Deobfuscation Chain Report

- Binary: `E:\ace\ACE-BASE.sys`
- Entry: `0x14043EC4E`
- Blocks: 1
- Total IR: 13
- Non-NOP: 13
- Side-effect: 7

## Optimized IR

```
func devirt_0x14043ec4e {
  %v2 = STORE #0x7fffffeffd90, %rbp  ; va=0x14043ec56
  %v4 = STORE #0x7fffffeffd78, %rflags
  %v8 = STORE #0x7fffffeffdf8, %r14  ; va=0x14043ec62
  %v9 = NOT %r8  ; va=0x14043ec66
  %v12 = STORE #0x7fffffeffdb0, %v9  ; va=0x14043ec6c
  %v15 = PASSTHROUGH %r8  ; va=0x14043ec76
  %v18 = STORE #0x7fffffeffdd0, %v15  ; va=0x14043ec7c
  %v21 = PASSTHROUGH %r9  ; va=0x14043ec86
  %v24 = STORE #0x7fffffeffd88, %v21  ; va=0x14043ec8b
  %v26 = STORE #0x7fffffeffd78, %rcx  ; va=0x14043ec8f
  %v27 = ADD %unk_reg_41, #0xfffffffffd9cb217
  %v28 = ADD %v27, #0x2635492
  %v29 = JMP %v28  ; va=0x14043ec9e
}
```

## Side-effect Instructions Only

```
  %v2 = STORE #0x7fffffeffd90, %rbp  ; va=0x14043ec56
  %v4 = STORE #0x7fffffeffd78, %rflags
  %v8 = STORE #0x7fffffeffdf8, %r14  ; va=0x14043ec62
  %v12 = STORE #0x7fffffeffdb0, %v9  ; va=0x14043ec6c
  %v18 = STORE #0x7fffffeffdd0, %v15  ; va=0x14043ec7c
  %v24 = STORE #0x7fffffeffd88, %v21  ; va=0x14043ec8b
  %v26 = STORE #0x7fffffeffd78, %rcx  ; va=0x14043ec8f
```
