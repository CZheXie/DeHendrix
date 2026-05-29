# Deobfuscation Chain Report

- Binary: `E:\ace\ACE-BASE.sys`
- Entry: `0x14043EC4E`
- Blocks: 8
- Total IR: 168
- Non-NOP: 168
- Side-effect: 47

## Optimized IR

```
func devirt_0x14043ec4e {
  %v29 = JMP #0x14043f340  ; va=0x14043ec9e
  %v31 = ADD %v25, #0x8
  %v34 = NOT %r10  ; va=0x14043f346
  %v36 = ADD %v0, #0x60
  %v37 = STORE %v36, %v34  ; va=0x14043f34c
  %v40 = ADD %v0, #0x18
  %v41 = STORE %v40, %rbx  ; va=0x14043f356
  %v42 = ADD %v0, #0x40
  %v43 = STORE %v42, %v31  ; va=0x14043f35a
  %v44 = ADD %v0, #0x28
  %v45 = STORE %v44, %rdi  ; va=0x14043f35e
  %v46 = ADD %v0, #0x70
  %v47 = STORE %v46, %r13  ; va=0x14043f362
  %v48 = PASSTHROUGH %r11  ; va=0x14043f366
  %v50 = ADD %v0, #0x48
  %v51 = STORE %v50, %v48  ; va=0x14043f36c
  %v54 = ADD %v0, #0x80
  %v55 = STORE %v54, %r15  ; va=0x14043f376
  %v56 = PASSTHROUGH %rax  ; va=0x14043f37d
  %v58 = ADD %v0, #0x58
  %v59 = STORE %v58, %v56  ; va=0x14043f382
  %v62 = ADD %v0, #0x38
  %v63 = STORE %v62, %rsi  ; va=0x14043f38b
  %v64 = ADD %v0, #0x68
  %v65 = STORE %v64, %r12  ; va=0x14043f38f
  %v66 = SUB %v31, #0x8
  %v67 = STORE %v66, %v13  ; va=0x14043f393
  %v70 = JMP #0x14043f2ea  ; va=0x14043f3a2
  %v71 = LOAD %v66  ; va=0x14043f2ea
  %v72 = ADD %v66, #0x8
  %v73 = NOT %v71  ; va=0x14043f2eb
  %v75 = ADD %v0, #0x20
  %v76 = STORE %v75, %v73  ; va=0x14043f2f1
  %v79 = SUB %v72, #0x8
  %v80 = STORE %v79, %rflags
  %v84 = LOAD %v79
  %v85 = ADD %v79, #0x8
  %v87 = ADD %v0, #0x8  ; addr_calc=1
  %v88 = LOAD %v87  ; va=0x14043f30c
  %v89 = SUB %v85, #0x8
  %v90 = STORE %v89, %v88  ; va=0x14043f30c
  %v92 = ADD %v89, #0x8
  %v93 = ADD %v0, #0x18  ; addr_calc=1
  %v94 = LOAD %v93  ; va=0x14043f312
  %v95 = SUB %v92, #0x8
  %v96 = STORE %v95, %v94  ; va=0x14043f312
  %v98 = ADD %v95, #0x8
  %v99 = ADD %v0, #0x20  ; addr_calc=1
  %v100 = LOAD %v99  ; va=0x14043f319
  %v101 = SUB %v98, #0x8
  %v102 = STORE %v101, %v100  ; va=0x14043f319
  %v104 = ADD %v101, #0x8
  %v105 = ADD %v0, #0x30  ; addr_calc=1
  %v106 = LOAD %v105  ; va=0x14043f320
  %v107 = SUB %v104, #0x8
  %v108 = STORE %v107, %v106  ; va=0x14043f320
  %v110 = ADD %v107, #0x8
  %v111 = SUB %v110, #0x8
  %v112 = STORE %v111, %v52  ; va=0x14043f327
  %v113 = SUB %v111, #0x8
  %v114 = STORE %v113, %v84
  %v118 = JMP #0x14043f097  ; va=0x14043f33c
  %v120 = ADD %v117, #0x8
  %v121 = ADD %v0, #0x40  ; addr_calc=1
  %v122 = LOAD %v121  ; va=0x14043f099
  %v123 = SUB %v120, #0x8
  %v124 = STORE %v123, %v122  ; va=0x14043f099
  %v126 = ADD %v123, #0x8
  %v127 = ADD %v0, #0x10  ; addr_calc=1
  %v128 = LOAD %v127  ; va=0x14043f0a0
  %v129 = SUB %v126, #0x8
  %v130 = STORE %v129, %v128  ; va=0x14043f0a0
  %v132 = ADD %v129, #0x8
  %v133 = ADD %v0, #0x38  ; addr_calc=1
  %v134 = LOAD %v133  ; va=0x14043f0a7
  %v135 = SUB %v132, #0x8
  %v136 = STORE %v135, %v134  ; va=0x14043f0a7
  %v138 = ADD %v135, #0x8
  %v139 = ADD %v0, #0x28  ; addr_calc=1
  %v140 = LOAD %v139  ; va=0x14043f0ae
  %v141 = SUB %v138, #0x8
  %v142 = STORE %v141, %v140  ; va=0x14043f0ae
  %v144 = ADD %v141, #0x8
  %v145 = ADD %v0, #0x48  ; addr_calc=1
  %v146 = LOAD %v145  ; va=0x14043f0b5
  %v147 = SUB %v144, #0x8
  %v148 = STORE %v147, %v146  ; va=0x14043f0b5
  %v150 = ADD %v147, #0x8
  %v151 = ADD %v0, #0x50  ; addr_calc=1
  %v152 = LOAD %v151  ; va=0x14043f0bc
  %v153 = SUB %v150, #0x8
  %v154 = STORE %v153, %v152  ; va=0x14043f0bc
  %v156 = ADD %v153, #0x8
  %v157 = ADD %v0, #0x58  ; addr_calc=1
  %v158 = LOAD %v157  ; va=0x14043f0c3
  %v159 = SUB %v156, #0x8
  %v160 = STORE %v159, %v158  ; va=0x14043f0c3
  %v162 = ADD %v159, #0x8
  %v163 = ADD %v0, #0x60  ; addr_calc=1
  %v164 = LOAD %v163  ; va=0x14043f0ca
  %v165 = SUB %v162, #0x8
  %v166 = STORE %v165, %v164  ; va=0x14043f0ca
  %v168 = ADD %v165, #0x8
  %v169 = ADD %v0, #0x68  ; addr_calc=1
  %v170 = LOAD %v169  ; va=0x14043f0d1
  %v171 = SUB %v168, #0x8
  %v172 = STORE %v171, %v170  ; va=0x14043f0d1
  %v174 = ADD %v171, #0x8
  %v175 = ADD %v0, #0x70  ; addr_calc=1
  %v176 = LOAD %v175  ; va=0x14043f0d8
  %v177 = SUB %v174, #0x8
  %v178 = STORE %v177, %v176  ; va=0x14043f0d8
  %v179 = SUB %v177, #0x8
  %v180 = STORE %v179, %v21  ; va=0x14043f0db
  %v181 = SUB %v179, #0x8
  %v182 = STORE %v181, %v116
  %v186 = JMP #0x14043f3a5  ; va=0x14043f0f0
  %v188 = ADD %v185, #0x8
  %v190 = ADD %v188, #0x8
  %v191 = ADD %v0, #0x78  ; addr_calc=1
  %v192 = LOAD %v191  ; va=0x14043f3ab
  %v193 = SUB %v190, #0x8
  %v194 = STORE %v193, %v192  ; va=0x14043f3ab
  %v196 = ADD %v193, #0x8
  %v197 = ADD %v0, #0x80  ; addr_calc=1
  %v198 = LOAD %v197  ; va=0x14043f3b2
  %v199 = SUB %v196, #0x8
  %v200 = STORE %v199, %v198  ; va=0x14043f3b2
  %v202 = ADD %v199, #0x8
  %v203 = PASSTHROUGH %v0  ; addr_calc=1
  %v204 = LOAD %v203  ; va=0x14043f3bc
  %v205 = SUB %v202, #0x8
  %v206 = STORE %v205, %v204  ; va=0x14043f3bc
  %v208 = ADD %v205, #0x8
  %v210 = ADD %v0, #0x88
  %v211 = ADD %v208, #0xfffffffffffffff8
  %v212 = PASSTHROUGH %v211
  %v213 = STORE %v212, %v210  ; va=0x14043f3d9
  %v214 = ADD %v211, #0xfffffffffffffff8
  %v215 = PASSTHROUGH %v214
  %v216 = STORE %v215, #0x1403bcd4f  ; va=0x14043f3e2
  %v217 = CALL #0x1404cc11c  ; va=0x14043f3e6
  %v218 = SUB %v214, #0x8
  %v219 = STORE %v218, %rsi  ; va=0x1404cc11c
  %v220 = SUB %v218, #0x8
  %v221 = STORE %v220, %rbx  ; va=0x1404cc11d
  %v222 = SUB %v220, #0x8
  %v223 = STORE %v222, %rdi  ; va=0x1404cc11e
  %v224 = SUB %v222, #0x8
  %v225 = STORE %v224, %r12  ; va=0x1404cc11f
  %v226 = SUB %v224, #0x8
  %v227 = STORE %v226, %v0  ; va=0x1404cc121
  %v228 = SUB %v226, #0x8
  %v229 = STORE %v228, %r15  ; va=0x1404cc122
  %v235 = CALL #0x1406318ad  ; va=0x1404cc139
  %v236 = ADD %v230, #0x8
  %v238 = PASSTHROUGH %v230
  %v239 = STORE %v238, #0x14066ae1d  ; va=0x1406318b9
  %v285 = SUB %v236, #0x8
  %v286 = STORE %v285, %v232  ; va=0x140631947
  %v289 = JMP #0x1404e90d4  ; va=0x140631957
  %v301 = ADD %v230, #0x60
  %v302 = STORE %v301, %v278  ; va=0x1404e90f0
  %v305 = AND #0x1404cc11c, %v119  ; va=0x1404e9101
  %v306 = XOR %v305, %v119  ; va=0x1404e9104
  %v307 = OR %v306, %v119  ; va=0x1404e9107
  %v308 = ADD %v307, #0x1404cc11c  ; va=0x1404e910a
  %v309 = JMP %v308  ; va=0x1404e910d
}
```

