; ModuleID = '_init_devirt.ll'
source_filename = "dehex_devirt"
target datalayout = "e-m:w-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-windows-msvc19.44.35222"

declare i64 @external_call(i64) local_unnamed_addr

define noundef i64 @devirt(i64 %rbp, i64 %rflags, i64 %r14, i64 %rdx, i64 %r9, i64 %rax, i64 %rbx, i64 %rdi, i64 %r13, i64 %r8, i64 %r15, i64 %rcx, i64 %rsi, i64 %r12) local_unnamed_addr {
entry:
  store volatile i64 %rbp, ptr inttoptr (i64 140737488342256 to ptr), align 16
  store volatile i64 %rflags, ptr inttoptr (i64 140737488342232 to ptr), align 8
  store volatile i64 %r14, ptr inttoptr (i64 140737488342360 to ptr), align 8
  store volatile i64 %rdx, ptr inttoptr (i64 140737488342288 to ptr), align 16
  store volatile i64 %r9, ptr inttoptr (i64 140737488342320 to ptr), align 16
  store volatile i64 %rax, ptr inttoptr (i64 140737488342232 to ptr), align 8
  store volatile i64 %rflags, ptr inttoptr (i64 140737488342224 to ptr), align 16
  store volatile i64 undef, ptr inttoptr (i64 8 to ptr), align 8
  store volatile i64 0, ptr inttoptr (i64 96 to ptr), align 32
  store volatile i64 %rbx, ptr inttoptr (i64 24 to ptr), align 8
  store volatile i64 8, ptr inttoptr (i64 64 to ptr), align 64
  store volatile i64 %rdi, ptr inttoptr (i64 40 to ptr), align 8
  store volatile i64 %r13, ptr inttoptr (i64 112 to ptr), align 16
  store volatile i64 %r8, ptr inttoptr (i64 72 to ptr), align 8
  store volatile i64 %r15, ptr inttoptr (i64 128 to ptr), align 128
  store volatile i64 %rcx, ptr null, align 4294967296
  store volatile i64 0, ptr inttoptr (i64 -8 to ptr), align 8
  store volatile i64 0, ptr inttoptr (i64 88 to ptr), align 8
  store volatile i64 %rsi, ptr inttoptr (i64 56 to ptr), align 8
  store volatile i64 %r12, ptr inttoptr (i64 104 to ptr), align 8
  store volatile i64 undef, ptr inttoptr (i64 32 to ptr), align 32
  store volatile i64 0, ptr null, align 4294967296
  %t24 = load i64, ptr inttoptr (i64 8 to ptr), align 8
  store volatile i64 %t24, ptr null, align 4294967296
  %t26 = load i64, ptr inttoptr (i64 24 to ptr), align 8
  store volatile i64 %t26, ptr null, align 4294967296
  store volatile i64 undef, ptr null, align 4294967296
  %t30 = load i64, ptr inttoptr (i64 48 to ptr), align 16
  store volatile i64 %t30, ptr null, align 4294967296
  store volatile i64 0, ptr inttoptr (i64 -8 to ptr), align 8
  %t33 = load i64, ptr inttoptr (i64 64 to ptr), align 64
  store volatile i64 %t33, ptr null, align 4294967296
  %t35 = load i64, ptr inttoptr (i64 16 to ptr), align 16
  store volatile i64 %t35, ptr null, align 4294967296
  %t37 = load i64, ptr inttoptr (i64 56 to ptr), align 8
  store volatile i64 %t37, ptr null, align 4294967296
  %t39 = load i64, ptr inttoptr (i64 40 to ptr), align 8
  store volatile i64 %t39, ptr null, align 4294967296
  %t41 = load i64, ptr inttoptr (i64 72 to ptr), align 8
  store volatile i64 %t41, ptr null, align 4294967296
  %t43 = load i64, ptr inttoptr (i64 80 to ptr), align 16
  store volatile i64 %t43, ptr null, align 4294967296
  %t45 = load i64, ptr inttoptr (i64 88 to ptr), align 8
  store volatile i64 %t45, ptr null, align 4294967296
  %t47 = load i64, ptr inttoptr (i64 96 to ptr), align 32
  store volatile i64 %t47, ptr null, align 4294967296
  %t49 = load i64, ptr inttoptr (i64 104 to ptr), align 8
  store volatile i64 %t49, ptr null, align 4294967296
  %t51 = load i64, ptr inttoptr (i64 112 to ptr), align 16
  store volatile i64 %t51, ptr null, align 4294967296
  store volatile i64 0, ptr null, align 4294967296
  %t54 = load i64, ptr inttoptr (i64 120 to ptr), align 8
  store volatile i64 %t54, ptr null, align 4294967296
  %t56 = load i64, ptr inttoptr (i64 128 to ptr), align 128
  store volatile i64 %t56, ptr null, align 4294967296
  store volatile i64 %t56, ptr null, align 4294967296
  store volatile i64 136, ptr null, align 4294967296
  store volatile i64 5372780644, ptr inttoptr (i64 -8 to ptr), align 8
  %v203 = tail call i64 @external_call(i64 5373739292)
  store volatile i64 %rsi, ptr inttoptr (i64 -16 to ptr), align 16
  store volatile i64 %rbx, ptr inttoptr (i64 -24 to ptr), align 8
  store volatile i64 %rdi, ptr inttoptr (i64 -32 to ptr), align 32
  store volatile i64 %r12, ptr inttoptr (i64 -40 to ptr), align 8
  store volatile i64 0, ptr inttoptr (i64 -48 to ptr), align 16
  store volatile i64 %r15, ptr inttoptr (i64 -56 to ptr), align 8
  %v221 = tail call i64 @external_call(i64 5375203501)
  store volatile i64 5375438365, ptr null, align 4294967296
  %t69 = load i64, ptr inttoptr (i64 4 to ptr), align 4
  store volatile i64 %t69, ptr null, align 4294967296
  store volatile i64 0, ptr inttoptr (i64 96 to ptr), align 32
  ret i64 0
}
