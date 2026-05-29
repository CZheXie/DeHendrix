; ModuleID = 'dehex_devirt_v2'
source_filename = "dehex_devirt"

declare i64 @external_call(i64)

define i64 @devirt(i64 %rbp, i64 %rflags, i64 %r14, i64 %rdx, i64 %r9, i64 %rax, i64 %rbx, i64 %rdi, i64 %r13, i64 %r8, i64 %r15, i64 %rcx, i64 %rsi, i64 %r12) {
entry:
  %t0 = inttoptr i64 140737488342256 to ptr
  store volatile i64 %rbp, ptr %t0
  %t1 = inttoptr i64 140737488342232 to ptr
  store volatile i64 %rflags, ptr %t1
  %t2 = inttoptr i64 140737488342360 to ptr
  store volatile i64 %r14, ptr %t2
  %t3 = inttoptr i64 140737488342288 to ptr
  store volatile i64 %rdx, ptr %t3
  %t4 = inttoptr i64 140737488342320 to ptr
  store volatile i64 %r9, ptr %t4
  %t5 = inttoptr i64 140737488342232 to ptr
  store volatile i64 %rax, ptr %t5
  %t6 = inttoptr i64 140737488342224 to ptr
  store volatile i64 %rflags, ptr %t6
  %v24 = add i64 0, 0 ; JMP
  ; LOAD from 0
  %v25 = inttoptr i64 0 to ptr
  %t7 = load i64, ptr %v25
  %v26 = add i64 0, 8
  %v28 = add i64 0, 8
  %t8 = inttoptr i64 %v28 to ptr
  store volatile i64 %t7, ptr %t8
  %v32 = add i64 0, 96
  %t9 = inttoptr i64 %v32 to ptr
  store volatile i64 0, ptr %t9
  %v35 = add i64 0, 24
  %t10 = inttoptr i64 %v35 to ptr
  store volatile i64 %rbx, ptr %t10
  %v37 = add i64 0, 64
  %t11 = inttoptr i64 %v37 to ptr
  store volatile i64 %v26, ptr %t11
  %v39 = add i64 0, 40
  %t12 = inttoptr i64 %v39 to ptr
  store volatile i64 %rdi, ptr %t12
  %v41 = add i64 0, 112
  %t13 = inttoptr i64 %v41 to ptr
  store volatile i64 %r13, ptr %t13
  %v44 = add i64 0, 72
  %t14 = inttoptr i64 %v44 to ptr
  store volatile i64 %r8, ptr %t14
  %v47 = add i64 0, 128
  %t15 = inttoptr i64 %v47 to ptr
  store volatile i64 %r15, ptr %t15
  %v50 = sub i64 %v26, 8
  %t16 = inttoptr i64 %v50 to ptr
  store volatile i64 %rcx, ptr %t16
  %v52 = sub i64 %v50, 8
  %t17 = inttoptr i64 %v52 to ptr
  store volatile i64 0, ptr %t17
  %v57 = add i64 0, 0 ; JMP
  ; LOAD from 0
  %v58 = inttoptr i64 0 to ptr
  %t18 = load i64, ptr %v58
  %v59 = add i64 0, 8
  %v60 = add i64 0, 88
  %t19 = inttoptr i64 %v60 to ptr
  store volatile i64 0, ptr %t19
  %v63 = add i64 0, 56
  %t20 = inttoptr i64 %v63 to ptr
  store volatile i64 %rsi, ptr %t20
  %v65 = add i64 0, 104
  %t21 = inttoptr i64 %v65 to ptr
  store volatile i64 %r12, ptr %t21
  %v68 = add i64 0, 32
  %t22 = inttoptr i64 %v68 to ptr
  store volatile i64 %t18, ptr %t22
  %v71 = sub i64 %v59, 8
  %t23 = inttoptr i64 %v71 to ptr
  store volatile i64 0, ptr %t23
  %v77 = add i64 %v71, 8
  %v79 = add i64 0, 8
  ; LOAD from %v79
  %v80 = inttoptr i64 %v79 to ptr
  %t24 = load i64, ptr %v80
  %v81 = sub i64 %v77, 8
  %t25 = inttoptr i64 %v81 to ptr
  store volatile i64 %t24, ptr %t25
  %v84 = add i64 %v81, 8
  %v85 = add i64 0, 24
  ; LOAD from %v85
  %v86 = inttoptr i64 %v85 to ptr
  %t26 = load i64, ptr %v86
  %v87 = sub i64 %v84, 8
  %t27 = inttoptr i64 %v87 to ptr
  store volatile i64 %t26, ptr %t27
  %v90 = add i64 %v87, 8
  %v91 = add i64 0, 32
  ; LOAD from %v91
  %v92 = inttoptr i64 %v91 to ptr
  %t28 = load i64, ptr %v92
  %v93 = sub i64 %v90, 8
  %t29 = inttoptr i64 %v93 to ptr
  store volatile i64 %t28, ptr %t29
  %v96 = add i64 %v93, 8
  %v97 = add i64 0, 48
  ; LOAD from %v97
  %v98 = inttoptr i64 %v97 to ptr
  %t30 = load i64, ptr %v98
  %v99 = sub i64 %v96, 8
  %t31 = inttoptr i64 %v99 to ptr
  store volatile i64 %t30, ptr %t31
  %v101 = sub i64 %v99, 8
  %t32 = inttoptr i64 %v101 to ptr
  store volatile i64 0, ptr %t32
  %v105 = add i64 0, 0 ; JMP
  %v107 = add i64 %v101, 8
  %v109 = add i64 %v107, 8
  %v110 = add i64 0, 64
  ; LOAD from %v110
  %v111 = inttoptr i64 %v110 to ptr
  %t33 = load i64, ptr %v111
  %v112 = sub i64 %v109, 8
  %t34 = inttoptr i64 %v112 to ptr
  store volatile i64 %t33, ptr %t34
  %v115 = add i64 %v112, 8
  %v116 = add i64 0, 16
  ; LOAD from %v116
  %v117 = inttoptr i64 %v116 to ptr
  %t35 = load i64, ptr %v117
  %v118 = sub i64 %v115, 8
  %t36 = inttoptr i64 %v118 to ptr
  store volatile i64 %t35, ptr %t36
  %v121 = add i64 %v118, 8
  %v122 = add i64 0, 56
  ; LOAD from %v122
  %v123 = inttoptr i64 %v122 to ptr
  %t37 = load i64, ptr %v123
  %v124 = sub i64 %v121, 8
  %t38 = inttoptr i64 %v124 to ptr
  store volatile i64 %t37, ptr %t38
  %v127 = add i64 %v124, 8
  %v128 = add i64 0, 40
  ; LOAD from %v128
  %v129 = inttoptr i64 %v128 to ptr
  %t39 = load i64, ptr %v129
  %v130 = sub i64 %v127, 8
  %t40 = inttoptr i64 %v130 to ptr
  store volatile i64 %t39, ptr %t40
  %v133 = add i64 %v130, 8
  %v134 = add i64 0, 72
  ; LOAD from %v134
  %v135 = inttoptr i64 %v134 to ptr
  %t41 = load i64, ptr %v135
  %v136 = sub i64 %v133, 8
  %t42 = inttoptr i64 %v136 to ptr
  store volatile i64 %t41, ptr %t42
  %v139 = add i64 %v136, 8
  %v140 = add i64 0, 80
  ; LOAD from %v140
  %v141 = inttoptr i64 %v140 to ptr
  %t43 = load i64, ptr %v141
  %v142 = sub i64 %v139, 8
  %t44 = inttoptr i64 %v142 to ptr
  store volatile i64 %t43, ptr %t44
  %v145 = add i64 %v142, 8
  %v146 = add i64 0, 88
  ; LOAD from %v146
  %v147 = inttoptr i64 %v146 to ptr
  %t45 = load i64, ptr %v147
  %v148 = sub i64 %v145, 8
  %t46 = inttoptr i64 %v148 to ptr
  store volatile i64 %t45, ptr %t46
  %v151 = add i64 %v148, 8
  %v152 = add i64 0, 96
  ; LOAD from %v152
  %v153 = inttoptr i64 %v152 to ptr
  %t47 = load i64, ptr %v153
  %v154 = sub i64 %v151, 8
  %t48 = inttoptr i64 %v154 to ptr
  store volatile i64 %t47, ptr %t48
  %v157 = add i64 %v154, 8
  %v158 = add i64 0, 104
  ; LOAD from %v158
  %v159 = inttoptr i64 %v158 to ptr
  %t49 = load i64, ptr %v159
  %v160 = sub i64 %v157, 8
  %t50 = inttoptr i64 %v160 to ptr
  store volatile i64 %t49, ptr %t50
  %v163 = add i64 %v160, 8
  %v164 = add i64 0, 112
  ; LOAD from %v164
  %v165 = inttoptr i64 %v164 to ptr
  %t51 = load i64, ptr %v165
  %v166 = sub i64 %v163, 8
  %t52 = inttoptr i64 %v166 to ptr
  store volatile i64 %t51, ptr %t52
  %v169 = add i64 %v166, 8
  %v170 = sub i64 %v169, 8
  %t53 = inttoptr i64 %v170 to ptr
  store volatile i64 0, ptr %t53
  %v174 = add i64 0, 0 ; JMP
  %v176 = add i64 %v170, 8
  %v177 = add i64 0, 120
  ; LOAD from %v177
  %v178 = inttoptr i64 %v177 to ptr
  %t54 = load i64, ptr %v178
  %v179 = sub i64 %v176, 8
  %t55 = inttoptr i64 %v179 to ptr
  store volatile i64 %t54, ptr %t55
  %v182 = add i64 %v179, 8
  %v183 = add i64 0, 128
  ; LOAD from %v183
  %v184 = inttoptr i64 %v183 to ptr
  %t56 = load i64, ptr %v184
  %v185 = sub i64 %v182, 8
  %t57 = inttoptr i64 %v185 to ptr
  store volatile i64 %t56, ptr %t57
  %v188 = add i64 %v185, 8
  ; LOAD from 0
  %v190 = inttoptr i64 0 to ptr
  %t58 = load i64, ptr %v190
  %v191 = sub i64 %v188, 8
  %t59 = inttoptr i64 %v191 to ptr
  store volatile i64 %t58, ptr %t59
  %v194 = add i64 %v191, 8
  %v196 = add i64 0, 136
  %v197 = add i64 %v194, 18446744073709551608
  %t60 = inttoptr i64 %v197 to ptr
  store volatile i64 %v196, ptr %t60
  %v200 = add i64 %v197, 18446744073709551608
  %t61 = inttoptr i64 %v200 to ptr
  store volatile i64 5372780644, ptr %t61
  ; CALL 5373739292
  %v203 = call i64 @external_call(i64 5373739292)
  %v204 = sub i64 %v200, 8
  %t62 = inttoptr i64 %v204 to ptr
  store volatile i64 %rsi, ptr %t62
  %v206 = sub i64 %v204, 8
  %t63 = inttoptr i64 %v206 to ptr
  store volatile i64 %rbx, ptr %t63
  %v208 = sub i64 %v206, 8
  %t64 = inttoptr i64 %v208 to ptr
  store volatile i64 %rdi, ptr %t64
  %v210 = sub i64 %v208, 8
  %t65 = inttoptr i64 %v210 to ptr
  store volatile i64 %r12, ptr %t65
  %v212 = sub i64 %v210, 8
  %t66 = inttoptr i64 %v212 to ptr
  store volatile i64 0, ptr %t66
  %v214 = sub i64 %v212, 8
  %t67 = inttoptr i64 %v214 to ptr
  store volatile i64 %r15, ptr %t67
  ; CALL 5375203501
  %v221 = call i64 @external_call(i64 5375203501)
  %v222 = add i64 0, 8
  %t68 = inttoptr i64 0 to ptr
  store volatile i64 5375438365, ptr %t68
  %v233 = and i64 4, 0
  %v239 = sub i64 4, %v233
  %v240 = or i64 %v239, 4
  %v241 = add i64 %v240, 0
  ; LOAD from %v241
  %v243 = inttoptr i64 %v241 to ptr
  %t69 = load i64, ptr %v243
  %v271 = sub i64 %v222, 8
  %t70 = inttoptr i64 %v271 to ptr
  store volatile i64 %t69, ptr %t70
  %v275 = add i64 0, 0 ; JMP
  %v278 = xor i64 0, -1
  %v279 = add i64 %v278, 0
  ; LOAD from %v279
  %v281 = inttoptr i64 %v279 to ptr
  %t71 = load i64, ptr %v281
  %v282 = sub i64 %t71, %t69
  %v283 = or i64 %v282, %t69
  %v284 = and i64 %v283, %t69
  %v285 = xor i64 %v284, %t71
  %v286 = xor i64 %v285, -1
  %v287 = add i64 0, 96
  %t72 = inttoptr i64 %v287 to ptr
  store volatile i64 0, ptr %t72
  %v290 = add i64 0, 0 ; unsupported: SEXT
  %v294 = add i64 %v290, 5373739292
  %v295 = add i64 0, 0 ; JMP
  ret i64 0
}