## Side-effect Instructions Only

```
  %v37 = STORE %v36, %v34  ; va=0x14043f34c
  %v41 = STORE %v40, %rbx  ; va=0x14043f356
  %v43 = STORE %v42, %v31  ; va=0x14043f35a
  %v45 = STORE %v44, %rdi  ; va=0x14043f35e
  %v47 = STORE %v46, %r13  ; va=0x14043f362
  %v51 = STORE %v50, %v48  ; va=0x14043f36c
  %v55 = STORE %v54, %r15  ; va=0x14043f376
  %v59 = STORE %v58, %v56  ; va=0x14043f382
  %v63 = STORE %v62, %rsi  ; va=0x14043f38b
  %v65 = STORE %v64, %r12  ; va=0x14043f38f
  %v67 = STORE %v66, %v13  ; va=0x14043f393
  %v76 = STORE %v75, %v73  ; va=0x14043f2f1
  %v80 = STORE %v79, %rflags
  %v90 = STORE %v89, %v88  ; va=0x14043f30c
  %v96 = STORE %v95, %v94  ; va=0x14043f312
  %v102 = STORE %v101, %v100  ; va=0x14043f319
  %v108 = STORE %v107, %v106  ; va=0x14043f320
  %v112 = STORE %v111, %v52  ; va=0x14043f327
  %v114 = STORE %v113, %v84
  %v124 = STORE %v123, %v122  ; va=0x14043f099
  %v130 = STORE %v129, %v128  ; va=0x14043f0a0
  %v136 = STORE %v135, %v134  ; va=0x14043f0a7
  %v142 = STORE %v141, %v140  ; va=0x14043f0ae
  %v148 = STORE %v147, %v146  ; va=0x14043f0b5
  %v154 = STORE %v153, %v152  ; va=0x14043f0bc
  %v160 = STORE %v159, %v158  ; va=0x14043f0c3
  %v166 = STORE %v165, %v164  ; va=0x14043f0ca
  %v172 = STORE %v171, %v170  ; va=0x14043f0d1
  %v178 = STORE %v177, %v176  ; va=0x14043f0d8
  %v180 = STORE %v179, %v21  ; va=0x14043f0db
  %v182 = STORE %v181, %v116
  %v194 = STORE %v193, %v192  ; va=0x14043f3ab
  %v200 = STORE %v199, %v198  ; va=0x14043f3b2
  %v206 = STORE %v205, %v204  ; va=0x14043f3bc
  %v213 = STORE %v212, %v210  ; va=0x14043f3d9
  %v216 = STORE %v215, #0x1403bcd4f  ; va=0x14043f3e2
  %v217 = CALL #0x1404cc11c  ; va=0x14043f3e6
  %v219 = STORE %v218, %rsi  ; va=0x1404cc11c
  %v221 = STORE %v220, %rbx  ; va=0x1404cc11d
  %v223 = STORE %v222, %rdi  ; va=0x1404cc11e
  %v225 = STORE %v224, %r12  ; va=0x1404cc11f
  %v227 = STORE %v226, %v0  ; va=0x1404cc121
  %v229 = STORE %v228, %r15  ; va=0x1404cc122
  %v235 = CALL #0x1406318ad  ; va=0x1404cc139
  %v239 = STORE %v238, #0x14066ae1d  ; va=0x1406318b9
  %v286 = STORE %v285, %v232  ; va=0x140631947
  %v302 = STORE %v301, %v278  ; va=0x1404e90f0
```
