; ModuleID = 'dehex_devirt_v2'
source_filename = "dehex_devirt"

declare i64 @external_call(i64)

define i64 @devirt(i64 %rbp, i64 %rflags, i64 %r14, i64 %rdx, i64 %r9, i64 %rax, i64 %rcx, i64 %r11, i64 %rbx, i64 %rdi, i64 %r13, i64 %r15, i64 %r10, i64 %rsi, i64 %r12) {
entry:
  %t0 = inttoptr i64 140737488342416 to ptr
  store volatile i64 %rbp, ptr %t0
  %t1 = inttoptr i64 140737488342392 to ptr
  store volatile i64 %rflags, ptr %t1
  %t2 = inttoptr i64 140737488342520 to ptr
  store volatile i64 %r14, ptr %t2
  %t3 = inttoptr i64 140737488342448 to ptr
  store volatile i64 %rdx, ptr %t3
  %t4 = inttoptr i64 140737488342480 to ptr
  store volatile i64 %r9, ptr %t4
  %t5 = inttoptr i64 140737488342408 to ptr
  store volatile i64 %rax, ptr %t5
  %t6 = inttoptr i64 140737488342392 to ptr
  store volatile i64 %rcx, ptr %t6
  %v24 = add i64 0, 0 ; JMP
  %v26 = add i64 0, 8
  %v29 = add i64 0, 96
  %t7 = inttoptr i64 %v29 to ptr
  store volatile i64 %r11, ptr %t7
  %v32 = add i64 0, 24
  %t8 = inttoptr i64 %v32 to ptr
  store volatile i64 %rbx, ptr %t8
  %v34 = add i64 0, 64
  %t9 = inttoptr i64 %v34 to ptr
  store volatile i64 %v26, ptr %t9
  %v36 = add i64 0, 40
  %t10 = inttoptr i64 %v36 to ptr
  store volatile i64 %rdi, ptr %t10
  %v38 = add i64 0, 112
  %t11 = inttoptr i64 %v38 to ptr
  store volatile i64 %r13, ptr %t11
  %v41 = add i64 0, 72
  %t12 = inttoptr i64 %v41 to ptr
  store volatile i64 0, ptr %t12
  %v44 = add i64 0, 128
  %t13 = inttoptr i64 %v44 to ptr
  store volatile i64 %r15, ptr %t13
  %v47 = add i64 0, 88
  %t14 = inttoptr i64 %v47 to ptr
  store volatile i64 %r10, ptr %t14
  %v50 = add i64 0, 56
  %t15 = inttoptr i64 %v50 to ptr
  store volatile i64 %rsi, ptr %t15
  %v52 = add i64 0, 104
  %t16 = inttoptr i64 %v52 to ptr
  store volatile i64 %r12, ptr %t16
  %v54 = sub i64 %v26, 8
  %t17 = inttoptr i64 %v54 to ptr
  store volatile i64 %rdx, ptr %t17
  %v58 = add i64 0, 0 ; JMP
  %v60 = add i64 %v54, 8
  %v62 = add i64 0, 32
  %t18 = inttoptr i64 %v62 to ptr
  store volatile i64 0, ptr %t18
  %v65 = sub i64 %v60, 8
  %t19 = inttoptr i64 %v65 to ptr
  store volatile i64 %rflags, ptr %t19
  %v71 = add i64 %v65, 8
  %v73 = add i64 0, 8
  ; LOAD from %v73
  %v74 = inttoptr i64 %v73 to ptr
  %t20 = load i64, ptr %v74
  %v75 = sub i64 %v71, 8
  %t21 = inttoptr i64 %v75 to ptr
  store volatile i64 %t20, ptr %t21
  %v78 = add i64 %v75, 8
  %v79 = add i64 0, 24
  ; LOAD from %v79
  %v80 = inttoptr i64 %v79 to ptr
  %t22 = load i64, ptr %v80
  %v81 = sub i64 %v78, 8
  %t23 = inttoptr i64 %v81 to ptr
  store volatile i64 %t22, ptr %t23
  %v84 = add i64 %v81, 8
  %v85 = add i64 0, 32
  ; LOAD from %v85
  %v86 = inttoptr i64 %v85 to ptr
  %t24 = load i64, ptr %v86
  %v87 = sub i64 %v84, 8
  %t25 = inttoptr i64 %v87 to ptr
  store volatile i64 %t24, ptr %t25
  %v90 = add i64 %v87, 8
  %v91 = add i64 0, 48
  ; LOAD from %v91
  %v92 = inttoptr i64 %v91 to ptr
  %t26 = load i64, ptr %v92
  %v93 = sub i64 %v90, 8
  %t27 = inttoptr i64 %v93 to ptr
  store volatile i64 %t26, ptr %t27
  %v96 = add i64 %v93, 8
  %v97 = sub i64 %v96, 8
  %t28 = inttoptr i64 %v97 to ptr
  store volatile i64 0, ptr %t28
  %v99 = sub i64 %v97, 8
  %t29 = inttoptr i64 %v99 to ptr
  store volatile i64 %rflags, ptr %t29
  %v104 = add i64 0, 0 ; JMP
  %v106 = add i64 0, 8
  %v107 = add i64 0, 64
  ; LOAD from %v107
  %v108 = inttoptr i64 %v107 to ptr
  %t30 = load i64, ptr %v108
  %v109 = sub i64 %v106, 8
  %t31 = inttoptr i64 %v109 to ptr
  store volatile i64 %t30, ptr %t31
  %v112 = add i64 %v109, 8
  %v113 = add i64 0, 16
  ; LOAD from %v113
  %v114 = inttoptr i64 %v113 to ptr
  %t32 = load i64, ptr %v114
  %v115 = sub i64 %v112, 8
  %t33 = inttoptr i64 %v115 to ptr
  store volatile i64 %t32, ptr %t33
  %v118 = add i64 %v115, 8
  %v119 = add i64 0, 56
  ; LOAD from %v119
  %v120 = inttoptr i64 %v119 to ptr
  %t34 = load i64, ptr %v120
  %v121 = sub i64 %v118, 8
  %t35 = inttoptr i64 %v121 to ptr
  store volatile i64 %t34, ptr %t35
  %v124 = add i64 %v121, 8
  %v125 = add i64 0, 40
  ; LOAD from %v125
  %v126 = inttoptr i64 %v125 to ptr
  %t36 = load i64, ptr %v126
  %v127 = sub i64 %v124, 8
  %t37 = inttoptr i64 %v127 to ptr
  store volatile i64 %t36, ptr %t37
  %v130 = add i64 %v127, 8
  %v131 = add i64 0, 72
  ; LOAD from %v131
  %v132 = inttoptr i64 %v131 to ptr
  %t38 = load i64, ptr %v132
  %v133 = sub i64 %v130, 8
  %t39 = inttoptr i64 %v133 to ptr
  store volatile i64 %t38, ptr %t39
  %v136 = add i64 %v133, 8
  %v137 = add i64 0, 80
  ; LOAD from %v137
  %v138 = inttoptr i64 %v137 to ptr
  %t40 = load i64, ptr %v138
  %v139 = sub i64 %v136, 8
  %t41 = inttoptr i64 %v139 to ptr
  store volatile i64 %t40, ptr %t41
  %v142 = add i64 %v139, 8
  %v143 = add i64 0, 88
  ; LOAD from %v143
  %v144 = inttoptr i64 %v143 to ptr
  %t42 = load i64, ptr %v144
  %v145 = sub i64 %v142, 8
  %t43 = inttoptr i64 %v145 to ptr
  store volatile i64 %t42, ptr %t43
  %v148 = add i64 %v145, 8
  %v149 = add i64 0, 96
  ; LOAD from %v149
  %v150 = inttoptr i64 %v149 to ptr
  %t44 = load i64, ptr %v150
  %v151 = sub i64 %v148, 8
  %t45 = inttoptr i64 %v151 to ptr
  store volatile i64 %t44, ptr %t45
  %v154 = add i64 %v151, 8
  %v155 = add i64 0, 104
  ; LOAD from %v155
  %v156 = inttoptr i64 %v155 to ptr
  %t46 = load i64, ptr %v156
  %v157 = sub i64 %v154, 8
  %t47 = inttoptr i64 %v157 to ptr
  store volatile i64 %t46, ptr %t47
  %v160 = add i64 %v157, 8
  %v161 = add i64 0, 112
  ; LOAD from %v161
  %v162 = inttoptr i64 %v161 to ptr
  %t48 = load i64, ptr %v162
  %v163 = sub i64 %v160, 8
  %t49 = inttoptr i64 %v163 to ptr
  store volatile i64 %t48, ptr %t49
  %v165 = sub i64 %v163, 8
  %t50 = inttoptr i64 %v165 to ptr
  store volatile i64 0, ptr %t50
  %v167 = sub i64 %v165, 8
  %t51 = inttoptr i64 %v167 to ptr
  store volatile i64 0, ptr %t51
  %v172 = add i64 0, 0 ; JMP
  %v174 = add i64 0, 8
  %v176 = add i64 %v174, 8
  %v177 = add i64 0, 120
  ; LOAD from %v177
  %v178 = inttoptr i64 %v177 to ptr
  %t52 = load i64, ptr %v178
  %v179 = sub i64 %v176, 8
  %t53 = inttoptr i64 %v179 to ptr
  store volatile i64 %t52, ptr %t53
  %v182 = add i64 %v179, 8
  %v183 = add i64 0, 128
  ; LOAD from %v183
  %v184 = inttoptr i64 %v183 to ptr
  %t54 = load i64, ptr %v184
  %v185 = sub i64 %v182, 8
  %t55 = inttoptr i64 %v185 to ptr
  store volatile i64 %t54, ptr %t55
  %v188 = add i64 %v185, 8
  ; LOAD from 0
  %v190 = inttoptr i64 0 to ptr
  %t56 = load i64, ptr %v190
  %v191 = sub i64 %v188, 8
  %t57 = inttoptr i64 %v191 to ptr
  store volatile i64 %t56, ptr %t57
  %v194 = add i64 %v191, 8
  %v196 = add i64 0, 136
  %v197 = add i64 %v194, 18446744073709551608
  %t58 = inttoptr i64 %v197 to ptr
  store volatile i64 %v196, ptr %t58
  %v200 = add i64 %v197, 18446744073709551608
  %t59 = inttoptr i64 %v200 to ptr
  store volatile i64 5372628303, ptr %t59
  ; CALL 5373739292
  %v203 = call i64 @external_call(i64 5373739292)
  %v204 = sub i64 %v200, 8
  %t60 = inttoptr i64 %v204 to ptr
  store volatile i64 %rsi, ptr %t60
  %v206 = sub i64 %v204, 8
  %t61 = inttoptr i64 %v206 to ptr
  store volatile i64 %rbx, ptr %t61
  %v208 = sub i64 %v206, 8
  %t62 = inttoptr i64 %v208 to ptr
  store volatile i64 %rdi, ptr %t62
  %v210 = sub i64 %v208, 8
  %t63 = inttoptr i64 %v210 to ptr
  store volatile i64 %r12, ptr %t63
  %v212 = sub i64 %v210, 8
  %t64 = inttoptr i64 %v212 to ptr
  store volatile i64 0, ptr %t64
  %v214 = sub i64 %v212, 8
  %t65 = inttoptr i64 %v214 to ptr
  store volatile i64 %r15, ptr %t65
  ; CALL 5375203501
  %v221 = call i64 @external_call(i64 5375203501)
  %v222 = add i64 0, 8
  %t66 = inttoptr i64 0 to ptr
  store volatile i64 5375438365, ptr %t66
  %v233 = and i64 4, 0
  %v239 = sub i64 4, %v233
  %v240 = or i64 %v239, 4
  %v241 = add i64 %v240, 0
  ; LOAD from %v241
  %v243 = inttoptr i64 %v241 to ptr
  %t67 = load i64, ptr %v243
  %v271 = sub i64 %v222, 8
  %t68 = inttoptr i64 %v271 to ptr
  store volatile i64 %t67, ptr %t68
  %v275 = add i64 0, 0 ; JMP
  %v278 = xor i64 0, -1
  %v279 = add i64 %v278, 0
  ; LOAD from %v279
  %v281 = inttoptr i64 %v279 to ptr
  %t69 = load i64, ptr %v281
  %v282 = sub i64 %t69, %t67
  %v283 = or i64 %v282, %t67
  %v284 = and i64 %v283, %t67
  %v285 = xor i64 %v284, %t69
  %v286 = xor i64 %v285, -1
  %v287 = add i64 0, 96
  %t70 = inttoptr i64 %v287 to ptr
  store volatile i64 0, ptr %t70
  %v290 = add i64 0, 0 ; unsupported: SEXT
  %v294 = add i64 %v290, 5373739292
  %v295 = add i64 0, 0 ; JMP
  ret i64 0
}
