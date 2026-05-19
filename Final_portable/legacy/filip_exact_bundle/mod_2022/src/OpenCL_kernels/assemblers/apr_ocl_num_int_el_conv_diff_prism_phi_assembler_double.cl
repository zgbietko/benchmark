        .file "/tmp/ea5c8d8e-911d-4dd3-a2ff-138db6141218.TMP"
        .text
        .align 4
        .globl _const_0
_const_0:
        .long 0x3f800000
        .align 4
        .globl _const_1
_const_1:
        .long 0xbf000000
        .align 4
        .globl _const_2
_const_2:
        .long 0x3f000000
        .align 32
        .globl _const_3
_const_3:
        .long 0
        .long 0
        .long 0
        .long 1072693248
        .long 0
        .long 1094189056
        .long 0
        .long 1094188034
        .align 32
        .globl _const_3
_const_3:
        .long 0
        .long -1048576
        .long 0
        .long 1048576
        .long 0
        .long -2147483648
        .long 0
        .long 2146435072
        .align 16
        .globl _const_3
_const_3:
        .long 1083181056
        .long -1064250368
        .long 2145386496
        .long 55574528
        .align 16
        .globl _const_3
_const_3:
        .long 0
        .long 1065353216
        .long 0
        .long 0
        .align 4
        .globl _const_3
_const_3:
        .long 166725
        .align 4
        .globl _const_3
_const_3:
        .long 100418
# mark_begin;
# Threads 4
        .align    16,0x90
	.globl tmr_ocl_num_int_el
tmr_ocl_num_int_el:
..B1.1:                         # Preds ..B1.0 Latency 93
        pushq     %rbp                                          #
        movq      %rsp, %rbp                                    #
        subq      $1264, %rsp                                   # c1
        movq      %r12, -1192(%rbp)                             # c1
        movq      %rdi, %r12                                    # c5
        movl      %esi, -408(%rbp)                              # c5
        movq      %r15, -1216(%rbp)                             # c9
        xorl      %ecx, %ecx                                    # c9
        movq      88(%r12), %rax                                # c13
        movq      (%r12), %rsi                                  # c13
        movq      %rax, -392(%rbp)                              # c17
        movq      16(%r12), %rax                                # c17
        movq      80(%r12), %rdx                                # c21
        movq      72(%r12), %r8                                 # c21
        movq      %rdx, -384(%rbp)                              # c25
        movq      48(%r12), %r9                                 # c25
        movq      %r8, -368(%rbp)                               # c29
        movq      32(%r12), %r10                                # c29
        movq      24(%r12), %r11                                # c33
        movq      8(%r12), %rdx                                 # c33
        lea       24(%rax), %r8                                 # c37
        movl      (%rsi), %r15d                                 # c37
        movl      $4, -1256(%rbp)                               # c41
        movq      %r14, -1208(%rbp)                             # c45
        movq      %rdi, %r12                                    # c45
        movq      %r13, -1200(%rbp)                             # c49
        movq      %rdi, %r13                                    # c49
        movq      %rcx, -344(%rbp)                              # c53
        movq      %rdi, %r14                                    # c53
        movq      %r9, -1248(%rbp)                              # c57
        movq      %rdi, %r9                                     # c57
        movq      %r10, -376(%rbp)                              # c61
        movq      %rdi, %r10                                    # c61
        movq      %r11, -1240(%rbp)                             # c65
        movq      %rdi, %r11                                    # c65
        movq      %rdx, -1232(%rbp)                             # c69
        movq      %rdi, %rdx                                    # c69
        movq      %r8, -1224(%rbp)                              # c73
        movq      %rdi, %r8                                     # c73
        movq      %rdi, -352(%rbp)                              # c77
        movq      %rcx, -440(%rbp)                              # c77
        movq      %rdi, -360(%rbp)                              # c81
        movq      %rdi, %rcx                                    # c81
        movq      %rdi, -400(%rbp)                              # c85
        movq      %rdi, -416(%rbp)                              # c85
        movq      %rdi, -424(%rbp)                              # c89
        movq      %rdi, -432(%rbp)                              # c89
        movl      %r15d, -448(%rbp)                             # c93
                                # LOE rax rdx rcx rbx rsi rdi r8 r9 r10 r11 r12 r13 r14 edi dil
..B1.36:                        # Preds ..B1.1 Latency 5
        movq      %rax, -1184(%rbp)                             # c1
        movq      %rsi, -1176(%rbp)                             # c1
        movq      %rdi, -296(%rbp)                              # c5
                                # LOE rdx rcx rbx r8 r9 r10 r11 r12 r13 r14
..CL1:
..B1.2:                         # Preds ..B1.22 ..B1.24 ..B1.36 Latency 21
        movq      -344(%rbp), %rsi                              # c1
        movq      -368(%rbp), %r15                              # c1
        movq      %rsi, %rdi                                    # c5
        movq      -440(%rbp), %rax                              # c5
        shlq      $5, %rdi                                      # c9
        movl      (%rdi,%r15), %edi                             # c13
        movq      -392(%rbp), %r15                              # c13
        movl      %edi, (%rax,%r15)                             # c17
        movl      -448(%rbp), %edi                              # c17
        testl     %edi, %edi                                    # c21
        jle       ..B1.26       # Prob 0%                       # c21
                                # LOE rax rdx rcx rbx rsi rdi r8 r9 r10 r11 r12 r13 r14 edi dil
..B1.3:                         # Preds ..B1.2 Latency 1
..CL3:
..B1.4:                         # Preds ..B1.3 Latency 183
        movq      -1248(%rbp), %r14                             # c1
        movl      %edi, %r11d                                   # c1
        movl      (%r14), %r9d                                  # c5
        movq      -1176(%rbp), %r10                             # c5
        imull     %r11d, %r9d                                   # c9
        shll      $4, %r9d                                      # c19
        movl      4(%r10), %r8d                                 # c19
        lea       (%r8,%r8,8), %edi                             # c23
        movq      -392(%rbp), %r12                              # c23
        movq      -440(%rbp), %rcx                              # c27
        lea       5(%rdi,%rdi), %esi                            # c27
        lea       688(%rcx,%r12), %r13                          # c31
        movl      %esi, -968(%rbp)                              # c31
        lea       4(%rdi,%rdi), %esi                            # c35
        movq      %r13, -304(%rbp)                              # c35
        movl      %r9d, -408(%rbp)                              # c39
        lea       (%rdi,%rdi), %r8d                             # c39
        movq      %r13, 8(%r12,%rcx)                            # c43
        lea       15(%rdi,%rdi), %r9d                           # c43
        lea       14(%rdi,%rdi), %r10d                          # c47
        lea       13(%rdi,%rdi), %r11d                          # c47
        lea       12(%rdi,%rdi), %r12d                          # c51
        lea       11(%rdi,%rdi), %r13d                          # c51
        lea       10(%rdi,%rdi), %r14d                          # c55
        lea       9(%rdi,%rdi), %r15d                           # c55
        lea       8(%rdi,%rdi), %eax                            # c59
        lea       7(%rdi,%rdi), %edx                            # c59
        lea       6(%rdi,%rdi), %ecx                            # c63
        movl      %esi, -960(%rbp)                              # c63
        lea       3(%rdi,%rdi), %esi                            # c67
        lea       2(%rdi,%rdi), %edi                            # c67
        movl      %edi, -952(%rbp)                              # c71
        movl      %r8d, %edi                                    # c71
        orl       $1, %edi                                      # c75
        movslq    %ecx, %rcx                                    # c75
        movslq    %edi, %rdi                                    # c79
        movslq    %r8d, %r8                                     # c79
        movslq    %r10d, %r10                                   # c83
        movq      %rdi, -920(%rbp)                              # c83
        movq      %rcx, -944(%rbp)                              # c87
        movslq    %r14d, %r14                                   # c87
        movq      -1240(%rbp), %rdi                             # c91
        movslq    %eax, %rax                                    # c91
        movl      -968(%rbp), %ecx                              # c95
        movslq    %edx, %rdx                                    # c95
        movslq    %ecx, %rcx                                    # c99
        movslq    %r9d, %r9                                     # c99
        movslq    %r11d, %r11                                   # c103
        movq      %rcx, -936(%rbp)                              # c103
        lea       (%rdi,%r8,8), %r8                             # c107
        lea       (%rdi,%r10,8), %r10                           # c107
        movslq    %r13d, %r13                                   # c111
        movslq    %r15d, %r15                                   # c111
        movq      %r8, -432(%rbp)                               # c115
        lea       (%rdi,%rax,8), %r8                            # c115
        movq      %r10, -424(%rbp)                              # c119
        lea       (%rdi,%r14,8), %r10                           # c119
        lea       (%rdi,%rdx,8), %r14                           # c123
        movq      -944(%rbp), %rdx                              # c123
        movq      -936(%rbp), %rax                              # c127
        movslq    %esi, %rsi                                    # c127
        movslq    %r12d, %r12                                   # c131
        movl      -960(%rbp), %ecx                              # c131
        movq      %rsi, -928(%rbp)                              # c135
        lea       (%rdi,%r9,8), %r9                             # c135
        movl      -952(%rbp), %esi                              # c139
        lea       (%rdi,%r11,8), %r11                           # c139
        movslq    %ecx, %rcx                                    # c143
        movslq    %esi, %rsi                                    # c143
        movq      %r9, -400(%rbp)                               # c147
        lea       (%rdi,%r15,8), %r9                            # c147
        movq      %r11, -296(%rbp)                              # c151
        lea       (%rdi,%r13,8), %r11                           # c151
        lea       (%rdi,%rdx,8), %r13                           # c155
        lea       (%rdi,%rax,8), %r15                           # c155
        movq      -928(%rbp), %rdx                              # c159
        lea       (%rdi,%r12,8), %r12                           # c159
        movq      -920(%rbp), %rax                              # c163
        movq      %r12, -352(%rbp)                              # c163
        lea       (%rdi,%rcx,8), %r12                           # c167
        lea       (%rdi,%rdx,8), %rcx                           # c167
        lea       (%rdi,%rsi,8), %rdx                           # c171
        lea       (%rdi,%rax,8), %rsi                           # c171
        movq      $0, -312(%rbp)                                # c175
        movq      %r15, -360(%rbp)                              # c179
        movq      %rsi, -416(%rbp)                              # c179
                                # LOE rdx rcx rbx r8 r9 r10 r11 r12 r13 r14
..B1.35:                        # Preds ..B1.4 Latency 25
        movq      %rcx, -288(%rbp)                              # c1
        movq      -440(%rbp), %r15                              # c1
        movq      %r10, -280(%rbp)                              # c5
        movq      -344(%rbp), %rcx                              # c5
        movq      %r14, -272(%rbp)                              # c9
        movq      -376(%rbp), %r14                              # c9
        movq      %r12, -264(%rbp)                              # c13
        movq      -392(%rbp), %r12                              # c13
        movq      %r11, -256(%rbp)                              # c17
        movq      %r8, -248(%rbp)                               # c17
        movq      %rdx, -336(%rbp)                              # c21
        movq      %r9, -328(%rbp)                               # c21
        movq      %r13, -320(%rbp)                              # c25
                                # LOE rcx rbx r12 r14 r15
..CL4:
..B1.5:                         # Preds ..B1.17 ..B1.35 Latency 461
        movq      %r14, -376(%rbp)                              # c1
        lea       480(%r12,%r15), %r14                          # c1
        movq      %r14, -224(%rbp)                              # c5
        lea       512(%r12,%r15), %r14                          # c5
        movq      %r14, -216(%rbp)                              # c9
        lea       520(%r12,%r15), %r14                          # c9
        movq      %r14, -208(%rbp)                              # c13
        lea       528(%r12,%r15), %r14                          # c13
        movq      %r14, -200(%rbp)                              # c17
        lea       536(%r12,%r15), %r14                          # c17
        movq      %r14, -192(%rbp)                              # c21
        lea       544(%r12,%r15), %r14                          # c21
        movq      %r14, -184(%rbp)                              # c25
        lea       552(%r12,%r15), %r14                          # c25
        movq      %r14, -176(%rbp)                              # c29
        lea       560(%r12,%r15), %r14                          # c29
        movq      %r14, -168(%rbp)                              # c33
        lea       568(%r12,%r15), %r14                          # c33
        movq      %r14, -160(%rbp)                              # c37
        lea       576(%r12,%r15), %r14                          # c37
        movq      %r14, -152(%rbp)                              # c41
        lea       584(%r12,%r15), %r14                          # c41
        movq      %r14, -144(%rbp)                              # c45
        lea       592(%r12,%r15), %r14                          # c45
        movq      %r14, -136(%rbp)                              # c49
        lea       600(%r12,%r15), %r14                          # c49
        movq      %r14, -128(%rbp)                              # c53
        lea       608(%r12,%r15), %r14                          # c53
        movq      %r14, -120(%rbp)                              # c57
        lea       616(%r12,%r15), %r14                          # c57
        movq      %r14, -112(%rbp)                              # c61
        lea       624(%r12,%r15), %r14                          # c61
        movq      %r14, -104(%rbp)                              # c65
        lea       632(%r12,%r15), %r14                          # c65
        movq      %r14, -96(%rbp)                               # c69
        lea       640(%r12,%r15), %r14                          # c69
        movq      %r14, -88(%rbp)                               # c73
        lea       648(%r12,%r15), %r14                          # c73
        movq      %r14, -80(%rbp)                               # c77
        lea       656(%r12,%r15), %r14                          # c77
        movq      %r14, -72(%rbp)                               # c81
        lea       664(%r12,%r15), %r14                          # c81
        movq      %r14, -64(%rbp)                               # c85
        lea       672(%r12,%r15), %r14                          # c85
        movq      %r14, -56(%rbp)                               # c89
        lea       680(%r12,%r15), %r14                          # c89
        movq      %r14, -48(%rbp)                               # c93
        lea       696(%r12,%r15), %r14                          # c93
        movq      %r14, -40(%rbp)                               # c97
        lea       704(%r12,%r15), %r14                          # c97
        movq      %r14, -32(%rbp)                               # c101
        lea       712(%r12,%r15), %r14                          # c101
        movq      -312(%rbp), %r11                              # c105
        movq      %r14, -24(%rbp)                               # c105
        movl      %r11d, %r13d                                  # c109
        shll      $4, %r13d                                     # c113
        lea       720(%r12,%r15), %r14                          # c113
        addl      -408(%rbp), %r13d                             # c117
        movq      %r14, -16(%rbp)                               # c117
        lea       728(%r12,%r15), %r14                          # c121
        movq      %r11, 16(%r15,%r12)                           # c121
        movq      %r14, -8(%rbp)                                # c125
        lea       448(%r12,%r15), %rdi                          # c125
        movq      -432(%rbp), %r11                              # c129
        movq      %rcx, -344(%rbp)                              # c129
        lea       456(%r12,%r15), %r10                          # c133
        lea       464(%r12,%r15), %r9                           # c133
        movq      -416(%rbp), %r14                              # c137
        lea       472(%r12,%r15), %r8                           # c137
        movq      (%r11), %r11                                  # c141
        lea       488(%r12,%r15), %rsi                          # c141
        movq      %r11, 32(%r15,%r12)                           # c145
        lea       496(%r12,%r15), %rdx                          # c145
        movq      (%r14), %r11                                  # c149
        lea       504(%r12,%r15), %rcx                          # c149
        movq      -336(%rbp), %r14                              # c153
        movq      %r11, 40(%r15,%r12)                           # c153
        xorl      %eax, %eax                                    # c157
        movq      %rdi, 160(%r15,%r12)                          # c157
        movq      %rax, (%rdi)                                  # c161
        movq      (%r14), %r11                                  # c161
        movq      -288(%rbp), %r14                              # c165
        movq      %r11, 48(%r15,%r12)                           # c165
        movq      %r10, 168(%r15,%r12)                          # c169
        movq      %r9, 176(%r15,%r12)                           # c169
        movq      %rax, (%r10)                                  # c173
        movq      (%r14), %r11                                  # c173
        movq      -264(%rbp), %r14                              # c177
        movq      %r11, 56(%r15,%r12)                           # c177
        movq      %rax, (%r9)                                   # c181
        movq      (%r14), %r11                                  # c181
        movq      -360(%rbp), %r14                              # c185
        movq      %r11, 64(%r15,%r12)                           # c185
        movq      %r8, 184(%r15,%r12)                           # c189
        movq      %rsi, 200(%r15,%r12)                          # c189
        movq      %rax, (%r8)                                   # c193
        movq      (%r14), %r11                                  # c193
        movq      -320(%rbp), %r14                              # c197
        movq      %r11, 72(%r15,%r12)                           # c197
        movq      %rax, (%rsi)                                  # c201
        movq      (%r14), %r11                                  # c201
        movq      -272(%rbp), %r14                              # c205
        movq      %r11, 80(%r15,%r12)                           # c205
        movq      %rdx, 208(%r15,%r12)                          # c209
        movq      %rcx, 216(%r15,%r12)                          # c209
        movq      %rax, (%rdx)                                  # c213
        movq      (%r14), %r11                                  # c213
        movq      -248(%rbp), %r14                              # c217
        movq      %r11, 88(%r15,%r12)                           # c217
        movq      %rax, (%rcx)                                  # c221
        movq      (%r14), %r11                                  # c221
        movq      -328(%rbp), %r14                              # c225
        movq      %r11, 96(%r15,%r12)                           # c225
        movl      %r13d, 24(%r15,%r12)                          # c229
        movq      (%r14), %r11                                  # c229
        movq      -280(%rbp), %r14                              # c233
        movq      %r11, 104(%r15,%r12)                          # c233
        movl      $0, -240(%rbp)                                # c237
        movq      %rax, -232(%rbp)                              # c241
        movq      (%r14), %r11                                  # c241
        movq      -256(%rbp), %r14                              # c245
        movq      %r11, 112(%r15,%r12)                          # c245
        movq      (%r14), %r11                                  # c249
        movq      -352(%rbp), %r14                              # c249
        movq      %r11, 120(%r15,%r12)                          # c253
        movq      (%r14), %r11                                  # c253
        movq      -296(%rbp), %r14                              # c257
        movq      %r11, 128(%r15,%r12)                          # c257
        movq      (%r14), %r11                                  # c261
        movq      -424(%rbp), %r14                              # c261
        movq      %r11, 136(%r15,%r12)                          # c265
        movq      (%r14), %r11                                  # c265
        movq      -400(%rbp), %r14                              # c269
        movq      %r11, 144(%r15,%r12)                          # c269
        movq      -224(%rbp), %rdi                              # c273
        movq      -216(%rbp), %rdx                              # c273
        movq      -208(%rbp), %rcx                              # c277
        movq      %rdi, 192(%r15,%r12)                          # c277
        movq      %rax, (%rdi)                                  # c281
        movq      -200(%rbp), %rsi                              # c281
        movq      -192(%rbp), %r8                               # c285
        movq      %rdx, 224(%r15,%r12)                          # c285
        movq      %rax, (%rdx)                                  # c289
        movq      -184(%rbp), %r9                               # c289
        movq      -176(%rbp), %r10                              # c293
        movq      %rcx, 232(%r15,%r12)                          # c293
        movq      %rax, (%rcx)                                  # c297
        movq      (%r14), %r11                                  # c297
        movq      %rsi, 240(%r15,%r12)                          # c301
        movq      %r8, 248(%r15,%r12)                           # c301
        movq      %rax, (%rsi)                                  # c305
        movq      -168(%rbp), %rdx                              # c305
        movq      %rax, (%r8)                                   # c309
        movq      -160(%rbp), %rcx                              # c309
        movq      %r9, 256(%r15,%r12)                           # c313
        movq      %r10, 264(%r15,%r12)                          # c313
        movq      %rax, (%r9)                                   # c317
        movq      -152(%rbp), %rsi                              # c317
        movq      %rax, (%r10)                                  # c321
        movq      -144(%rbp), %rdi                              # c321
        movq      -136(%rbp), %r8                               # c325
        movq      %r11, 152(%r15,%r12)                          # c325
        movq      %rdx, 272(%r15,%r12)                          # c329
        movq      %rcx, 280(%r15,%r12)                          # c329
        movq      %rax, (%rdx)                                  # c333
        movq      -128(%rbp), %r9                               # c333
        movq      -120(%rbp), %r10                              # c337
        movq      %rax, (%rcx)                                  # c337
        movq      %rsi, 288(%r15,%r12)                          # c341
        movq      %rdi, 296(%r15,%r12)                          # c341
        movq      %rax, (%rsi)                                  # c345
        movq      -112(%rbp), %r11                              # c345
        movq      %rax, (%rdi)                                  # c349
        movq      -104(%rbp), %r14                              # c349
        movq      %r8, 304(%r15,%r12)                           # c353
        movq      %r9, 312(%r15,%r12)                           # c353
        movq      %rax, (%r8)                                   # c357
        movq      -96(%rbp), %rdx                               # c357
        movq      %rax, (%r9)                                   # c361
        movq      -88(%rbp), %rcx                               # c361
        movq      %r10, 320(%r15,%r12)                          # c365
        movq      %r11, 328(%r15,%r12)                          # c365
        movq      %rax, (%r10)                                  # c369
        movq      -80(%rbp), %rsi                               # c369
        movq      -72(%rbp), %rdi                               # c373
        movq      %rax, (%r11)                                  # c373
        movq      %r14, 336(%r15,%r12)                          # c377
        movq      %rdx, 344(%r15,%r12)                          # c377
        movq      %rax, (%r14)                                  # c381
        movq      -64(%rbp), %r8                                # c381
        movq      -56(%rbp), %r9                                # c385
        movq      %rax, (%rdx)                                  # c385
        movq      %rcx, 352(%r15,%r12)                          # c389
        movq      %rsi, 360(%r15,%r12)                          # c389
        movq      %rax, (%rcx)                                  # c393
        movq      -48(%rbp), %r10                               # c393
        movq      -304(%rbp), %r11                              # c397
        movq      %rax, (%rsi)                                  # c397
        movq      %rdi, 368(%r15,%r12)                          # c401
        movq      %r8, 376(%r15,%r12)                           # c401
        movq      %rax, (%rdi)                                  # c405
        movq      -24(%rbp), %rcx                               # c405
        movq      %rax, (%r8)                                   # c409
        movq      -40(%rbp), %r14                               # c409
        movq      %r9, 384(%r15,%r12)                           # c413
        movq      %r10, 392(%r15,%r12)                          # c413
        movq      %rax, (%r9)                                   # c417
        movq      -32(%rbp), %rdx                               # c417
        movq      %rax, (%r10)                                  # c421
        movq      -16(%rbp), %rsi                               # c421
        movq      %rax, (%r11)                                  # c425
        movq      -8(%rbp), %rdi                                # c425
        addl      (%r15,%r12), %r13d                            # c429
        movq      %rcx, 416(%r15,%r12)                          # c429
        movq      %rax, (%rcx)                                  # c433
        movq      -344(%rbp), %rcx                              # c433
        movl      %r13d, 440(%r15,%r12)                         # c437
        movq      %r14, 400(%r15,%r12)                          # c437
        movq      %rax, (%r14)                                  # c441
        movq      -384(%rbp), %r13                              # c441
        movq      %rdx, 408(%r15,%r12)                          # c445
        movq      %rsi, 424(%r15,%r12)                          # c445
        movq      %rax, (%rdx)                                  # c449
        cmpq      %r13, %rcx                                    # c449
        movq      %rax, (%rsi)                                  # c453
        movq      -376(%rbp), %r14                              # c453
        movq      %rdi, 432(%r15,%r12)                          # c457
        movq      %rax, (%rdi)                                  # c457
        jae       ..B1.9        # Prob 0%                       # c461
                                # LOE rax rcx rbx r12 r14 r15
..B1.6:                         # Preds ..B1.5 Latency 1
..CL6:
..B1.7:                         # Preds ..B1.6 Latency 17
        incq      %rcx                                          # c1
        addq      $736, %r15                                    # c1
        movl      -1256(%rbp), %eax                             # c5
        movq      %rcx, -232(%rbp)                              # c5
        movl      %eax, -240(%rbp)                              # c9
        movq      %r15, %rax                                    # c9
        cmpl      $4, -1256(%rbp)                               # c13
        je        ..B1.24       # Prob 0%                       # c17
                                # LOE rax rcx rbx r12 r14 r15 ecx cl ch
..B1.8:                         # Preds ..B1.7 Latency 1
..CL5:
..B1.9:                         # Preds ..B1.23 ..B1.8 ..B1.5 Latency 53
        lea       (%r12,%rax), %r8                              # c1
        movq      -1240(%rbp), %rsi                             # c1
        movl      440(%r8), %edx                                # c5
        xorl      %r9d, %r9d                                    # c5
        lea       (%rdx,%rdx,8), %ecx                           # c9
        movl      $1, %edx                                      # c9
        addl      %ecx, %ecx                                    # c13
        kmov      %edx, %k1                                     # c13
        movslq    %ecx, %rcx                                    # c17
        movq      -1224(%rbp), %rdx                             # c17
        movq      %rcx, %rdi                                    # c21
        vbroadcastsd (%rsi,%rcx,8), %zmm5{%k1}                  # c25
        orq       $1, %rdi                                      # c25
        vbroadcasti64x4 _const_3(%rip), %zmm30                  # c29
        vbroadcasti64x4 _const_3(%rip), %zmm4                   # c33
        vbroadcasti32x4 _const_3(%rip), %zmm3                   # c37
        vbroadcasti32x4 _const_3(%rip), %zmm2                   # c41
        vpbroadcastd _const_3(%rip), %zmm1                      # c45
        vpbroadcastd _const_3(%rip), %zmm0                      # c49
                                # LOE rax rdx rcx rbx rdi r8 r9 r12 r14 zmm0 zmm1 zmm2 zmm3 zmm4 zmm5 zmm30 k1
..B1.34:                        # Preds ..B1.9 Latency 21
        xorl      %esi, %esi                                    # c1
        incl      %esi                                          #
        movq      %rdi, -1160(%rbp)                             # c1
        kmov      %esi, %k2                                     # c5
        movq      %rcx, -1152(%rbp)                             # c5
        vpackstorelpd %zmm5, -1144(%rbp){%k2}                   # c9
        movq      %r8, -1264(%rbp)                              # c13
        movq      %r9, -992(%rbp)                               # c13
        movq      %rax, -1168(%rbp)                             # c17
        movq      %r14, -376(%rbp)                              # c17
        movq      %r12, -392(%rbp)                              # c21
        jmp       ..B1.10       # Prob 100%                     # c21
                                # LOE rdx rbx zmm30 k1
..B1.14:                        # Preds ..B1.13 Latency 1
..CL7:
..B1.10:                        # Preds ..B1.14 ..B1.34 Latency 817
        vcvtps2pd _const_0(%rip){1to8}, %zmm25{%k1}             # c1
        vcvtps2pd _const_1(%rip){1to8}, %zmm1{%k1}              # c5
        vcvtps2pd _const_2(%rip){1to8}, %zmm8{%k1}              # c9
        vbroadcastsd -1144(%rbp), %zmm24                        # c13
        movq      -992(%rbp), %r14                              # c17
        movl      $488, %eax                                    # c17
        lea       (,%r14,8), %r10                               # c21
        shlq      $5, %r14                                      # c25
        lea       (%r10,%r10,2), %r10                           # c25
        movq      %r10, %r9                                     # c29
        movq      %r10, %r15                                    # c29
        orq       $7, %r9                                       # c33
        orq       $3, %r15                                      # c33
        movq      %r9, -1032(%rbp)                              # c37
        lea       8(%r10), %r9                                  # c37
        movq      %r15, -1088(%rbp)                             # c41
        movq      %r9, %r15                                     # c41
        orq       $3, %r15                                      # c45
        movq      %r10, %rsi                                    # c45
        lea       12(%r10), %rdi                                # c49
        movq      %r15, -1064(%rbp)                             # c49
        movq      %r9, %r15                                     # c53
        orq       $1, %r9                                       # c53
        orq       $2, %rsi                                      # c57
        movq      %r9, -1024(%rbp)                              # c57
        movq      %rdi, %r9                                     # c61
        movq      %rsi, -1080(%rbp)                             # c61
        movq      %r10, %rsi                                    # c65
        orq       $3, %r9                                       # c65
        orq       $5, %rsi                                      # c69
        lea       16(%r10), %rcx                                # c69
        orq       $2, %r15                                      # c73
        movq      %r9, -1072(%rbp)                              # c73
        movq      %rdi, %r9                                     # c77
        orq       $1, %rdi                                      # c77
        movq      %rsi, -1040(%rbp)                             # c81
        lea       20(%r10), %rsi                                # c81
        movq      %r15, -1048(%rbp)                             # c85
        movq      %rcx, %r15                                    # c85
        movq      %rdi, -1016(%rbp)                             # c89
        movq      %rcx, %rdi                                    # c89
        orq       $1, %rcx                                      # c93
        movq      %r14, %r12                                    # c93
        movq      %rcx, -1008(%rbp)                             # c97
        movq      %rsi, %rcx                                    # c97
        orq       $3, %rcx                                      # c101
        orq       $16, %r12                                     # c101
        movq      %rcx, -1104(%rbp)                             # c105
        movq      %r14, %r8                                     # c105
        movq      -1232(%rbp), %rcx                             # c109
        movq      %r14, %r13                                    # c109
        vaddpd    (%r12,%rcx){1to8}, %zmm25, %zmm10{%k1}        # c113
        orq       $8, %r8                                       # c113
        vsubpd    (%r14,%rcx){1to8}, %zmm25, %zmm6{%k1}         # c117
        orq       $24, %r13                                     # c117
        vmulpd    (%r14,%rcx){1to8}, %zmm8, %zmm4{%k1}          # c121
        orq       $3, %rdi                                      # c121
        vmulpd    (%r8,%rcx){1to8}, %zmm1, %zmm19{%k1}          # c125
        movq      %rdi, -1056(%rbp)                             # c125
        vmulpd    %zmm8, %zmm10, %zmm12{%k1}                    # c129
        movq      %rsi, %rdi                                    # c129
        vbroadcastsd (%r13,%rcx), %zmm16{%k1}                   # c133
        orq       $2, %rdi                                      # c133
        vsubpd    (%r12,%rcx){1to8}, %zmm25, %zmm9{%k1}         # c137
        movq      %rdi, -1128(%rbp)                             # c137
        vmulpd    (%r14,%rcx){1to8}, %zmm1, %zmm18{%k1}         # c141
        movl      $1, %edi                                      # c141
        vmulpd    (%r8,%rcx){1to8}, %zmm8, %zmm22{%k1}          # c145
        kmov      %edi, %k2                                     # c145
        vsubpd    (%r8,%rcx){1to8}, %zmm6, %zmm0{%k1}           # c149
        vpackstorelpd %zmm16, -1000(%rbp){%k2}                  # c149
        vmulpd    %zmm8, %zmm9, %zmm13{%k1}                     # c153
        movq      -1184(%rbp), %r13                             # c153
        vbroadcastsd (%r13,%r10,8), %zmm7{%k1}                  # c157
        movq      %r10, %r11                                    # c157
        vmulpd    %zmm8, %zmm0, %zmm23{%k1}                     # c161
        movq      -1152(%rbp), %r8                              # c161
        vpackstorelpd %zmm7, -1136(%rbp){%k2}                   # c165
        vmulpd    %zmm1, %zmm9, %zmm26{%k1}                     # c169
        movq      -1240(%rbp), %r12                             # c169
        vmulpd    96(%r12,%r8,8){1to8}, %zmm4, %zmm3{%k1}       # c173
        orq       $1, %r11                                      # c173
        vmulpd    56(%r12,%r8,8){1to8}, %zmm19, %zmm2{%k1}      # c177
        movq      %r11, -1112(%rbp)                             # c177
        vmulpd    %zmm1, %zmm0, %zmm31{%k1}                     # c181
        vpackstorelpd %zmm3, -904(%rbp){%k2}                    # c181
        vmulpd    120(%r12,%r8,8){1to8}, %zmm12, %zmm14{%k1}    # c185
        vpackstorelpd %zmm2, -896(%rbp){%k2}                    # c185
        vmulpd    120(%r12,%r8,8){1to8}, %zmm22, %zmm5{%k1}     # c189
        movq      %r10, %r11                                    # c189
        vmulpd    112(%r12,%r8,8){1to8}, %zmm4, %zmm16{%k1}     # c193
        vpackstorelpd %zmm14, -888(%rbp){%k2}                   # c193
        vmulpd    104(%r12,%r8,8){1to8}, %zmm4, %zmm17{%k1}     # c197
        vpackstorelpd %zmm5, -912(%rbp){%k2}                    # c197
        vmulpd    64(%r12,%r8,8){1to8}, %zmm19, %zmm7{%k1}      # c201
        orq       $4, %r11                                      # c201
        vmulpd    48(%r12,%r8,8){1to8}, %zmm19, %zmm8{%k1}      # c205
        movq      %r11, -1096(%rbp)                             # c205
        vmulpd    %zmm1, %zmm10, %zmm11{%k1}                    # c209
        movq      %r10, %r11                                    # c209
        vmulpd    40(%r12,%r8,8){1to8}, %zmm18, %zmm2{%k1}      # c213
        orq       $2, %r15                                      # c213
        vmulpd    32(%r12,%r8,8){1to8}, %zmm18, %zmm3{%k1}      # c217
        orq       $6, %r11                                      # c217
        vmulpd    24(%r12,%r8,8){1to8}, %zmm18, %zmm4{%k1}      # c221
        orq       $2, %r9                                       # c221
        vmulpd    136(%r12,%r8,8){1to8}, %zmm12, %zmm18{%k1}    # c225
        movq      %r15, -1120(%rbp)                             # c225
        vmulpd    136(%r12,%r8,8){1to8}, %zmm22, %zmm20{%k1}    # c229
        orq       $1, %rsi                                      # c229
        vmulpd    128(%r12,%r8,8){1to8}, %zmm12, %zmm19{%k1}    # c233
        movq      -1160(%rbp), %r14                             # c233
        vmulpd    112(%r12,%r8,8){1to8}, %zmm12, %zmm14{%k1}    # c237
        vmulpd    104(%r12,%r8,8){1to8}, %zmm12, %zmm15{%k1}    # c241
        vmulpd    96(%r12,%r8,8){1to8}, %zmm12, %zmm12{%k1}     # c245
        vmulpd    48(%r12,%r8,8){1to8}, %zmm13, %zmm27{%k1}     # c249
        vmulpd    72(%r12,%r8,8){1to8}, %zmm23, %zmm29{%k1}     # c253
        vpackstorelpd %zmm12, -880(%rbp){%k2}                   # c253
        vmulpd    128(%r12,%r8,8){1to8}, %zmm22, %zmm21{%k1}    # c257
        vpackstorelpd %zmm27, -872(%rbp){%k2}                   # c257
        vmulpd    64(%r12,%r8,8){1to8}, %zmm13, %zmm5{%k1}      # c261
        vpackstorelpd %zmm29, -864(%rbp){%k2}                   # c261
        vmulpd    56(%r12,%r8,8){1to8}, %zmm13, %zmm6{%k1}      # c265
        vmulpd    40(%r12,%r8,8){1to8}, %zmm13, %zmm0{%k1}      # c269
        vmulpd    32(%r12,%r8,8){1to8}, %zmm13, %zmm1{%k1}      # c273
        vmulpd    24(%r12,%r8,8){1to8}, %zmm13, %zmm28{%k1}     # c277
        vmulpd    16(%r12,%r8,8){1to8}, %zmm26, %zmm22{%k1}     # c281
        vmulpd    (%r12,%r14,8){1to8}, %zmm26, %zmm27{%k1}      # c285
        vmulpd    -1144(%rbp){1to8}, %zmm26, %zmm26{%k1}        # c289
        vmulpd    88(%r12,%r8,8){1to8}, %zmm23, %zmm12{%k1}     # c293
        vmulpd    80(%r12,%r8,8){1to8}, %zmm23, %zmm13{%k1}     # c297
        vmulpd    16(%r12,%r8,8){1to8}, %zmm31, %zmm29{%k1}     # c301
        vmulpd    (%r12,%r14,8){1to8}, %zmm31, %zmm23{%k1}      # c305
        vmulpd    %zmm31, %zmm24, %zmm24{%k1}                   # c309
        vaddpd    %zmm1, %zmm27, %zmm31{%k1}                    # c313
        vaddpd    %zmm28, %zmm26, %zmm1{%k1}                    # c317
        vaddpd    %zmm2, %zmm29, %zmm2{%k1}                     # c321
        vaddpd    %zmm3, %zmm23, %zmm29{%k1}                    # c325
        vaddpd    %zmm4, %zmm24, %zmm28{%k1}                    # c329
        vmulpd    88(%r12,%r8,8){1to8}, %zmm11, %zmm9{%k1}      # c333
        vmulpd    80(%r12,%r8,8){1to8}, %zmm11, %zmm10{%k1}     # c337
        vmulpd    72(%r12,%r8,8){1to8}, %zmm11, %zmm11{%k1}     # c341
        vaddpd    %zmm0, %zmm22, %zmm0{%k1}                     # c345
        vaddpd    %zmm5, %zmm22, %zmm23{%k1}                    # c349
        vaddpd    %zmm6, %zmm27, %zmm24{%k1}                    # c353
        vaddpd    -872(%rbp){1to8}, %zmm26, %zmm22{%k1}         # c357
        vaddpd    %zmm7, %zmm2, %zmm27{%k1}                     # c361
        vaddpd    -896(%rbp){1to8}, %zmm29, %zmm26{%k1}         # c365
        vaddpd    %zmm8, %zmm28, %zmm28{%k1}                    # c369
        vaddpd    %zmm9, %zmm23, %zmm8{%k1}                     # c373
        vaddpd    %zmm9, %zmm0, %zmm7{%k1}                      # c377
        vaddpd    %zmm10, %zmm24, %zmm9{%k1}                    # c381
        vaddpd    %zmm10, %zmm31, %zmm31{%k1}                   # c385
        vaddpd    %zmm11, %zmm22, %zmm10{%k1}                   # c389
        vaddpd    %zmm11, %zmm1, %zmm11{%k1}                    # c393
        vaddpd    %zmm12, %zmm27, %zmm6{%k1}                    # c397
        vaddpd    %zmm13, %zmm26, %zmm0{%k1}                    # c401
        vaddpd    -864(%rbp){1to8}, %zmm28, %zmm1{%k1}          # c405
        vaddpd    %zmm16, %zmm6, %zmm16{%k1}                    # c409
        vaddpd    %zmm17, %zmm0, %zmm17{%k1}                    # c413
        vaddpd    -904(%rbp){1to8}, %zmm1, %zmm26{%k1}          # c417
        vaddpd    %zmm14, %zmm7, %zmm13{%k1}                    # c421
        vaddpd    %zmm15, %zmm31, %zmm14{%k1}                   # c425
        vaddpd    -880(%rbp){1to8}, %zmm11, %zmm15{%k1}         # c429
        vaddpd    %zmm18, %zmm8, %zmm7{%k1}                     # c433
        vaddpd    %zmm19, %zmm9, %zmm3{%k1}                     # c437
        vaddpd    -888(%rbp){1to8}, %zmm10, %zmm10{%k1}         # c441
        vaddpd    %zmm20, %zmm16, %zmm20{%k1}                   # c445
        vaddpd    %zmm21, %zmm17, %zmm21{%k1}                   # c449
        vaddpd    -912(%rbp){1to8}, %zmm26, %zmm9{%k1}          # c453
        vmulpd    %zmm13, %zmm3, %zmm0{%k1}                     # c457
        vmulpd    %zmm7, %zmm14, %zmm8{%k1}                     # c461
        vmulpd    %zmm14, %zmm10, %zmm11{%k1}                   # c465
        vmulpd    %zmm13, %zmm10, %zmm22{%k1}                   # c469
        vmulpd    %zmm3, %zmm15, %zmm1{%k1}                     # c473
        vmulpd    %zmm7, %zmm15, %zmm2{%k1}                     # c477
        vmulpd    %zmm20, %zmm15, %zmm6{%k1}                    # c481
        vmulpd    %zmm20, %zmm14, %zmm19{%k1}                   # c485
        vmulpd    %zmm20, %zmm10, %zmm17{%k1}                   # c489
        vmulpd    %zmm20, %zmm3, %zmm18{%k1}                    # c493
        vmulpd    %zmm21, %zmm15, %zmm5{%k1}                    # c497
        vmulpd    %zmm13, %zmm21, %zmm20{%k1}                   # c501
        vmulpd    %zmm21, %zmm10, %zmm4{%k1}                    # c505
        vmulpd    %zmm21, %zmm7, %zmm21{%k1}                    # c509
        vmulpd    %zmm9, %zmm3, %zmm3{%k1}                      # c513
        vmulpd    %zmm9, %zmm7, %zmm23{%k1}                     # c517
        vmulpd    %zmm14, %zmm9, %zmm12{%k1}                    # c521
        vmulpd    %zmm9, %zmm13, %zmm16{%k1}                    # c525
        vsubpd    %zmm0, %zmm8, %zmm10{%k1}                     # c529
        vsubpd    %zmm11, %zmm1, %zmm9{%k1}                     # c533
        vsubpd    %zmm2, %zmm22, %zmm8{%k1}                     # c537
        vsubpd    %zmm21, %zmm18, %zmm1{%k1}                    # c541
        vsubpd    %zmm3, %zmm4, %zmm11{%k1}                     # c545
        vsubpd    %zmm17, %zmm23, %zmm22{%k1}                   # c549
        vmulpd    %zmm1, %zmm15, %zmm15{%k1}                    # c553
        vmulpd    %zmm11, %zmm13, %zmm4{%k1}                    # c557
        vmulpd    %zmm22, %zmm14, %zmm13{%k1}                   # c561
        vsubpd    %zmm19, %zmm20, %zmm0{%k1}                    # c565
        vsubpd    %zmm5, %zmm12, %zmm7{%k1}                     # c569
        vaddpd    %zmm13, %zmm15, %zmm5{%k1}                    # c573
        vpxorq    %zmm20, %zmm20, %zmm20                        # c577
        vpbroadcastd _const_3(%rip), %zmm3                      # c581
        vaddpd    %zmm4, %zmm5, %zmm4{%k1}                      # c585
        vfixupnanpd %zmm3, %zmm25, %zmm20{%k1}                  # c589
        vcmpeqpd  %zmm20, %zmm20, %k2{%k1}                      # c593
        vcmpneqpd %zmm30{aaaa}, %zmm4, %k0{%k1}                 # c597
        vmovapd   %zmm30{bbbb}, %zmm19                          # c601
        vfixupnanpd %zmm3, %zmm4, %zmm20{%k1}                   # c605
        kor       %k0, %k2                                      # c605
        vmovaps   %zmm19, %zmm3                                 # c609
        kand      %k1, %k2                                      # c609
        vgetmantpd $0, %zmm4, %zmm3{%k2}                        # c613
        vbroadcasti32x4 _const_3(%rip), %zmm21                  # c617
        vcvtpd2ps {rz-sae}, %zmm3, %zmm12                       # c621
        vmovaps   %zmm21{bbbb}, %zmm14                          # c625
        vrcp23ps  %zmm12, %zmm14{%k2}                           # c629
        vsubpd    %zmm16, %zmm6, %zmm6{%k1}                     # c633
        vpxorq    %zmm17, %zmm17, %zmm17                        # c637
        vpxorq    %zmm16, %zmm16, %zmm16                        # c641
        vgetexppd %zmm4, %zmm17{%k1}                            # c645
        vgetexppd %zmm25, %zmm16{%k1}                           # c649
        vcvtps2pd %zmm14, %zmm21                                # c653
        vmovaps   %zmm19, %zmm15                                # c657
        vbroadcasti32x4 _const_3(%rip), %zmm24                  # c661
        vpxorq    %zmm23, %zmm23, %zmm23                        # c665
        vsubpd    {rn-sae}, %zmm17, %zmm16, %zmm23{%k1}         # c669
        vfnmadd231pd {rn-sae}, %zmm21, %zmm3, %zmm15{%k1}       # c673
        vpminsd   %zmm24{aaaa}, %zmm23, %zmm13                  # c677
        vcmpeqpd  %zmm20, %zmm20, %k4{%k1}                      # c681
        vfmadd231pd {rn-sae}, %zmm15, %zmm15, %zmm15{%k1}       # c685
        vmovaps   %zmm21, %zmm20                                # c689
        kmov      %k4, %k3                                      # c689
        vmovaps   %zmm4, %zmm18                                 # c693
        kandn     %k1, %k3                                      # c693
        vpbroadcastd _const_3(%rip), %zmm2                      # c697
        vpminud   %zmm24{bbbb}, %zmm13, %zmm12                  # c701
        vmovapd   %zmm30{cccc}, %zmm26                          # c705
        vfmadd231pd {rn-sae}, %zmm21, %zmm15, %zmm20{%k1}       # c709
        vpxorq    %zmm15, %zmm15, %zmm15                        # c713
        vgetmantpd $0, %zmm25, %zmm15{%k1}                      # c717
        vfixupnanpd %zmm2, %zmm4, %zmm18{%k1}                   # c721
        vaddpd    {rn-sae}, %zmm26, %zmm12, %zmm12{%k1}         # c725
        vmulpd    {rn-sae}, %zmm21, %zmm15, %zmm5{%k4}          # c729
        vpslld    $20, %zmm12, %zmm27                           # c733
        vmulpd    {rn}, %zmm18, %zmm25, %zmm5{%k3}              # c737
        vmovaps   %zmm15, %zmm25                                # c741
        vbroadcasti64x4 _const_3(%rip), %zmm31                  # c745
        vpsrad    $1, %zmm27, %zmm13                            # c749
        vfnmadd231pd {rn-sae}, %zmm5, %zmm3, %zmm25{%k4}        # c753
        vmovaps   %zmm5, %zmm14                                 # c757
        vpandq    %zmm31{aaaa}, %zmm13, %zmm13{%k1}             # c761
        vcmplepd  %zmm30{dddd}, %zmm12, %k2{%k4}                # c765
        vfmadd231pd {rn-sae}, %zmm20, %zmm25, %zmm14{%k4}       # c769
        vfnmadd231pd {rn-sae}, %zmm20, %zmm3, %zmm19{%k4}       # c773
        vmovaps   %zmm15, %zmm12                                # c777
        vpsubd    %zmm13, %zmm27, %zmm28                        # c781
        vmovdqa64 %zmm30{bbbb}, %zmm29                          # c785
        vfmadd231pd {rn-sae}, %zmm20, %zmm19, %zmm20{%k4}       # c789
        vfnmadd231pd {rn-sae}, %zmm14, %zmm3, %zmm12{%k4}       # c793
        vpaddd    %zmm29, %zmm28, %zmm2                         # c797
        vfmadd231pd %zmm20, %zmm12, %zmm14{%k4}                 # c801
        vpandq    %zmm31{dddd}, %zmm2, %zmm2{%k1}               # c805
        vpaddd    %zmm13, %zmm14, %zmm16                        # c809
        movq      $0, -984(%rbp)                                # c813
        vmulpd    %zmm2, %zmm16, %zmm5{%k4}                     # c817
        jknzd     ..B1.30, %k2  # Prob 5%                       # c817
                                # LOE rax rdx rbx rsi r9 r10 r11 zmm0 zmm1 zmm2 zmm3 zmm4 zmm5 zmm6 zmm7 zmm8 zmm9 zmm10 zmm11 zmm1
4 zmm15 zmm16 zmm22 zmm30 k1 k2
..B1.29:                        # Preds ..B1.32 ..B1.31 ..B1.10 Latency 37
        vmulpd    %zmm5, %zmm9, %zmm17{%k1}                     # c1
        vmulpd    %zmm5, %zmm10, %zmm15{%k1}                    # c5
        vmulpd    -1000(%rbp){1to8}, %zmm4, %zmm18{%k1}         # c9
        vmulpd    %zmm5, %zmm8, %zmm16{%k1}                     # c13
        vmulpd    %zmm5, %zmm7, %zmm14{%k1}                     # c17
        vmulpd    %zmm5, %zmm6, %zmm13{%k1}                     # c21
        vmulpd    %zmm5, %zmm0, %zmm12{%k1}                     # c25
        vmulpd    %zmm5, %zmm11, %zmm11{%k1}                    # c29
        vmulpd    %zmm5, %zmm22, %zmm10{%k1}                    # c33
        vmulpd    %zmm5, %zmm1, %zmm9{%k1}                      # c37
                                # LOE rax rdx rbx rsi r9 r10 r11 zmm9 zmm10 zmm11 zmm12 zmm13 zmm14 zmm15 zmm16 zmm17 zmm18 zmm30 k
1
..B1.33:                        # Preds ..B1.29 Latency 9
        movq      %rsi, -976(%rbp)                              # c1
        movq      -984(%rbp), %r14                              # c1
        movq      -1264(%rbp), %r13                             # c5
        movq      -1168(%rbp), %rcx                             # c5
        movq      -1184(%rbp), %rsi                             # c9
        movq      -392(%rbp), %rdi                              # c9
                                # LOE rax rdx rcx rbx rsi rdi r9 r10 r11 r13 r14 zmm9 zmm10 zmm11 zmm12 zmm13 zmm14 zmm15 zmm16 zmm
17 zmm18 k1
..CL8:
..B1.11:                        # Preds ..B1.11 ..B1.33 Latency 761
        movq      -1112(%rbp), %r15                             # c1
        lea       (,%r14,4), %r8                                # c1
        vmulpd    (%rsi,%r15,8){1to8}, %zmm11, %zmm30{%k1}      # c5
        movq      %rax, %r12                                    # c5
        vmulpd    (%rsi,%r15,8){1to8}, %zmm10, %zmm19{%k1}      # c9
        lea       48(%r12), %rax                                # c9
        vmulpd    (%rsi,%r15,8){1to8}, %zmm9, %zmm21{%k1}       # c13
        addq      $8, %r14                                      # c13
        vmulpd    -8(%rdx,%r8){1to8}, %zmm13, %zmm31{%k1}       # c17
        cmpl      $776, %eax                                    # c17
        vmulpd    -8(%rdx,%r8){1to8}, %zmm12, %zmm27{%k1}       # c21
        movq      -1080(%rbp), %r15                             # c21
        vmulpd    (%rsi,%r15,8){1to8}, %zmm14, %zmm23{%k1}      # c25
        vmulpd    (%rsi,%r15,8){1to8}, %zmm13, %zmm20{%k1}      # c29
        vmulpd    (%rsi,%r15,8){1to8}, %zmm12, %zmm22{%k1}      # c33
        movq      -1088(%rbp), %r15                             # c33
        vmulpd    -16(%rdx,%r8){1to8}, %zmm10, %zmm26{%k1}      # c37
        vmulpd    -16(%rdx,%r8){1to8}, %zmm9, %zmm29{%k1}       # c41
        vmulpd    (%rdx,%r8){1to8}, %zmm16, %zmm3{%k1}          # c45
        vmulpd    (%rdx,%r8){1to8}, %zmm15, %zmm4{%k1}          # c49
        vmulpd    (%rsi,%r15,8){1to8}, %zmm16, %zmm0{%k1}       # c53
        vmulpd    -8(%rdx,%r8){1to8}, %zmm14, %zmm24{%k1}       # c57
        vmulpd    -16(%rdx,%r8){1to8}, %zmm11, %zmm25{%k1}      # c61
        vaddpd    %zmm23, %zmm30, %zmm30{%k1}                   # c65
        vaddpd    %zmm20, %zmm19, %zmm23{%k1}                   # c69
        vaddpd    %zmm22, %zmm21, %zmm20{%k1}                   # c73
        vaddpd    %zmm31, %zmm26, %zmm21{%k1}                   # c77
        vaddpd    %zmm27, %zmm29, %zmm22{%k1}                   # c81
        vmulpd    (%rdx,%r8){1to8}, %zmm17, %zmm2{%k1}          # c85
        vaddpd    %zmm24, %zmm25, %zmm25{%k1}                   # c89
        vaddpd    %zmm0, %zmm23, %zmm26{%k1}                    # c93
        vaddpd    %zmm3, %zmm21, %zmm3{%k1}                     # c97
        vaddpd    %zmm4, %zmm22, %zmm0{%k1}                     # c101
        vmulpd    (%rsi,%r15,8){1to8}, %zmm17, %zmm28{%k1}      # c105
        vmulpd    (%rsi,%r15,8){1to8}, %zmm15, %zmm1{%k1}       # c109
        movq      -1040(%rbp), %r15                             # c109
        vaddpd    %zmm2, %zmm25, %zmm2{%k1}                     # c113
        vmulpd    64(%rcx,%rdi){1to8}, %zmm3, %zmm27{%k1}       # c117
        vmulpd    40(%rcx,%rdi){1to8}, %zmm3, %zmm23{%k1}       # c121
        vmulpd    56(%rcx,%rdi){1to8}, %zmm0, %zmm22{%k1}       # c125
        vmulpd    32(%rcx,%rdi){1to8}, %zmm0, %zmm25{%k1}       # c129
        vbroadcastsd -24(%rdx,%r8), %zmm8{%k1}                  # c133
        movq      -1032(%rbp), %r8                              # c133
        vaddpd    %zmm28, %zmm30, %zmm28{%k1}                   # c137
        vaddpd    %zmm1, %zmm20, %zmm19{%k1}                    # c141
        vmulpd    72(%rcx,%rdi){1to8}, %zmm2, %zmm24{%k1}       # c145
        vmulpd    48(%rcx,%rdi){1to8}, %zmm2, %zmm30{%k1}       # c149
        vmulpd    112(%rcx,%rdi){1to8}, %zmm3, %zmm4{%k1}       # c153
        vmulpd    88(%rcx,%rdi){1to8}, %zmm3, %zmm21{%k1}       # c157
        vmulpd    104(%rcx,%rdi){1to8}, %zmm0, %zmm1{%k1}       # c161
        vmulpd    80(%rcx,%rdi){1to8}, %zmm0, %zmm20{%k1}       # c165
        vaddpd    %zmm27, %zmm22, %zmm27{%k1}                   # c169
        vaddpd    %zmm23, %zmm25, %zmm23{%k1}                   # c173
        vmulpd    152(%rcx,%rdi){1to8}, %zmm8, %zmm5{%k1}       # c177
        vmulpd    144(%rcx,%rdi){1to8}, %zmm8, %zmm6{%k1}       # c181
        vmulpd    136(%rcx,%rdi){1to8}, %zmm8, %zmm7{%k1}       # c185
        vmulpd    128(%rcx,%rdi){1to8}, %zmm8, %zmm8{%k1}       # c189
        vmulpd    120(%rcx,%rdi){1to8}, %zmm2, %zmm29{%k1}      # c193
        vmulpd    96(%rcx,%rdi){1to8}, %zmm2, %zmm31{%k1}       # c197
        vaddpd    %zmm4, %zmm1, %zmm3{%k1}                      # c201
        vaddpd    %zmm21, %zmm20, %zmm2{%k1}                    # c205
        vaddpd    %zmm24, %zmm27, %zmm24{%k1}                   # c209
        vaddpd    %zmm30, %zmm23, %zmm30{%k1}                   # c213
        vaddpd    %zmm29, %zmm3, %zmm29{%k1}                    # c217
        vaddpd    %zmm31, %zmm2, %zmm31{%k1}                    # c221
        vaddpd    %zmm7, %zmm24, %zmm2{%k1}                     # c225
        vaddpd    %zmm8, %zmm30, %zmm3{%k1}                     # c229
        vaddpd    %zmm6, %zmm31, %zmm0{%k1}                     # c233
        vmulpd    %zmm26, %zmm2, %zmm7{%k1}                     # c237
        vmulpd    %zmm19, %zmm3, %zmm8{%k1}                     # c241
        vaddpd    %zmm5, %zmm29, %zmm1{%k1}                     # c245
        vmulpd    %zmm28, %zmm0, %zmm5{%k1}                     # c249
        vaddpd    %zmm7, %zmm8, %zmm6{%k1}                      # c253
        vmulpd    -1136(%rbp){1to8}, %zmm1, %zmm4{%k1}          # c257
        vaddpd    %zmm5, %zmm6, %zmm19{%k1}                     # c261
        vaddpd    %zmm4, %zmm19, %zmm26{%k1}                    # c265
        vmulpd    (%rsi,%r11,8){1to8}, %zmm13, %zmm19{%k1}      # c269
        vmulpd    (%rsi,%r11,8){1to8}, %zmm12, %zmm21{%k1}      # c273
        vmulpd    (%rsi,%r15,8){1to8}, %zmm10, %zmm5{%k1}       # c277
        vmulpd    (%rsi,%r15,8){1to8}, %zmm9, %zmm20{%k1}       # c281
        vmulpd    %zmm18, %zmm26, %zmm28{%k1}                   # c285
        vmulpd    (%rsi,%r8,8){1to8}, %zmm16, %zmm25{%k1}       # c289
        vmulpd    (%rsi,%r8,8){1to8}, %zmm15, %zmm27{%k1}       # c293
        vmulpd    (%rsi,%r11,8){1to8}, %zmm14, %zmm6{%k1}       # c297
        vmulpd    (%rsi,%r15,8){1to8}, %zmm11, %zmm7{%k1}       # c301
        movq      -1064(%rbp), %r15                             # c301
        vaddpd    %zmm19, %zmm5, %zmm24{%k1}                    # c305
        vaddpd    %zmm21, %zmm20, %zmm26{%k1}                   # c309
        vaddpd    -40(%r12,%r13){1to8}, %zmm28, %zmm8{%k1}      # c313
        vmulpd    (%rsi,%r8,8){1to8}, %zmm17, %zmm23{%k1}       # c317
        movq      -1096(%rbp), %r8                              # c317
        vaddpd    %zmm6, %zmm7, %zmm22{%k1}                     # c321
        vpackstorelpd %zmm8, -40(%r12,%r13){%k1}                # c321
        vaddpd    %zmm25, %zmm24, %zmm29{%k1}                   # c325
        vaddpd    %zmm27, %zmm26, %zmm30{%k1}                   # c329
        vaddpd    %zmm23, %zmm22, %zmm28{%k1}                   # c333
        vmulpd    %zmm29, %zmm2, %zmm8{%k1}                     # c337
        vmulpd    %zmm30, %zmm3, %zmm31{%k1}                    # c341
        vmulpd    %zmm28, %zmm0, %zmm6{%k1}                     # c345
        vaddpd    %zmm8, %zmm31, %zmm7{%k1}                     # c349
        vmulpd    (%rsi,%r8,8){1to8}, %zmm1, %zmm4{%k1}         # c353
        movq      -1048(%rbp), %r8                              # c353
        vaddpd    %zmm6, %zmm7, %zmm5{%k1}                      # c357
        vmulpd    (%rsi,%r15,8){1to8}, %zmm17, %zmm24{%k1}      # c361
        vmulpd    (%rsi,%r15,8){1to8}, %zmm16, %zmm26{%k1}      # c365
        vmulpd    (%rsi,%r15,8){1to8}, %zmm15, %zmm28{%k1}      # c369
        movq      -1024(%rbp), %r15                             # c369
        vaddpd    %zmm4, %zmm5, %zmm4{%k1}                      # c373
        vmulpd    (%rsi,%r8,8){1to8}, %zmm13, %zmm20{%k1}       # c377
        vmulpd    (%rsi,%r8,8){1to8}, %zmm12, %zmm22{%k1}       # c381
        vmulpd    (%rsi,%r15,8){1to8}, %zmm10, %zmm19{%k1}      # c385
        vmulpd    (%rsi,%r15,8){1to8}, %zmm9, %zmm21{%k1}       # c389
        vmulpd    %zmm18, %zmm4, %zmm8{%k1}                     # c393
        vmulpd    (%rsi,%r8,8){1to8}, %zmm14, %zmm4{%k1}        # c397
        movq      -1072(%rbp), %r8                              # c397
        vmulpd    (%rsi,%r15,8){1to8}, %zmm11, %zmm6{%k1}       # c401
        movq      -1016(%rbp), %r15                             # c401
        vaddpd    %zmm20, %zmm19, %zmm25{%k1}                   # c405
        vaddpd    %zmm22, %zmm21, %zmm27{%k1}                   # c409
        vaddpd    -32(%r12,%r13){1to8}, %zmm8, %zmm7{%k1}       # c413
        vaddpd    %zmm4, %zmm6, %zmm23{%k1}                     # c417
        vaddpd    %zmm26, %zmm25, %zmm30{%k1}                   # c421
        vpackstorelpd %zmm7, -32(%r12,%r13){%k1}                # c421
        vaddpd    %zmm28, %zmm27, %zmm31{%k1}                   # c425
        vaddpd    %zmm24, %zmm23, %zmm29{%k1}                   # c429
        vmulpd    %zmm30, %zmm2, %zmm7{%k1}                     # c433
        vmulpd    %zmm31, %zmm3, %zmm8{%k1}                     # c437
        vmulpd    %zmm29, %zmm0, %zmm4{%k1}                     # c441
        vaddpd    %zmm7, %zmm8, %zmm6{%k1}                      # c445
        vmulpd    64(%rsi,%r10,8){1to8}, %zmm1, %zmm5{%k1}      # c449
        vaddpd    %zmm4, %zmm6, %zmm19{%k1}                     # c453
        vaddpd    %zmm5, %zmm19, %zmm5{%k1}                     # c457
        vmulpd    (%rsi,%r9,8){1to8}, %zmm13, %zmm20{%k1}       # c461
        vmulpd    (%rsi,%r9,8){1to8}, %zmm12, %zmm22{%k1}       # c465
        vmulpd    (%rsi,%r15,8){1to8}, %zmm10, %zmm19{%k1}      # c469
        vmulpd    (%rsi,%r15,8){1to8}, %zmm9, %zmm21{%k1}       # c473
        vmulpd    %zmm18, %zmm5, %zmm8{%k1}                     # c477
        vmulpd    (%rsi,%r8,8){1to8}, %zmm16, %zmm26{%k1}       # c481
        vmulpd    (%rsi,%r8,8){1to8}, %zmm15, %zmm28{%k1}       # c485
        vmulpd    (%rsi,%r9,8){1to8}, %zmm14, %zmm5{%k1}        # c489
        vmulpd    (%rsi,%r15,8){1to8}, %zmm11, %zmm4{%k1}       # c493
        movq      -1120(%rbp), %r15                             # c493
        vaddpd    %zmm20, %zmm19, %zmm25{%k1}                   # c497
        vaddpd    %zmm22, %zmm21, %zmm27{%k1}                   # c501
        vaddpd    -24(%r12,%r13){1to8}, %zmm8, %zmm7{%k1}       # c505
        vmulpd    (%rsi,%r8,8){1to8}, %zmm17, %zmm24{%k1}       # c509
        movq      -1056(%rbp), %r8                              # c509
        vaddpd    %zmm5, %zmm4, %zmm23{%k1}                     # c513
        vpackstorelpd %zmm7, -24(%r12,%r13){%k1}                # c513
        vaddpd    %zmm26, %zmm25, %zmm30{%k1}                   # c517
        vaddpd    %zmm28, %zmm27, %zmm31{%k1}                   # c521
        vaddpd    %zmm24, %zmm23, %zmm29{%k1}                   # c525
        vmulpd    %zmm30, %zmm2, %zmm7{%k1}                     # c529
        vmulpd    %zmm31, %zmm3, %zmm8{%k1}                     # c533
        vmulpd    %zmm29, %zmm0, %zmm4{%k1}                     # c537
        vaddpd    %zmm7, %zmm8, %zmm30{%k1}                     # c541
        vmulpd    96(%rsi,%r10,8){1to8}, %zmm1, %zmm6{%k1}      # c545
        vaddpd    %zmm4, %zmm30, %zmm5{%k1}                     # c549
        vaddpd    %zmm6, %zmm5, %zmm6{%k1}                      # c553
        vmulpd    (%rsi,%r8,8){1to8}, %zmm17, %zmm23{%k1}       # c557
        vmulpd    (%rsi,%r8,8){1to8}, %zmm16, %zmm25{%k1}       # c561
        vmulpd    (%rsi,%r8,8){1to8}, %zmm15, %zmm27{%k1}       # c565
        movq      -1008(%rbp), %r8                              # c565
        vmulpd    %zmm18, %zmm6, %zmm8{%k1}                     # c569
        vmulpd    (%rsi,%r15,8){1to8}, %zmm13, %zmm19{%k1}      # c573
        vmulpd    (%rsi,%r15,8){1to8}, %zmm12, %zmm21{%k1}      # c577
        vmulpd    (%rsi,%r8,8){1to8}, %zmm10, %zmm6{%k1}        # c581
        vmulpd    (%rsi,%r8,8){1to8}, %zmm9, %zmm20{%k1}        # c585
        vmulpd    (%rsi,%r15,8){1to8}, %zmm14, %zmm5{%k1}       # c589
        movq      -1104(%rbp), %r15                             # c589
        vmulpd    (%rsi,%r8,8){1to8}, %zmm11, %zmm4{%k1}        # c593
        movq      -1128(%rbp), %r8                              # c593
        vaddpd    %zmm19, %zmm6, %zmm24{%k1}                    # c597
        vaddpd    %zmm21, %zmm20, %zmm26{%k1}                   # c601
        vaddpd    %zmm5, %zmm4, %zmm22{%k1}                     # c605
        vaddpd    %zmm25, %zmm24, %zmm29{%k1}                   # c609
        vaddpd    %zmm27, %zmm26, %zmm30{%k1}                   # c613
        vaddpd    %zmm23, %zmm22, %zmm28{%k1}                   # c617
        vaddpd    -16(%r12,%r13){1to8}, %zmm8, %zmm7{%k1}       # c621
        vmulpd    %zmm29, %zmm2, %zmm8{%k1}                     # c625
        vmulpd    %zmm30, %zmm3, %zmm31{%k1}                    # c629
        vpackstorelpd %zmm7, -16(%r12,%r13){%k1}                # c629
        vmulpd    %zmm28, %zmm0, %zmm4{%k1}                     # c633
        vmulpd    (%rsi,%r15,8){1to8}, %zmm17, %zmm26{%k1}      # c637
        vmulpd    (%rsi,%r15,8){1to8}, %zmm16, %zmm27{%k1}      # c641
        vmulpd    (%rsi,%r15,8){1to8}, %zmm15, %zmm28{%k1}      # c645
        movq      -976(%rbp), %r15                              # c645
        vaddpd    %zmm8, %zmm31, %zmm29{%k1}                    # c649
        vmulpd    (%rsi,%r8,8){1to8}, %zmm14, %zmm21{%k1}       # c653
        vmulpd    (%rsi,%r8,8){1to8}, %zmm13, %zmm23{%k1}       # c657
        vmulpd    (%rsi,%r8,8){1to8}, %zmm12, %zmm25{%k1}       # c661
        vmulpd    (%rsi,%r15,8){1to8}, %zmm11, %zmm20{%k1}      # c665
        vmulpd    (%rsi,%r15,8){1to8}, %zmm10, %zmm22{%k1}      # c669
        vmulpd    (%rsi,%r15,8){1to8}, %zmm9, %zmm24{%k1}       # c673
        vmulpd    128(%rsi,%r10,8){1to8}, %zmm1, %zmm7{%k1}     # c677
        vaddpd    %zmm4, %zmm29, %zmm5{%k1}                     # c681
        vmulpd    160(%rsi,%r10,8){1to8}, %zmm1, %zmm8{%k1}     # c685
        vaddpd    %zmm21, %zmm20, %zmm1{%k1}                    # c689
        vaddpd    %zmm23, %zmm22, %zmm20{%k1}                   # c693
        vaddpd    %zmm25, %zmm24, %zmm21{%k1}                   # c697
        vaddpd    %zmm7, %zmm5, %zmm6{%k1}                      # c701
        vaddpd    %zmm27, %zmm20, %zmm4{%k1}                    # c705
        vaddpd    %zmm28, %zmm21, %zmm5{%k1}                    # c709
        vaddpd    %zmm26, %zmm1, %zmm1{%k1}                     # c713
        vmulpd    %zmm4, %zmm2, %zmm2{%k1}                      # c717
        vmulpd    %zmm5, %zmm3, %zmm3{%k1}                      # c721
        vmulpd    %zmm1, %zmm0, %zmm1{%k1}                      # c725
        vaddpd    %zmm2, %zmm3, %zmm0{%k1}                      # c729
        vaddpd    %zmm1, %zmm0, %zmm0{%k1}                      # c733
        vaddpd    %zmm8, %zmm0, %zmm2{%k1}                      # c737
        vmulpd    %zmm18, %zmm6, %zmm7{%k1}                     # c741
        vmulpd    %zmm18, %zmm2, %zmm3{%k1}                     # c745
        vaddpd    -8(%r12,%r13){1to8}, %zmm7, %zmm19{%k1}       # c749
        vaddpd    (%r12,%r13){1to8}, %zmm3, %zmm4{%k1}          # c753
        movb      %al, %al                                      # c753
        vpackstorelpd %zmm19, -8(%r12,%r13){%k1}                # c757
        vpackstorelpd %zmm4, (%r12,%r13){%k1}                   # c761
        jne       ..B1.11       # Prob 0%                       # c761
                                # LOE rax rdx rcx rbx rsi rdi r9 r10 r11 r13 r14 zmm9 zmm10 zmm11 zmm12 zmm13 zmm14 zmm15 zmm16 zmm
17 zmm18 k1
..B1.12:                        # Preds ..B1.11 Latency 5
        vbroadcasti64x4 _const_3(%rip), %zmm30                  # c1
                                # LOE rdx rbx zmm30 k1
..CL9:
..B1.13:                        # Preds ..B1.12 Latency 13
        movq      -992(%rbp), %rax                              # c1
        addq      $192, %rdx                                    # c1
        incq      %rax                                          # c5
        movq      %rax, -992(%rbp)                              # c9
        cmpl      $6, %eax                                      # c9
        jne       ..B1.14       # Prob 100%                     # c13
                                # LOE rdx rbx zmm30 k1
..CL10:
..B1.15:                        # Preds ..B1.13                 # Infreq Latency 603
        movq      -1168(%rbp), %rax                             # c1
        movq      -392(%rbp), %r12                              # c1
        movl      24(%rax,%r12), %r14d                          # c5
        movl      (%rax,%r12), %r13d                            # c5
        imull     $42, %r14d, %r14d                             # c9
        lea       416(%r14,%r13), %edi                          # c19
        movq      160(%rax,%r12), %r11                          # c19
        movl      %edi, -848(%rbp)                              # c23
        lea       400(%r14,%r13), %edi                          # c23
        movl      %edi, -840(%rbp)                              # c27
        lea       384(%r14,%r13), %edi                          # c27
        movl      %edi, -832(%rbp)                              # c31
        lea       368(%r14,%r13), %edi                          # c31
        movl      %edi, -824(%rbp)                              # c35
        lea       352(%r14,%r13), %edi                          # c35
        movl      %edi, -816(%rbp)                              # c39
        lea       336(%r14,%r13), %edi                          # c39
        movl      %edi, -808(%rbp)                              # c43
        lea       320(%r14,%r13), %edi                          # c43
        movl      %edi, -800(%rbp)                              # c47
        lea       304(%r14,%r13), %edi                          # c47
        movl      %edi, -792(%rbp)                              # c51
        lea       288(%r14,%r13), %edi                          # c51
        movl      %edi, -784(%rbp)                              # c55
        lea       272(%r14,%r13), %edi                          # c55
        movl      %edi, -776(%rbp)                              # c59
        lea       256(%r14,%r13), %edi                          # c59
        movl      %edi, -768(%rbp)                              # c63
        lea       240(%r14,%r13), %edi                          # c63
        movl      %edi, -760(%rbp)                              # c67
        lea       224(%r14,%r13), %edi                          # c67
        movl      %edi, -752(%rbp)                              # c71
        lea       208(%r14,%r13), %edi                          # c71
        movl      %edi, -744(%rbp)                              # c75
        lea       192(%r14,%r13), %edi                          # c75
        movl      %edi, -736(%rbp)                              # c79
        lea       176(%r14,%r13), %edi                          # c79
        movl      %edi, -728(%rbp)                              # c83
        lea       160(%r14,%r13), %edi                          # c83
        movl      %edi, -720(%rbp)                              # c87
        lea       144(%r14,%r13), %edi                          # c87
        movl      %edi, -712(%rbp)                              # c91
        lea       128(%r14,%r13), %edi                          # c91
        movl      %edi, -704(%rbp)                              # c95
        lea       112(%r14,%r13), %edi                          # c95
        movl      %edi, -696(%rbp)                              # c99
        lea       96(%r14,%r13), %edi                           # c99
        movl      %edi, -688(%rbp)                              # c103
        lea       80(%r14,%r13), %edi                           # c103
        movl      %edi, -680(%rbp)                              # c107
        lea       64(%r14,%r13), %edi                           # c107
        movq      (%r11), %r10                                  # c111
        movl      %r14d, %r15d                                  # c111
        addl      (%rax,%r12), %r15d                            # c115
        movl      %edi, -672(%rbp)                              # c115
        lea       48(%r14,%r13), %edi                           # c119
        movq      %r10, -856(%rbp)                              # c119
        lea       560(%r14,%r13), %r12d                         # c123
        lea       544(%r14,%r13), %r11d                         # c123
        lea       528(%r14,%r13), %r10d                         # c127
        lea       512(%r14,%r13), %r9d                          # c127
        lea       496(%r14,%r13), %r8d                          # c131
        lea       480(%r14,%r13), %eax                          # c131
        lea       464(%r14,%r13), %edx                          # c135
        lea       448(%r14,%r13), %ecx                          # c135
        lea       432(%r14,%r13), %esi                          # c139
        movl      %edi, -664(%rbp)                              # c139
        lea       32(%r14,%r13), %edi                           # c143
        lea       16(%r14,%r13), %r13d                          # c143
        movl      -848(%rbp), %r14d                             # c147
        movslq    %eax, %rax                                    # c147
        movslq    %r14d, %r14                                   # c151
        movslq    %r12d, %r12                                   # c151
        movq      %r14, -584(%rbp)                              # c155
        movslq    %r15d, %r15                                   # c155
        movl      -792(%rbp), %r14d                             # c159
        movq      %rax, -616(%rbp)                              # c159
        movslq    %r14d, %r14                                   # c163
        movq      %r12, -656(%rbp)                              # c163
        movq      %r14, -528(%rbp)                              # c167
        movslq    %r13d, %r13                                   # c167
        movl      -840(%rbp), %eax                              # c171
        movslq    %esi, %rsi                                    # c171
        movl      -720(%rbp), %r14d                             # c175
        movslq    %eax, %rax                                    # c175
        movl      -832(%rbp), %r12d                             # c179
        movslq    %r14d, %r14                                   # c179
        movq      %rax, -576(%rbp)                              # c183
        movslq    %r12d, %r12                                   # c183
        movl      -784(%rbp), %eax                              # c187
        movq      %r14, -456(%rbp)                              # c187
        movq      %r12, -568(%rbp)                              # c191
        movslq    %eax, %rax                                    # c191
        movq      -376(%rbp), %r14                              # c195
        movq      %rax, -520(%rbp)                              # c195
        movslq    %edi, %rdi                                    # c199
        movq      %rsi, -592(%rbp)                              # c199
        movslq    %ecx, %rcx                                    # c203
        movslq    %edx, %rdx                                    # c203
        movl      -776(%rbp), %r12d                             # c207
        movq      %rcx, -600(%rbp)                              # c207
        movslq    %r12d, %r12                                   # c211
        movq      %rdx, -608(%rbp)                              # c211
        movq      %r12, -512(%rbp)                              # c215
        movslq    %r8d, %r8                                     # c215
        movq      -856(%rbp), %rax                              # c219
        movq      %r8, -624(%rbp)                               # c219
        movq      %rax, (%r14,%r15,8)                           # c223
        movslq    %r9d, %r9                                     # c223
        movq      -1168(%rbp), %rax                             # c227
        movq      %r9, -632(%rbp)                               # c227
        movslq    %r10d, %r10                                   # c231
        movslq    %r11d, %r11                                   # c231
        movq      -392(%rbp), %r12                              # c235
        movq      %r10, -640(%rbp)                              # c235
        movq      %r11, -648(%rbp)                              # c239
        movq      168(%rax,%r12), %r15                          # c239
        movq      (%r15), %r15                                  # c243
        movl      -728(%rbp), %esi                              # c243
        movq      %r15, (%r14,%r13,8)                           # c247
        movslq    %esi, %rsi                                    # c247
        movq      176(%rax,%r12), %r13                          # c251
        movq      %rsi, -464(%rbp)                              # c251
        movq      (%r13), %r13                                  # c255
        movl      -664(%rbp), %esi                              # c255
        movq      %r13, (%r14,%rdi,8)                           # c259
        movslq    %esi, %rsi                                    # c259
        movq      184(%rax,%r12), %rdi                          # c263
        movq      264(%rax,%r12), %r13                          # c263
        movl      -736(%rbp), %ecx                              # c267
        movq      (%rdi), %r15                                  # c267
        movslq    %ecx, %rcx                                    # c271
        movq      %r15, (%r14,%rsi,8)                           # c271
        movq      %rcx, -472(%rbp)                              # c275
        movl      -672(%rbp), %ecx                              # c275
        movq      192(%rax,%r12), %rsi                          # c279
        movslq    %ecx, %rcx                                    # c279
        movl      -744(%rbp), %edx                              # c283
        movq      (%rsi), %rsi                                  # c283
        movslq    %edx, %rdx                                    # c287
        movq      %rsi, (%r14,%rcx,8)                           # c287
        movq      %rdx, -480(%rbp)                              # c291
        movl      -680(%rbp), %edx                              # c291
        movq      200(%rax,%r12), %rcx                          # c295
        movslq    %edx, %rdx                                    # c295
        movl      -800(%rbp), %r8d                              # c299
        movq      (%rcx), %rcx                                  # c299
        movslq    %r8d, %r8                                     # c303
        movl      -808(%rbp), %r9d                              # c303
        movq      %r8, -536(%rbp)                               # c307
        movslq    %r9d, %r9                                     # c307
        movl      -688(%rbp), %r8d                              # c311
        movq      %rcx, (%r14,%rdx,8)                           # c311
        movslq    %r8d, %r8                                     # c315
        movq      %r9, -544(%rbp)                               # c315
        movq      208(%rax,%r12), %rdx                          # c319
        movq      248(%rax,%r12), %rsi                          # c319
        movl      -752(%rbp), %r9d                              # c323
        movq      (%rdx), %rdx                                  # c323
        movslq    %r9d, %r9                                     # c327
        movl      -816(%rbp), %r10d                             # c327
        movq      %r9, -488(%rbp)                               # c331
        movslq    %r10d, %r10                                   # c331
        movl      -696(%rbp), %r9d                              # c335
        movq      %rdx, (%r14,%r8,8)                            # c335
        movslq    %r9d, %r9                                     # c339
        movq      %r10, -552(%rbp)                              # c339
        movq      216(%rax,%r12), %r8                           # c343
        movl      -760(%rbp), %r10d                             # c343
        movslq    %r10d, %r10                                   # c347
        movq      (%r8), %rdx                                   # c347
        movl      -824(%rbp), %r11d                             # c351
        movq      %r10, -496(%rbp)                              # c351
        movq      %rdx, (%r14,%r9,8)                            # c355
        movslq    %r11d, %r11                                   # c355
        movl      -704(%rbp), %r10d                             # c359
        movq      %r11, -560(%rbp)                              # c359
        movslq    %r10d, %r10                                   # c363
        movq      224(%rax,%r12), %r9                           # c363
        movl      -768(%rbp), %r11d                             # c367
        movq      (%r9), %rdx                                   # c367
        movslq    %r11d, %r11                                   # c371
        movq      %rdx, (%r14,%r10,8)                           # c371
        movq      %r11, -504(%rbp)                              # c375
        movl      -712(%rbp), %r11d                             # c375
        movq      232(%rax,%r12), %r10                          # c379
        movslq    %r11d, %r11                                   # c379
        movq      (%r10), %rdx                                  # c383
        movq      -456(%rbp), %rcx                              # c383
        movq      %rdx, (%r14,%r11,8)                           # c387
        movq      240(%rax,%r12), %r11                          # c387
        movq      -464(%rbp), %r8                               # c391
        movq      256(%rax,%r12), %r9                           # c391
        movq      (%r11), %rdx                                  # c395
        movq      -472(%rbp), %r11                              # c395
        movq      %rdx, (%r14,%rcx,8)                           # c399
        movq      (%rsi), %rdi                                  # c399
        movq      -480(%rbp), %rdx                              # c403
        movq      %rdi, (%r14,%r8,8)                            # c403
        movq      272(%rax,%r12), %rcx                          # c407
        movq      280(%rax,%r12), %r8                           # c407
        movq      (%r9), %r10                                   # c411
        movq      -488(%rbp), %rdi                              # c411
        movq      (%r13), %r15                                  # c415
        movq      %r10, (%r14,%r11,8)                           # c415
        movq      %r15, (%r14,%rdx,8)                           # c419
        movq      -496(%rbp), %r10                              # c419
        movq      288(%rax,%r12), %r11                          # c423
        movq      296(%rax,%r12), %rdx                          # c423
        movq      (%rcx), %rsi                                  # c427
        movq      -504(%rbp), %r15                              # c427
        movq      %rsi, (%r14,%rdi,8)                           # c431
        movq      (%r8), %r9                                    # c431
        movq      -512(%rbp), %rsi                              # c435
        movq      %r9, (%r14,%r10,8)                            # c435
        movq      304(%rax,%r12), %rdi                          # c439
        movq      312(%rax,%r12), %r10                          # c439
        movq      (%r11), %r13                                  # c443
        movq      -520(%rbp), %r9                               # c443
        movq      (%rdx), %rcx                                  # c447
        movq      %r13, (%r14,%r15,8)                           # c447
        movq      %rcx, (%r14,%rsi,8)                           # c451
        movq      -528(%rbp), %r13                              # c451
        movq      320(%rax,%r12), %r15                          # c455
        movq      328(%rax,%r12), %rsi                          # c455
        movq      (%rdi), %r8                                   # c459
        movq      -536(%rbp), %rcx                              # c459
        movq      %r8, (%r14,%r9,8)                             # c463
        movq      (%r10), %r11                                  # c463
        movq      -544(%rbp), %r8                               # c467
        movq      %r11, (%r14,%r13,8)                           # c467
        movq      336(%rax,%r12), %r9                           # c471
        movq      344(%rax,%r12), %r13                          # c471
        movq      (%r15), %rdx                                  # c475
        movq      -552(%rbp), %r11                              # c475
        movq      (%rsi), %rdi                                  # c479
        movq      %rdx, (%r14,%rcx,8)                           # c479
        movq      %rdi, (%r14,%r8,8)                            # c483
        movq      -560(%rbp), %rdx                              # c483
        movq      352(%rax,%r12), %rcx                          # c487
        movq      360(%rax,%r12), %r8                           # c487
        movq      (%r9), %r10                                   # c491
        movq      -568(%rbp), %rdi                              # c491
        movq      %r10, (%r14,%r11,8)                           # c495
        movq      (%r13), %r15                                  # c495
        movq      -576(%rbp), %r10                              # c499
        movq      %r15, (%r14,%rdx,8)                           # c499
        movq      368(%rax,%r12), %r11                          # c503
        movq      376(%rax,%r12), %rdx                          # c503
        movq      (%rcx), %rsi                                  # c507
        movq      -584(%rbp), %r15                              # c507
        movq      (%r8), %r9                                    # c511
        movq      %rsi, (%r14,%rdi,8)                           # c511
        movq      %r9, (%r14,%r10,8)                            # c515
        movq      -592(%rbp), %rsi                              # c515
        movq      (%r11), %r13                                  # c519
        movq      392(%rax,%r12), %r10                          # c519
        movq      384(%rax,%r12), %rdi                          # c523
        movq      %r13, (%r14,%r15,8)                           # c523
        movq      (%rdx), %rcx                                  # c527
        movq      -608(%rbp), %r13                              # c527
        movq      8(%rax,%r12), %r15                            # c531
        movq      %rcx, (%r14,%rsi,8)                           # c531
        movq      -600(%rbp), %r9                               # c535
        movq      -616(%rbp), %rcx                              # c535
        movq      (%r10), %r11                                  # c539
        movq      (%rdi), %r8                                   # c539
        movq      400(%rax,%r12), %rsi                          # c543
        movq      %r11, (%r14,%r13,8)                           # c543
        movq      %r8, (%r14,%r9,8)                             # c547
        movq      (%r15), %rdx                                  # c547
        movq      416(%rax,%r12), %r13                          # c551
        movq      408(%rax,%r12), %r9                           # c551
        movq      -624(%rbp), %r8                               # c555
        movq      %rdx, (%r14,%rcx,8)                           # c555
        movq      -640(%rbp), %rdx                              # c559
        movq      -632(%rbp), %r11                              # c559
        movq      (%rsi), %rdi                                  # c563
        movq      (%r13), %r15                                  # c563
        movq      %rdi, (%r14,%r8,8)                            # c567
        movq      (%r9), %r10                                   # c567
        movq      424(%rax,%r12), %rcx                          # c571
        movq      432(%rax,%r12), %r8                           # c571
        movq      %r15, (%r14,%rdx,8)                           # c575
        movq      16(%rax,%r12), %rdx                           # c575
        movq      %r10, (%r14,%r11,8)                           # c579
        incq      %rdx                                          # c579
        movq      -648(%rbp), %rdi                              # c583
        movq      %rdx, -312(%rbp)                              # c583
        movq      -656(%rbp), %r10                              # c587
        movq      (%rcx), %rsi                                  # c587
        movq      (%r8), %r9                                    # c591
        movq      %rsi, (%r14,%rdi,8)                           # c591
        movq      %r9, (%r14,%r10,8)                            # c595
        movl      -240(%rbp), %r11d                             # c595
        movl      %r11d, -1256(%rbp)                            # c599
        cmpl      -448(%rbp), %edx                              # c599
        je        ..B1.18       # Prob 0%                       # c603
                                # LOE rax rbx r11 r12 r14 r11d r11b
..B1.16:                        # Preds ..B1.15                 # Infreq Latency 1
..CL11:
..B1.17:                        # Preds ..B1.16                 # Infreq Latency 13
        movq      8(%rax,%r12), %rdx                            # c1
        movq      %rax, %r15                                    # c1
        movl      -240(%rbp), %esi                              # c5
        movq      %rdx, -304(%rbp)                              # c5
        movl      %esi, -1256(%rbp)                             # c9
        movq      -232(%rbp), %rcx                              # c9
        jmp       ..B1.5        # Prob 0%                       # c13
                                # LOE rcx rbx r12 r14 r15
..B1.18:                        # Preds ..B1.15                 # Infreq Latency 21
        movq      %r14, -376(%rbp)                              # c1
        movl      %r11d, %r13d                                  # c1
        movq      %r12, -392(%rbp)                              # c5
        movq      -232(%rbp), %rsi                              # c5
        movq      -288(%rbp), %rcx                              # c9
        movq      -280(%rbp), %r10                              # c9
        movq      -272(%rbp), %r14                              # c13
        movq      -264(%rbp), %r12                              # c13
        movq      -256(%rbp), %r11                              # c17
        movq      -248(%rbp), %r8                               # c17
        movq      -384(%rbp), %r15                              # c21
                                # LOE rax rcx rbx rsi r8 r10 r11 r12 r13 r14 r15 r13d r13b
..CL2:
..B1.19:                        # Preds ..B1.18 ..B1.26         # Infreq Latency 1
        cmpq      %r15, %rsi                                    # c1
        jae       ..B1.25       # Prob 0%                       # c1
                                # LOE rax rcx rbx rsi r8 r10 r11 r12 r13 r14 r15 r13d r13b
..B1.20:                        # Preds ..B1.19                 # Infreq Latency 1
..CL13:
..B1.21:                        # Preds ..B1.20                 # Infreq Latency 13
        lea       1(%rsi), %rdi                                 # c1
        lea       736(%rax), %rdx                               # c1
        movl      %r13d, %r9d                                   # c5
        movq      %rdx, %rax                                    # c5
        movq      %rdi, %rsi                                    # c9
        testl     %r13d, %r13d                                  # c9
        je        ..B1.23       # Prob 0%                       # c13
                                # LOE rax rdx rcx rbx rsi rdi r8 r9 r10 r11 r12 r14 r15 r9d r13d r9b
..B1.22:                        # Preds ..B1.21                 # Infreq Latency 9
        movq      %rdx, -440(%rbp)                              # c1
        movq      -336(%rbp), %rdx                              # c1
        movq      %rdi, -344(%rbp)                              # c5
        movq      -328(%rbp), %r9                               # c5
        movq      -320(%rbp), %r13                              # c9
        jmp       ..B1.2        # Prob 0%                       # c9
                                # LOE rdx rcx rbx r8 r9 r10 r11 r12 r13 r14
..B1.23:                        # Preds ..B1.21                 # Infreq Latency 18
        movq      %rsi, -232(%rbp)                              # c1
        movq      %r14, -272(%rbp)                              # c1
        movl      %r9d, -240(%rbp)                              # c5
        movq      -376(%rbp), %r14                              # c5
        movq      %rcx, -288(%rbp)                              # c9
        movq      %r12, -264(%rbp)                              # c9
        movq      %r10, -280(%rbp)                              # c13
        movq      -392(%rbp), %r12                              # c13
        movq      %r11, -256(%rbp)                              # c17
        movq      %r8, -248(%rbp)                               # c17
        jmp       ..B1.9        # Prob 100%                     # c17
                                # LOE rax rbx r12 r14
..B1.24:                        # Preds ..B1.7                  # Infreq Latency 26
        movq      %r15, -440(%rbp)                              # c1
        movq      -280(%rbp), %r10                              # c1
        movq      %r14, -376(%rbp)                              # c5
        movq      -272(%rbp), %r14                              # c5
        movq      %r12, -392(%rbp)                              # c9
        movq      -264(%rbp), %r12                              # c9
        movq      %rcx, -344(%rbp)                              # c13
        movq      -288(%rbp), %rcx                              # c13
        movq      -256(%rbp), %r11                              # c17
        movq      -248(%rbp), %r8                               # c17
        movq      -336(%rbp), %rdx                              # c21
        movq      -328(%rbp), %r9                               # c21
        movq      -320(%rbp), %r13                              # c25
        jmp       ..B1.2        # Prob 100%                     # c25
                                # LOE rdx rcx rbx r8 r9 r10 r11 r12 r13 r14
..CL12:
..B1.25:                        # Preds ..B1.19                 # Infreq Latency 9
        movq      -1192(%rbp), %r12                             # c1
        movq      -1200(%rbp), %r13                             # c1
        movq      -1208(%rbp), %r14                             # c5
        movq      -1216(%rbp), %r15                             # c5
        movq      %rbp, %rsp                                    # c9
        popq      %rbp                                          #
        ret                                                     #
                                # LOE rbx
..B1.26:                        # Preds ..B1.2                  # Infreq Latency 10
        movq      %rdx, -336(%rbp)                              # c1
        movq      -384(%rbp), %r15                              # c1
        movq      %r9, -328(%rbp)                               # c5
        movq      %r13, -320(%rbp)                              # c5
        movl      -1256(%rbp), %r13d                            # c9
        jmp       ..B1.19       # Prob 100%                     # c9
                                # LOE rax rcx rbx rsi r8 r10 r11 r12 r13 r14 r15 r13d r13b
..B1.30:                        # Preds ..B1.10                 # Infreq Latency 25
        vgetmantpd $0, %zmm16, %zmm13                           # c1
        vcmpneqpd %zmm30{aaaa}, %zmm5, %k2{%k2}                 # c5
        vgetmantpd $0, %zmm5, %zmm12                            # c9
        vfnmadd231pd {rn-sae}, %zmm14, %zmm3, %zmm15{%k2}       # c13
        vcmpneqpd %zmm13, %zmm12, %k3{%k2}                      # c17
        vcmpneqpd %zmm30{aaaa}, %zmm15, %k0{%k2}                # c21
        movb      %al, %al                                      # c21
        jkzd      ..B1.31, %k3  # Prob 95%                      # c25
                                # LOE rax rdx rbx rsi r9 r10 r11 zmm0 zmm1 zmm2 zmm3 zmm4 zmm5 zmm6 zmm7 zmm8 zmm9 zmm10 zmm11 zmm1
5 zmm16 zmm22 zmm30 k0 k1 k2 k3
..B1.32:                        # Preds ..B1.30                 # Infreq Latency 58
        vbroadcasti64x4 _const_3(%rip), %zmm12                  # c1
        vpxorq    %zmm13, %zmm13, %zmm13                        # c5
        kand      %k0, %k3                                      # c5
        vpxorq    %zmm3, %zmm15, %zmm15{%k3}                    # c9
        vbroadcasti32x4 _const_3(%rip), %zmm3                   # c13
        vpandq    %zmm12{cccc}, %zmm15, %zmm15{%k3}             # c17
        vpsubd    %zmm3{dddd}, %zmm16, %zmm14                   # c21
        vpandq    %zmm12{dddd}, %zmm14, %zmm14{%k3}             # c25
        vporq     %zmm15, %zmm14, %zmm14{%k3}                   # c29
        vpsubrd   %zmm3{cccc}, %zmm2, %zmm15                    # c33
        vpandq    %zmm12{dddd}, %zmm15, %zmm15{%k3}             # c37
        vmulpd    {rn-sae}, %zmm15, %zmm5, %zmm13{%k1}          # c41
        vpxorq    %zmm17, %zmm17, %zmm17                        # c45
        vsubpd    {rn-sae}, %zmm13, %zmm16, %zmm17{%k1}         # c49
        vaddpd    {rn-sae}, %zmm14, %zmm17, %zmm17{%k1}         # c53
        vfmadd231pd %zmm17, %zmm2, %zmm5{%k3}                   # c57
        jmp       ..B1.29       # Prob 100%                     # c57
                                # LOE rax rdx rbx rsi r9 r10 r11 zmm0 zmm1 zmm4 zmm5 zmm6 zmm7 zmm8 zmm9 zmm10 zmm11 zmm22 zmm30 k1
..B1.31:                        # Preds ..B1.30                 # Infreq Latency 26
        vbroadcasti64x4 _const_3(%rip), %zmm3                   # c1
        kandn     %k0, %k3                                      # c5
        vmovdqa64 %zmm3{aaaa}, %zmm2                            # c9
        vpandq    %zmm3{dddd}, %zmm5, %zmm2{%k3}                # c13
        vcmpeqpd  %zmm30{aaaa}, %zmm2, %k2{%k2}                 # c17
        vmovdqa64 %zmm3{bbbb}, %zmm12                           # c21
        vfmadd231pd {rn}, %zmm12, %zmm12, %zmm5{%k2}            # c25
        jmp       ..B1.29       # Prob 100%                     # c25
        .align    16,0x90
                                # LOE rax rdx rbx rsi r9 r10 r11 zmm0 zmm1 zmm4 zmm5 zmm6 zmm7 zmm8 zmm9 zmm10 zmm11 zmm22 zmm30 k1
# mark_end;
	.type	tmr_ocl_num_int_el,@function
	.size	tmr_ocl_num_int_el,.-tmr_ocl_num_int_el
        .align 64
        .globl _const_3
_const_3:
        .long 0
        .long 0
        .long 1
        .long 0
        .long 2
        .long 0
        .long 3
        .long 0
        .long 4
        .long 0
        .long 5
        .long 0
        .long 6
        .long 0
        .long 7
        .long 0
        .align 64
        .globl _const_4
_const_4:
        .long 8
        .long 0
        .long 9
        .long 0
        .long 10
        .long 0
        .long 11
        .long 0
        .long 12
        .long 0
        .long 13
        .long 0
        .long 14
        .long 0
        .long 15
        .long 0
        .align 64
        .globl _const_5
_const_5:
        .long 0
        .long 0
        .long 0
        .long 0
        .long 0
        .long 0
        .long 0
        .long 0
        .long 0
        .long 2
        .long 4
        .long 6
        .long 8
        .long 10
        .long 12
        .long 14
        .align 64
        .globl _const_6
_const_6:
        .long 0
        .long 2
        .long 4
        .long 6
        .long 8
        .long 10
        .long 12
        .long 14
        .long 0
        .long 0
        .long 0
        .long 0
        .long 0
        .long 0
        .long 0
        .long 0
        .align 64
        .globl _const_7
_const_7:
        .long 0
        .long 0
        .long 0
        .long 0
        .long 0
        .long 0
        .long 0
        .long 0
        .long 0
        .long 0
        .long 0
        .long 0
        .long 0
        .long 0
        .long 0
        .long 0
        .align 64
        .globl _const_8
_const_8:
        .long 18
        .long 18
        .long 18
        .long 18
        .long 18
        .long 18
        .long 18
        .long 18
        .long 18
        .long 18
        .long 18
        .long 18
        .long 18
        .long 18
        .long 18
        .long 18
        .align 64
        .globl _const_9
_const_9:
        .long 3
        .long 0
        .long 3
        .long 0
        .long 3
        .long 0
        .long 3
        .long 0
        .long 3
        .long 0
        .long 3
        .long 0
        .long 3
        .long 0
        .long 3
        .long 0
        .align 64
        .globl _const_10
_const_10:
        .long 1
        .long 0
        .long 1
        .long 0
        .long 1
        .long 0
        .long 1
        .long 0
        .long 1
        .long 0
        .long 1
        .long 0
        .long 1
        .long 0
        .long 1
        .long 0
        .align 64
        .globl _const_11
_const_11:
        .long 2
        .long 0
        .long 2
        .long 0
        .long 2
        .long 0
        .long 2
        .long 0
        .long 2
        .long 0
        .long 2
        .long 0
        .long 2
        .long 0
        .long 2
        .long 0
        .align 64
        .globl _const_12
_const_12:
        .long 4
        .long 0
        .long 4
        .long 0
        .long 4
        .long 0
        .long 4
        .long 0
        .long 4
        .long 0
        .long 4
        .long 0
        .long 4
        .long 0
        .long 4
        .long 0
        .align 64
        .globl _const_13
_const_13:
        .long 5
        .long 0
        .long 5
        .long 0
        .long 5
        .long 0
        .long 5
        .long 0
        .long 5
        .long 0
        .long 5
        .long 0
        .long 5
        .long 0
        .long 5
        .long 0
        .align 64
        .globl _const_14
_const_14:
        .long 6
        .long 0
        .long 6
        .long 0
        .long 6
        .long 0
        .long 6
        .long 0
        .long 6
        .long 0
        .long 6
        .long 0
        .long 6
        .long 0
        .long 6
        .long 0
        .align 64
        .globl _const_15
_const_15:
        .long 7
        .long 0
        .long 7
        .long 0
        .long 7
        .long 0
        .long 7
        .long 0
        .long 7
        .long 0
        .long 7
        .long 0
        .long 7
        .long 0
        .long 7
        .long 0
        .align 64
        .globl _const_16
_const_16:
        .long 8
        .long 0
        .long 8
        .long 0
        .long 8
        .long 0
        .long 8
        .long 0
        .long 8
        .long 0
        .long 8
        .long 0
        .long 8
        .long 0
        .long 8
        .long 0
        .align 64
        .globl _const_17
_const_17:
        .long 9
        .long 0
        .long 9
        .long 0
        .long 9
        .long 0
        .long 9
        .long 0
        .long 9
        .long 0
        .long 9
        .long 0
        .long 9
        .long 0
        .long 9
        .long 0
        .align 64
        .globl _const_18
_const_18:
        .long 10
        .long 0
        .long 10
        .long 0
        .long 10
        .long 0
        .long 10
        .long 0
        .long 10
        .long 0
        .long 10
        .long 0
        .long 10
        .long 0
        .long 10
        .long 0
        .align 64
        .globl _const_19
_const_19:
        .long 11
        .long 0
        .long 11
        .long 0
        .long 11
        .long 0
        .long 11
        .long 0
        .long 11
        .long 0
        .long 11
        .long 0
        .long 11
        .long 0
        .long 11
        .long 0
        .align 64
        .globl _const_20
_const_20:
        .long 12
        .long 0
        .long 12
        .long 0
        .long 12
        .long 0
        .long 12
        .long 0
        .long 12
        .long 0
        .long 12
        .long 0
        .long 12
        .long 0
        .long 12
        .long 0
        .align 64
        .globl _const_21
_const_21:
        .long 13
        .long 0
        .long 13
        .long 0
        .long 13
        .long 0
        .long 13
        .long 0
        .long 13
        .long 0
        .long 13
        .long 0
        .long 13
        .long 0
        .long 13
        .long 0
        .align 64
        .globl _const_22
_const_22:
        .long 14
        .long 0
        .long 14
        .long 0
        .long 14
        .long 0
        .long 14
        .long 0
        .long 14
        .long 0
        .long 14
        .long 0
        .long 14
        .long 0
        .long 14
        .long 0
        .align 64
        .globl _const_23
_const_23:
        .long 15
        .long 0
        .long 15
        .long 0
        .long 15
        .long 0
        .long 15
        .long 0
        .long 15
        .long 0
        .long 15
        .long 0
        .long 15
        .long 0
        .long 15
        .long 0
        .align 64
        .globl _const_24
_const_24:
        .long 16
        .long 0
        .long 16
        .long 0
        .long 16
        .long 0
        .long 16
        .long 0
        .long 16
        .long 0
        .long 16
        .long 0
        .long 16
        .long 0
        .long 16
        .long 0
        .align 64
        .globl _const_25
_const_25:
        .long 17
        .long 0
        .long 17
        .long 0
        .long 17
        .long 0
        .long 17
        .long 0
        .long 17
        .long 0
        .long 17
        .long 0
        .long 17
        .long 0
        .long 17
        .long 0
        .align 64
        .globl _const_26
_const_26:
        .long 8
        .long 8
        .long 9
        .long 9
        .long 10
        .long 10
        .long 11
        .long 11
        .long 12
        .long 12
        .long 13
        .long 13
        .long 14
        .long 14
        .long 15
        .long 15
        .align 64
        .globl _const_27
_const_27:
        .long 0
        .long 0
        .long 1
        .long 1
        .long 2
        .long 2
        .long 3
        .long 3
        .long 4
        .long 4
        .long 5
        .long 5
        .long 6
        .long 6
        .long 7
        .long 7
        .align 64
        .globl _const_34
_const_34:
        .long 0
        .long 1072693248
        .long 0
        .long 1072693248
        .long 0
        .long 1072693248
        .long 0
        .long 1072693248
        .long 0
        .long 1072693248
        .long 0
        .long 1072693248
        .long 0
        .long 1072693248
        .long 0
        .long 1072693248
        .align 64
        .globl _const_35
_const_35:
        .long 0
        .long 1
        .long 0
        .long 1
        .long 0
        .long 1
        .long 0
        .long 1
        .long 0
        .long 1
        .long 0
        .long 1
        .long 0
        .long 1
        .long 0
        .long 1
# mark_begin;
# Threads 4
        .align    16,0x90
	.globl __Vectorized_.tmr_ocl_num_int_el
__Vectorized_.tmr_ocl_num_int_el:
..B2.1:                         # Preds ..B2.0 Latency 109
        pushq     %rbx                                          #
        movq      %rsp, %rbx                                    #
        andq      $-64, %rsp                                    #
        subq      $56, %rsp                                     #
        pushq     %rbp                                          #
        movq      8(%rbx), %rbp                                 #
        movq      %rbp, 8(%rsp)                                 #
        movq      %rsp, %rbp                                    #
        subq      $11328, %rsp                                  # c1
        movl      %eax, -464(%rbp)                              # c1
        movq      %r15, -1304(%rbp)                             # c5
        xorl      %r15d, %r15d                                  # c5
        movq      16(%rdi), %rax                                # c9
        movq      (%rdi), %rdx                                  # c9
        movq      %rax, -1320(%rbp)                             # c13
        movq      %r14, %rax                                    # c13
        movq      %rax, -10176(%rbp)                            # c17
        movl      $255, %eax                                    # c17
        kmov      %eax, %k5                                     # c21
        movq      -10176(%rbp), %rax                            # c21
        movq      %rax, -10176(%rbp)                            # c25
        movl      $21845, %eax                                  # c25
        kmov      %eax, %k3                                     # c29
        movq      -10176(%rbp), %rax                            # c29
        movq      %rax, -10176(%rbp)                            # c33
        movl      $43690, %eax                                  # c33
        movq      %rdx, -456(%rbp)                              # c37
        kmov      %eax, %k1                                     # c37
        movq      88(%rdi), %rcx                                # c41
        movq      80(%rdi), %rsi                                # c41
        movq      72(%rdi), %r8                                 # c45
        movq      48(%rdi), %r9                                 # c45
        movq      32(%rdi), %r10                                # c49
        movq      24(%rdi), %r11                                # c49
        movq      8(%rdi), %rdx                                 # c53
        movq      %r14, -1296(%rbp)                             # c53
        movl      $5, -1344(%rbp)                               # c57
        movq      %r13, -1288(%rbp)                             # c61
        movq      %r14, %r13                                    # c61
        movq      -10176(%rbp), %rax                            # c65
        movq      %rcx, -448(%rbp)                              # c65
        movq      %rsi, -424(%rbp)                              # c69
        movq      %r14, %rcx                                    # c69
        movq      %r8, -440(%rbp)                               # c73
        movq      %r14, %r8                                     # c73
        movq      %r9, -1336(%rbp)                              # c77
        movq      %r14, %rsi                                    # c77
        movq      %r10, -432(%rbp)                              # c81
        movq      %r14, %r10                                    # c81
        movq      %r11, -1328(%rbp)                             # c85
        movq      %r14, %r11                                    # c85
        movq      %rdx, -1312(%rbp)                             # c89
        movq      %r14, %rdi                                    # c89
        movq      %r14, -416(%rbp)                              # c93
        movq      %r14, %rdx                                    # c93
        movq      %r14, -392(%rbp)                              # c97
        movq      %r14, %r9                                     # c97
        movq      %r14, -400(%rbp)                              # c101
        movq      %r14, -408(%rbp)                              # c101
        movq      %r15, -472(%rbp)                              # c105
        movq      %r14, -480(%rbp)                              # c105
                                # LOE rax rdx rcx rsi rdi r8 r9 r10 r11 r12 r13 r14 r15 r14d r15d r14b r15b k1 k3 k5
..B2.43:                        # Preds ..B2.1 Latency 5
        movq      %r14, -312(%rbp)                              # c1
        movq      %r15, -336(%rbp)                              # c1
        movq      %r12, -1280(%rbp)                             # c5
                                # LOE rax rdx rcx rsi rdi r8 r9 r10 r11 r13 k1 k3 k5
..CL14:
..B2.2:                         # Preds ..B2.22 ..B2.24 ..B2.43 Latency 77
        vmovaps   _const_5(%rip), %zmm0                         # c1
        vmovaps   _const_6(%rip), %zmm2                         # c5
        movq      -336(%rbp), %r14                              # c9
        movq      -440(%rbp), %r12                              # c9
        movq      %r14, %r15                                    # c13
        shlq      $5, %r15                                      # c17
        vbroadcastsd (%r15,%r12), %zmm1                         # c21
        vmovaps   %zmm1, %zmm3                                  # c25
        vpaddsetcd _const_4(%rip), %k6, %zmm1{%k3}              # c29
        vpaddsetcd _const_3(%rip), %k2, %zmm3{%k3}              # c33
        kmov      %k6, %r12d                                    # c37
        movb      %al, %al                                      # c37
        kmov      %k2, %r15d                                    # c41
        addl      %r12d, %r12d                                  # c41
        addl      %r15d, %r15d                                  # c45
        kmov      %r12d, %k7                                    # c45
        vpadcd    _const_4(%rip), %k7, %zmm1{%k1}               # c49
        vpermd    %zmm1, %zmm0, %zmm4                           # c53
        kmov      %r15d, %k4                                    # c53
        vpadcd    _const_3(%rip), %k4, %zmm3{%k1}               # c57
        vpermd    %zmm3, %zmm2, %zmm4{%k5}                      # c61
        movq      -448(%rbp), %r15                              # c61
        movq      -472(%rbp), %r12                              # c65
        vmovdqa32 %zmm4, (%r12,%r15)                            # c69
        movq      -456(%rbp), %r15                              # c69
        movl      (%r15), %r15d                                 # c73
        testl     %r15d, %r15d                                  # c77
        jle       ..B2.26       # Prob 0%                       # c77
                                # LOE rax rdx rcx rsi rdi r8 r9 r10 r11 r12 r13 r14 r15d k1 k3 k5
..B2.3:                         # Preds ..B2.2 Latency 1
..CL16:
..B2.4:                         # Preds ..B2.3 Latency 183
        movq      -1336(%rbp), %rdx                             # c1
        movl      %r15d, -1272(%rbp)                            # c1
        movq      $0, -328(%rbp)                                # c5
        movl      (%rdx), %r10d                                 # c9
        movq      -456(%rbp), %r9                               # c9
        imull     %r15d, %r10d                                  # c13
        shll      $4, %r10d                                     # c23
        movl      4(%r9), %r13d                                 # c23
        lea       (%r13,%r13,8), %ecx                           # c27
        movq      -448(%rbp), %rax                              # c27
        movq      -472(%rbp), %r11                              # c31
        lea       5(%rcx,%rcx), %r9d                            # c31
        lea       5888(%r11,%rax), %rdi                         # c35
        movl      %r9d, -936(%rbp)                              # c35
        lea       4(%rcx,%rcx), %r9d                            # c39
        movq      %rdi, -320(%rbp)                              # c39
        movl      %r10d, -464(%rbp)                             # c43
        lea       (%rcx,%rcx), %r8d                             # c43
        movq      %rdi, 64(%rax,%r11)                           # c47
        lea       15(%rcx,%rcx), %r13d                          # c47
        lea       14(%rcx,%rcx), %r12d                          # c51
        lea       13(%rcx,%rcx), %r11d                          # c51
        lea       12(%rcx,%rcx), %r10d                          # c55
        lea       11(%rcx,%rcx), %r15d                          # c55
        lea       10(%rcx,%rcx), %edx                           # c59
        lea       9(%rcx,%rcx), %esi                            # c59
        lea       8(%rcx,%rcx), %edi                            # c63
        lea       7(%rcx,%rcx), %eax                            # c63
        lea       6(%rcx,%rcx), %r14d                           # c67
        movl      %r9d, -928(%rbp)                              # c67
        lea       3(%rcx,%rcx), %r9d                            # c71
        lea       2(%rcx,%rcx), %ecx                            # c71
        movl      %ecx, -920(%rbp)                              # c75
        movl      %r8d, %ecx                                    # c75
        orl       $1, %ecx                                      # c79
        movslq    %r8d, %r8                                     # c79
        movslq    %ecx, %rcx                                    # c83
        movslq    %r13d, %r13                                   # c83
        movq      %rcx, -888(%rbp)                              # c87
        movslq    %r11d, %r11                                   # c87
        movq      -1328(%rbp), %rcx                             # c91
        movslq    %edx, %rdx                                    # c91
        lea       (%rcx,%r8,8), %r8                             # c95
        movslq    %esi, %rsi                                    # c95
        movslq    %r14d, %r14                                   # c99
        movq      %r8, -408(%rbp)                               # c99
        lea       (%rcx,%r13,8), %r8                            # c103
        movslq    %r12d, %r12                                   # c103
        movslq    %r10d, %r10                                   # c107
        movq      %r14, -912(%rbp)                              # c107
        movq      %r8, -416(%rbp)                               # c111
        lea       (%rcx,%r11,8), %r8                            # c111
        movl      -936(%rbp), %r14d                             # c115
        movslq    %edi, %rdi                                    # c115
        movslq    %eax, %rax                                    # c119
        movq      %r8, -480(%rbp)                               # c119
        lea       (%rcx,%rdx,8), %r8                            # c123
        lea       (%rcx,%rsi,8), %rdx                           # c123
        movq      -912(%rbp), %rsi                              # c127
        movslq    %r14d, %r14                                   # c127
        movslq    %r9d, %r9                                     # c131
        movslq    %r15d, %r15                                   # c131
        movq      %r14, -904(%rbp)                              # c135
        lea       (%rcx,%r12,8), %r12                           # c135
        movl      -928(%rbp), %r14d                             # c139
        movq      %r9, -896(%rbp)                               # c139
        lea       (%rcx,%r10,8), %r10                           # c143
        movslq    %r14d, %r14                                   # c143
        movl      -920(%rbp), %r9d                              # c147
        movq      %r12, -400(%rbp)                              # c147
        movslq    %r9d, %r9                                     # c151
        movq      %r10, -312(%rbp)                              # c151
        lea       (%rcx,%rdi,8), %r13                           # c155
        lea       (%rcx,%rax,8), %r10                           # c155
        lea       (%rcx,%rsi,8), %rdi                           # c159
        movq      -904(%rbp), %rax                              # c159
        movq      -896(%rbp), %rsi                              # c163
        lea       (%rcx,%r15,8), %r15                           # c163
        movq      -888(%rbp), %r12                              # c167
        movq      %r15, -392(%rbp)                              # c167
        lea       (%rcx,%rax,8), %r11                           # c171
        lea       (%rcx,%r14,8), %rax                           # c171
        lea       (%rcx,%rsi,8), %rsi                           # c175
        lea       (%rcx,%r9,8), %r9                             # c175
        lea       (%rcx,%r12,8), %rcx                           # c179
        movl      -1272(%rbp), %r15d                            # c179
                                # LOE rax rdx rcx rsi rdi r8 r9 r10 r11 r13 r15 r15d r15b k1 k3 k5
..B2.42:                        # Preds ..B2.4 Latency 29
        movl      %r15d, -1272(%rbp)                            # c1
        movq      -432(%rbp), %r14                              # c1
        movq      %r9, -304(%rbp)                               # c5
        movq      %rax, -368(%rbp)                              # c5
        movq      %rdx, -296(%rbp)                              # c9
        movq      -472(%rbp), %rax                              # c9
        movq      %rdi, -288(%rbp)                              # c13
        movq      %rcx, -360(%rbp)                              # c13
        movq      %rsi, -280(%rbp)                              # c17
        movq      -336(%rbp), %rcx                              # c17
        movq      %r8, -272(%rbp)                               # c21
        movq      %r13, -352(%rbp)                              # c21
        movq      %r10, -376(%rbp)                              # c25
        movq      -448(%rbp), %r13                              # c25
        movq      %r11, -344(%rbp)                              # c29
                                # LOE rax rcx r13 r14 k1 k3 k5
..CL17:
..B2.5:                         # Preds ..B2.17 ..B2.42 Latency 613
        vmovaps   _const_7(%rip), %zmm1                         # c1
        vpxord    %zmm14, %zmm14, %zmm14                        # c5
        lea       3200(%r13,%rax), %r15                         # c5
        movq      $0, -256(%rbp)                                # c9
        movq      %r15, -248(%rbp)                              # c13
        lea       3328(%r13,%rax), %r15                         # c13
        movq      %r15, -240(%rbp)                              # c17
        lea       3456(%r13,%rax), %r15                         # c17
        movq      %r15, -232(%rbp)                              # c21
        lea       3584(%r13,%rax), %r15                         # c21
        movq      %r15, -224(%rbp)                              # c25
        lea       3712(%r13,%rax), %r15                         # c25
        movq      %r15, -216(%rbp)                              # c29
        lea       3840(%r13,%rax), %r15                         # c29
        movq      %r15, -208(%rbp)                              # c33
        lea       3968(%r13,%rax), %r15                         # c33
        movq      %r15, -200(%rbp)                              # c37
        lea       4096(%r13,%rax), %r15                         # c37
        movq      %r15, -192(%rbp)                              # c41
        lea       4224(%r13,%rax), %r15                         # c41
        movq      %r15, -184(%rbp)                              # c45
        lea       4352(%r13,%rax), %r15                         # c45
        movq      %r15, -176(%rbp)                              # c49
        lea       4480(%r13,%rax), %r15                         # c49
        movq      %r15, -168(%rbp)                              # c53
        lea       4608(%r13,%rax), %r15                         # c53
        movq      %r15, -160(%rbp)                              # c57
        lea       4736(%r13,%rax), %r15                         # c57
        movq      %r15, -152(%rbp)                              # c61
        lea       4864(%r13,%rax), %r15                         # c61
        movq      %r15, -144(%rbp)                              # c65
        lea       4992(%r13,%rax), %r15                         # c65
        movq      %r15, -136(%rbp)                              # c69
        lea       5120(%r13,%rax), %r15                         # c69
        movq      %r15, -128(%rbp)                              # c73
        lea       5248(%r13,%rax), %r15                         # c73
        movq      %r15, -120(%rbp)                              # c77
        lea       5376(%r13,%rax), %r15                         # c77
        movq      %r15, -112(%rbp)                              # c81
        lea       5504(%r13,%rax), %r15                         # c81
        movq      %r15, -104(%rbp)                              # c85
        lea       5632(%r13,%rax), %r15                         # c85
        movq      %r14, -432(%rbp)                              # c89
        lea       2560(%r13,%rax), %r12                         # c89
        movq      -328(%rbp), %r14                              # c93
        vmovdqa32 %zmm14, 2624(%rax,%r13)                       # c93
        shll      $4, %r14d                                     # c97
        vmovdqa32 %zmm14, (%r12)                                # c97
        addl      -464(%rbp), %r14d                             # c101
        vmovdqa32 %zmm14, 2752(%rax,%r13)                       # c101
        movq      %r15, -96(%rbp)                               # c105
        lea       5760(%r13,%rax), %r15                         # c105
        movq      %r15, -88(%rbp)                               # c109
        lea       6016(%r13,%rax), %r15                         # c109
        movq      %r15, -80(%rbp)                               # c113
        lea       6144(%r13,%rax), %r15                         # c113
        movq      %r15, -72(%rbp)                               # c117
        lea       6272(%r13,%rax), %r15                         # c117
        movq      %r15, -64(%rbp)                               # c121
        lea       6400(%r13,%rax), %r15                         # c121
        movl      %r14d, -384(%rbp)                             # c125
        lea       2688(%r13,%rax), %rdx                         # c125
        vbroadcastss -384(%rbp), %zmm0                          # c129
        vmovdqa32 %zmm14, (%rdx)                                # c129
        vpermd    %zmm0, %zmm1, %zmm15                          # c133
        vmovdqa32 %zmm14, 2112(%rax,%r13)                       # c133
        movl      %r14d, 80(%rax,%r13)                          # c137
        movq      -408(%rbp), %r14                              # c137
        vbroadcastsd (%r14), %zmm2                              # c141
        vmovdqa32 %zmm14, 2240(%rax,%r13)                       # c141
        movq      %r15, -56(%rbp)                               # c145
        lea       6528(%r13,%rax), %r15                         # c145
        movq      %r15, -48(%rbp)                               # c149
        lea       2048(%r13,%rax), %r11                         # c149
        movq      -328(%rbp), %r15                              # c153
        vmovdqa32 %zmm14, 2368(%rax,%r13)                       # c153
        movq      %r15, 72(%rax,%r13)                           # c157
        movq      -360(%rbp), %r14                              # c157
        vbroadcastsd (%r14), %zmm3                              # c161
        vmovdqa32 %zmm14, 2496(%rax,%r13)                       # c161
        movq      -304(%rbp), %r15                              # c165
        vmovdqa32 %zmm14, 2880(%rax,%r13)                       # c165
        vbroadcastsd (%r15), %zmm4                              # c169
        vmovdqa32 %zmm14, 3008(%rax,%r13)                       # c169
        movq      -280(%rbp), %r14                              # c173
        vmovdqa32 %zmm14, 3136(%rax,%r13)                       # c173
        vbroadcastsd (%r14), %zmm5                              # c177
        vmovdqa32 %zmm14, 3264(%rax,%r13)                       # c177
        movq      -368(%rbp), %r15                              # c181
        vmovdqa32 %zmm14, (%r11)                                # c181
        vbroadcastsd (%r15), %zmm6                              # c185
        vmovdqa32 %zmm14, 3392(%rax,%r13)                       # c185
        movq      -344(%rbp), %r14                              # c189
        vmovdqa32 %zmm14, 3520(%rax,%r13)                       # c189
        vbroadcastsd (%r14), %zmm7                              # c193
        vmovdqa32 %zmm14, 3648(%rax,%r13)                       # c193
        movq      -288(%rbp), %r15                              # c197
        vmovdqa32 %zmm14, 3776(%rax,%r13)                       # c197
        vbroadcastsd (%r15), %zmm8                              # c201
        vmovdqa32 %zmm14, 3904(%rax,%r13)                       # c201
        movq      -376(%rbp), %r14                              # c205
        vmovdqa32 %zmm14, 4032(%rax,%r13)                       # c205
        vbroadcastsd (%r14), %zmm9                              # c209
        vmovdqa32 %zmm14, 4160(%rax,%r13)                       # c209
        movq      -352(%rbp), %r15                              # c213
        vmovdqa32 %zmm14, 4288(%rax,%r13)                       # c213
        vbroadcastsd (%r15), %zmm10                             # c217
        vmovdqa32 %zmm14, 4416(%rax,%r13)                       # c217
        movq      -296(%rbp), %r14                              # c221
        vmovdqa32 %zmm14, 4544(%rax,%r13)                       # c221
        vbroadcastsd (%r14), %zmm11                             # c225
        vmovdqa32 %zmm14, 4672(%rax,%r13)                       # c225
        movq      -272(%rbp), %r15                              # c229
        vmovdqa32 %zmm14, 4800(%rax,%r13)                       # c229
        vbroadcastsd (%r15), %zmm12                             # c233
        vmovdqa32 %zmm14, 4928(%rax,%r13)                       # c233
        movq      -392(%rbp), %r14                              # c237
        vmovdqa32 %zmm14, 5056(%rax,%r13)                       # c237
        vbroadcastsd (%r14), %zmm13                             # c241
        vmovdqa32 %zmm14, 5184(%rax,%r13)                       # c241
        movq      -312(%rbp), %r15                              # c245
        vmovdqa32 %zmm14, 5312(%rax,%r13)                       # c245
        movq      (%r15), %r14                                  # c249
        vmovdqa32 %zmm14, 5440(%rax,%r13)                       # c249
        movq      -480(%rbp), %r15                              # c253
        vmovdqa32 %zmm14, 5568(%rax,%r13)                       # c253
        movq      %r14, 1664(%rax,%r13)                         # c257
        movq      (%r15), %r14                                  # c257
        movq      -400(%rbp), %r15                              # c261
        vmovdqa32 %zmm14, 5696(%rax,%r13)                       # c261
        movq      %rcx, -336(%rbp)                              # c265
        lea       2176(%r13,%rax), %r10                         # c265
        lea       2304(%r13,%rax), %r9                          # c269
        vmovdqa32 %zmm14, (%r10)                                # c269
        lea       2432(%r13,%rax), %r8                          # c273
        vmovdqa32 %zmm14, (%r9)                                 # c273
        lea       2816(%r13,%rax), %rcx                         # c277
        vmovdqa32 %zmm14, (%r8)                                 # c277
        lea       2944(%r13,%rax), %rsi                         # c281
        vmovdqa32 %zmm14, (%rcx)                                # c281
        lea       3072(%r13,%rax), %rdi                         # c285
        vmovdqa32 %zmm14, (%rsi)                                # c285
        movq      %r12, 1728(%rax,%r13)                         # c289
        movq      -248(%rbp), %r12                              # c289
        movq      %rdx, 1736(%rax,%r13)                         # c293
        movq      %r11, 1696(%rax,%r13)                         # c293
        movq      %r14, 1672(%rax,%r13)                         # c297
        movq      %r10, 1704(%rax,%r13)                         # c297
        movq      %r9, 1712(%rax,%r13)                          # c301
        movq      %r8, 1720(%rax,%r13)                          # c301
        movq      %rcx, 1744(%rax,%r13)                         # c305
        movq      %rsi, 1752(%rax,%r13)                         # c305
        movq      %rdi, 1760(%rax,%r13)                         # c309
        movq      %r12, 1768(%rax,%r13)                         # c309
        vmovdqa32 %zmm14, (%rdi)                                # c313
        movq      -240(%rbp), %rdx                              # c313
        movq      (%r15), %r14                                  # c317
        vmovdqa32 %zmm14, (%r12)                                # c317
        movq      -416(%rbp), %r15                              # c321
        vmovdqa32 %zmm14, (%rdx)                                # c321
        movq      -232(%rbp), %rcx                              # c325
        vmovdqa32 %zmm14, 5824(%rax,%r13)                       # c325
        movq      -224(%rbp), %rsi                              # c329
        vmovdqa32 %zmm14, (%rcx)                                # c329
        movq      -216(%rbp), %rdi                              # c333
        vmovdqa32 %zmm14, (%rsi)                                # c333
        movq      -208(%rbp), %r8                               # c337
        vmovdqa32 %zmm14, (%rdi)                                # c337
        movq      -200(%rbp), %r9                               # c341
        vmovdqa32 %zmm14, (%r8)                                 # c341
        movq      -192(%rbp), %r10                              # c345
        vmovdqa32 %zmm14, (%r9)                                 # c345
        movq      -184(%rbp), %r11                              # c349
        vmovdqa32 %zmm14, (%r10)                                # c349
        movq      -176(%rbp), %r12                              # c353
        vmovdqa32 %zmm14, (%r11)                                # c353
        movq      %rdx, 1776(%rax,%r13)                         # c357
        movq      -168(%rbp), %rdx                              # c357
        movq      %r14, 1680(%rax,%r13)                         # c361
        movq      %rcx, 1784(%rax,%r13)                         # c361
        movq      %rsi, 1792(%rax,%r13)                         # c365
        movq      %rdi, 1800(%rax,%r13)                         # c365
        movq      %r8, 1808(%rax,%r13)                          # c369
        movq      %r9, 1816(%rax,%r13)                          # c369
        movq      %r10, 1824(%rax,%r13)                         # c373
        movq      %r11, 1832(%rax,%r13)                         # c373
        movq      %r12, 1840(%rax,%r13)                         # c377
        movq      %rdx, 1848(%rax,%r13)                         # c377
        vmovdqa32 %zmm14, (%r12)                                # c381
        movq      (%r15), %r14                                  # c381
        movq      %r14, 1688(%rax,%r13)                         # c385
        movq      -160(%rbp), %rcx                              # c385
        movq      -152(%rbp), %rsi                              # c389
        vmovdqa32 %zmm14, (%rdx)                                # c389
        movq      -144(%rbp), %rdi                              # c393
        vmovdqa64 %zmm2, 128(%rax,%r13)                         # c393
        movq      -136(%rbp), %r8                               # c397
        vmovdqa64 %zmm2, 192(%rax,%r13)                         # c397
        movq      -128(%rbp), %r9                               # c401
        vmovdqa64 %zmm3, 256(%rax,%r13)                         # c401
        movq      -120(%rbp), %r10                              # c405
        vmovdqa64 %zmm3, 320(%rax,%r13)                         # c405
        movq      -112(%rbp), %r11                              # c409
        vmovdqa64 %zmm4, 384(%rax,%r13)                         # c409
        movq      -104(%rbp), %r14                              # c413
        vmovdqa64 %zmm4, 448(%rax,%r13)                         # c413
        movq      -96(%rbp), %r15                               # c417
        vmovdqa64 %zmm5, 512(%rax,%r13)                         # c417
        movq      -88(%rbp), %r12                               # c421
        vmovdqa64 %zmm5, 576(%rax,%r13)                         # c421
        movq      -320(%rbp), %rdx                              # c425
        vmovdqa64 %zmm6, 640(%rax,%r13)                         # c425
        vmovdqa64 %zmm6, 704(%rax,%r13)                         # c429
        vmovdqa64 %zmm7, 768(%rax,%r13)                         # c433
        vmovdqa64 %zmm7, 832(%rax,%r13)                         # c437
        vmovdqa64 %zmm8, 896(%rax,%r13)                         # c441
        vmovdqa64 %zmm8, 960(%rax,%r13)                         # c445
        vmovdqa64 %zmm9, 1024(%rax,%r13)                        # c449
        vmovdqa64 %zmm9, 1088(%rax,%r13)                        # c453
        vmovdqa64 %zmm10, 1152(%rax,%r13)                       # c457
        vmovdqa64 %zmm10, 1216(%rax,%r13)                       # c461
        vmovdqa64 %zmm11, 1280(%rax,%r13)                       # c465
        vmovdqa64 %zmm11, 1344(%rax,%r13)                       # c469
        vmovdqa64 %zmm12, 1408(%rax,%r13)                       # c473
        vmovdqa64 %zmm12, 1472(%rax,%r13)                       # c477
        vmovdqa64 %zmm13, 1536(%rax,%r13)                       # c481
        vmovdqa64 %zmm13, 1600(%rax,%r13)                       # c485
        movb      %dl, %dl                                      # c485
        movq      %rcx, 1856(%rax,%r13)                         # c489
        movq      %rsi, 1864(%rax,%r13)                         # c489
        vmovdqa32 %zmm14, (%rcx)                                # c493
        movq      -80(%rbp), %rcx                               # c493
        vmovdqa32 %zmm14, (%rsi)                                # c497
        movq      -72(%rbp), %rsi                               # c497
        movq      %rdi, 1872(%rax,%r13)                         # c501
        movq      %r8, 1880(%rax,%r13)                          # c501
        vmovdqa32 %zmm14, (%rdi)                                # c505
        movq      -64(%rbp), %rdi                               # c505
        vmovdqa32 %zmm14, (%r8)                                 # c509
        movq      -56(%rbp), %r8                                # c509
        movq      %r9, 1888(%rax,%r13)                          # c513
        movq      %r10, 1896(%rax,%r13)                         # c513
        vmovdqa32 %zmm14, (%r9)                                 # c517
        movq      -48(%rbp), %r9                                # c517
        vmovdqa32 %zmm14, (%r10)                                # c521
        movq      -424(%rbp), %r10                              # c521
        movq      %r11, 1904(%rax,%r13)                         # c525
        movq      %r14, 1912(%rax,%r13)                         # c525
        vmovdqa32 %zmm14, (%r11)                                # c529
        vmovdqa32 %zmm14, (%r14)                                # c533
        movq      -432(%rbp), %r14                              # c533
        movq      %r15, 1920(%rax,%r13)                         # c537
        movq      %r12, 1928(%rax,%r13)                         # c537
        vmovdqa32 %zmm14, (%r15)                                # c541
        vmovdqa32 %zmm14, (%r12)                                # c545
        movq      -256(%rbp), %r12                              # c545
        vmovdqa32 %zmm14, 64(%rdx)                              # c549
        vmovdqa32 %zmm14, (%rdx)                                # c553
        vpaddd    (%rax,%r13), %zmm15, %zmm16                   # c557
        vmovdqa32 %zmm14, 6080(%rax,%r13)                       # c557
        movq      %rcx, 1936(%rax,%r13)                         # c561
        movq      %rsi, 1944(%rax,%r13)                         # c561
        vmovdqa32 %zmm14, (%rcx)                                # c565
        movq      -336(%rbp), %rcx                              # c565
        vmovdqa32 %zmm14, 6208(%rax,%r13)                       # c569
        cmpq      %r10, %rcx                                    # c569
        vmovdqa32 %zmm14, 6336(%rax,%r13)                       # c573
        vmovdqa32 %zmm14, 6464(%rax,%r13)                       # c577
        vmovdqa32 %zmm14, 6592(%rax,%r13)                       # c581
        movl      $1, -264(%rbp)                                # c585
        vmovdqa32 %zmm14, (%rsi)                                # c589
        movb      %dl, %dl                                      # c589
        movq      %rdi, 1952(%rax,%r13)                         # c593
        movq      %r8, 1960(%rax,%r13)                          # c593
        vmovdqa32 %zmm14, (%rdi)                                # c597
        vmovdqa32 %zmm14, (%r8)                                 # c601
        movb      %dl, %dl                                      # c601
        movq      %r9, 1968(%rax,%r13)                          # c605
        movb      %dl, %dl                                      # c605
        vmovdqa32 %zmm14, (%r9)                                 # c609
        vmovdqa32 %zmm16, 1984(%rax,%r13)                       # c613
        jae       ..B2.9        # Prob 0%                       # c613
                                # LOE rax rcx r12 r13 r14 k1 k3 k5
..B2.6:                         # Preds ..B2.5 Latency 1
..CL19:
..B2.7:                         # Preds ..B2.6 Latency 17
        incq      %rcx                                          # c1
        addq      $6656, %rax                                   # c1
        movl      -1344(%rbp), %edx                             # c5
        movq      %rcx, -256(%rbp)                              # c5
        movl      %edx, -264(%rbp)                              # c9
        movq      %rax, %r12                                    # c9
        cmpl      $1, -1344(%rbp)                               # c13
        jne       ..B2.24       # Prob 0%                       # c17
                                # LOE rax rcx r12 r13 r14 ecx cl ch k1 k3 k5
..B2.8:                         # Preds ..B2.7 Latency 1
..CL18:
..B2.9:                         # Preds ..B2.23 ..B2.8 ..B2.5 Latency 877
        vmovaps   _const_26(%rip), %zmm15                       # c1
        vmovaps   _const_7(%rip), %zmm17                        # c5
        vmovaps   _const_27(%rip), %zmm16                       # c9
        lea       (%r12,%r13), %rsi                             # c13
        movq      -1328(%rbp), %rcx                             # c13
        vmovaps   1984(%rsi), %zmm30                            # c17
        movq      %rsi, -1184(%rbp)                             # c17
        vpmulld   _const_8(%rip), %zmm30, %zmm18                # c21
        vpermd    %zmm18, %zmm15, %zmm14                        # c25
        xorl      %r9d, %r9d                                    # c25
        vpsrad    $31, %zmm14, %zmm14{%k1}                      # c29
        vpermd    %zmm18, %zmm17, %zmm19                        # c33
        vmovaps   %zmm14, %zmm17                                # c37
        vpermd    %zmm18, %zmm16, %zmm30                        # c41
        vmovaps   %zmm14, %zmm16                                # c45
        vpaddsetcd _const_9(%rip), %k6, %zmm17{%k3}             # c49
        vpaddsetcd _const_11(%rip), %k2, %zmm16{%k3}            # c53
        vpsubd    %zmm19, %zmm18, %zmm20                        # c57
        kmov      %k6, %r8d                                     # c57
        vmovaps   %zmm14, %zmm19                                # c61
        kmov      %k2, %edi                                     # c61
        vmovdqa32 %zmm18, -10240(%rbp)                          # c65
        vmovaps   %zmm14, %zmm18                                # c69
        movl      -10240(%rbp), %r10d                           # c69
        vpaddsetcd _const_13(%rip), %k6, %zmm19{%k3}            # c73
        vpaddsetcd _const_12(%rip), %k2, %zmm18{%k3}            # c77
        vmovaps   %zmm14, %zmm22                                # c81
        kmov      %k6, %r11d                                    # c81
        vmovaps   %zmm14, %zmm21                                # c85
        addl      %r11d, %r11d                                  # c85
        vpaddsetcd _const_16(%rip), %k6, %zmm22{%k3}            # c89
        vmovaps   %zmm20, -9984(%rbp)                           # c93
        vmovaps   %zmm14, %zmm20                                # c97
        addl      %edi, %edi                                    # c97
        shlq      $32, %r10                                     # c101
        kmov      %k2, %r15d                                    # c101
        sarq      $32, %r10                                     # c105
        kmov      %r11d, %k2                                    # c105
        vpadcd    _const_13(%rip), %k2, %zmm19{%k1}             # c109
        vpaddsetcd _const_15(%rip), %k2, %zmm21{%k3}            # c113
        vmovaps   %zmm14, %zmm25                                # c117
        kmov      %edi, %k4                                     # c117
        vpadcd    _const_11(%rip), %k4, %zmm16{%k1}             # c121
        vmovaps   %zmm14, %zmm24                                # c125
        addl      %r15d, %r15d                                  # c125
        vmovaps   %zmm14, %zmm23                                # c129
        kmov      %r15d, %k4                                    # c129
        vpadcd    _const_12(%rip), %k4, %zmm18{%k1}             # c133
        vpaddsetcd _const_14(%rip), %k4, %zmm20{%k3}            # c137
        vmovaps   %zmm14, %zmm28                                # c141
        lea       (%rcx,%r10,8), %rsi                           # c141
        vmovaps   %zmm14, %zmm27                                # c145
        movq      %rsi, -1232(%rbp)                             # c145
        vmovaps   %zmm14, %zmm26                                # c149
        kmov      %k6, %esi                                     # c149
        vpaddsetcd _const_19(%rip), %k6, %zmm25{%k3}            # c153
        vmovaps   %zmm14, %zmm0                                 # c157
        addl      %esi, %esi                                    # c157
        vmovaps   %zmm14, %zmm29                                # c161
        kmov      %k2, %eax                                     # c161
        vporq     _const_10(%rip), %zmm14, %zmm15               # c165
        vpsrad    $31, %zmm30, %zmm30{%k1}                      # c169
        kmov      %esi, %k2                                     # c169
        vpadcd    _const_16(%rip), %k2, %zmm22{%k1}             # c173
        vpaddsetcd _const_18(%rip), %k2, %zmm24{%k3}            # c177
        vmovaps   %zmm30, %zmm2                                 # c181
        addl      %eax, %eax                                    # c181
        vmovaps   %zmm30, %zmm1                                 # c185
        kmov      %k4, %edx                                     # c185
        vmovaps   %zmm30, %zmm5                                 # c189
        kmov      %eax, %k4                                     # c189
        vpadcd    _const_15(%rip), %k4, %zmm21{%k1}             # c193
        vpaddsetcd _const_17(%rip), %k4, %zmm23{%k3}            # c197
        vmovaps   %zmm30, %zmm4                                 # c201
        kmov      %k6, %r15d                                    # c201
        vpaddsetcd _const_22(%rip), %k6, %zmm28{%k3}            # c205
        vmovaps   %zmm30, %zmm3                                 # c209
        addl      %r8d, %r8d                                    # c209
        vmovaps   %zmm30, %zmm8                                 # c213
        addl      %r15d, %r15d                                  # c213
        vmovaps   %zmm30, %zmm7                                 # c217
        kmov      %r8d, %k7                                     # c217
        vpadcd    _const_9(%rip), %k7, %zmm17{%k1}              # c221
        vmovaps   %zmm30, %zmm6                                 # c225
        kmov      %k2, %r8d                                     # c225
        vmovaps   %zmm30, %zmm11                                # c229
        kmov      %r15d, %k2                                    # c229
        vpadcd    _const_19(%rip), %k2, %zmm25{%k1}             # c233
        vpaddsetcd _const_21(%rip), %k2, %zmm27{%k3}            # c237
        vmovaps   %zmm30, %zmm10                                # c241
        addl      %r8d, %r8d                                    # c241
        vmovaps   %zmm30, %zmm9                                 # c245
        kmov      %k4, %edi                                     # c245
        vmovaps   %zmm30, %zmm13                                # c249
        kmov      %r8d, %k4                                     # c249
        vpadcd    _const_18(%rip), %k4, %zmm24{%k1}             # c253
        vpaddsetcd _const_20(%rip), %k4, %zmm26{%k3}            # c257
        vmovaps   %zmm30, %zmm12                                # c261
        addl      %edx, %edx                                    # c261
        vmovaps   %zmm30, %zmm31                                # c265
        kmov      %edx, %k7                                     # c265
        vpadcd    _const_14(%rip), %k7, %zmm20{%k1}             # c269
        kmov      %k6, %edx                                     # c273
        kmov      %k2, %r10d                                    # c273
        vpaddsetcd _const_25(%rip), %k6, %zmm14{%k3}            # c277
        addl      %edx, %edx                                    # c281
        addl      %r10d, %r10d                                  # c281
        kmov      %edx, %k2                                     # c285
        kmov      %k4, %r11d                                    # c285
        vpadcd    _const_22(%rip), %k2, %zmm28{%k1}             # c289
        vpaddsetcd _const_24(%rip), %k2, %zmm0{%k3}             # c293
        kmov      %r10d, %k4                                    # c297
        addl      %edi, %edi                                    # c297
        vpadcd    _const_21(%rip), %k4, %zmm27{%k1}             # c301
        vpaddsetcd _const_23(%rip), %k4, %zmm29{%k3}            # c305
        kmov      %edi, %k7                                     # c309
        kmov      %k2, %edi                                     # c309
        vpadcd    _const_17(%rip), %k7, %zmm23{%k1}             # c313
        addl      %edi, %edi                                    # c317
        kmov      %k4, %esi                                     # c317
        kmov      %edi, %k4                                     # c321
        kmov      %k6, %r8d                                     # c321
        vpadcd    _const_24(%rip), %k4, %zmm0{%k1}              # c325
        vpaddsetcd _const_12(%rip), %k6, %zmm2{%k3}             # c329
        addl      %r8d, %r8d                                    # c333
        vmovaps   %zmm0, -6528(%rbp)                            # c333
        vmovaps   %zmm30, %zmm0                                 # c337
        kmov      %r8d, %k2                                     # c337
        vpadcd    _const_25(%rip), %k2, %zmm14{%k1}             # c341
        vpaddsetcd _const_9(%rip), %k2, %zmm1{%k3}              # c345
        vpaddsetcd _const_11(%rip), %k4, %zmm0{%k3}             # c349
        kmov      %k6, %r10d                                    # c353
        vmovaps   %zmm14, -6464(%rbp)                           # c353
        vpaddsetcd _const_15(%rip), %k6, %zmm5{%k3}             # c357
        vporq     _const_10(%rip), %zmm30, %zmm14               # c361
        addl      %r11d, %r11d                                  # c365
        addl      %r10d, %r10d                                  # c365
        vmovdqa64 %zmm14, -10304(%rbp)                          # c369
        kmov      %r11d, %k7                                    # c373
        kmov      %k2, %r11d                                    # c373
        vpadcd    _const_20(%rip), %k7, %zmm26{%k1}             # c377
        kmov      %r10d, %k2                                    # c381
        addl      %r11d, %r11d                                  # c381
        vpadcd    _const_12(%rip), %k2, %zmm2{%k1}              # c385
        vpaddsetcd _const_14(%rip), %k2, %zmm4{%k3}             # c389
        vmovdqa64 %zmm2, -10496(%rbp)                           # c393
        kmov      %k4, %r15d                                    # c397
        kmov      %r11d, %k4                                    # c397
        vpadcd    _const_9(%rip), %k4, %zmm1{%k1}               # c401
        vpaddsetcd _const_13(%rip), %k4, %zmm3{%k3}             # c405
        vmovdqa64 %zmm1, -10432(%rbp)                           # c409
        kmov      %k6, %edi                                     # c413
        addl      %esi, %esi                                    # c413
        vpaddsetcd _const_18(%rip), %k6, %zmm8{%k3}             # c417
        addl      %edi, %edi                                    # c421
        kmov      %esi, %k7                                     # c421
        vpadcd    _const_23(%rip), %k7, %zmm29{%k1}             # c425
        kmov      %k2, %esi                                     # c429
        kmov      %edi, %k2                                     # c429
        vpadcd    _const_15(%rip), %k2, %zmm5{%k1}              # c433
        vpaddsetcd _const_17(%rip), %k2, %zmm7{%k3}             # c437
        vmovdqa64 %zmm5, -10688(%rbp)                           # c441
        addl      %esi, %esi                                    # c445
        kmov      %k4, %eax                                     # c445
        kmov      %esi, %k4                                     # c449
        kmov      %k6, %r11d                                    # c449
        vpadcd    _const_14(%rip), %k4, %zmm4{%k1}              # c453
        vpaddsetcd _const_16(%rip), %k4, %zmm6{%k3}             # c457
        vpaddsetcd _const_21(%rip), %k6, %zmm11{%k3}            # c461
        vmovdqa64 %zmm4, -10624(%rbp)                           # c465
        addl      %r15d, %r15d                                  # c469
        addl      %r11d, %r11d                                  # c469
        kmov      %r15d, %k7                                    # c473
        kmov      %k2, %r15d                                    # c473
        vpadcd    _const_11(%rip), %k7, %zmm0{%k1}              # c477
        kmov      %r11d, %k2                                    # c481
        addl      %r15d, %r15d                                  # c481
        vpadcd    _const_18(%rip), %k2, %zmm8{%k1}              # c485
        vpaddsetcd _const_20(%rip), %k2, %zmm10{%k3}            # c489
        vmovdqa64 %zmm0, -10368(%rbp)                           # c493
        vmovdqa64 %zmm8, -10880(%rbp)                           # c497
        kmov      %k4, %r8d                                     # c501
        kmov      %r15d, %k4                                    # c501
        vpadcd    _const_17(%rip), %k4, %zmm7{%k1}              # c505
        vpaddsetcd _const_19(%rip), %k4, %zmm9{%k3}             # c509
        vmovdqa64 %zmm7, -10816(%rbp)                           # c513
        kmov      %k6, %esi                                     # c517
        kmov      %k2, %edx                                     # c517
        vpaddsetcd _const_24(%rip), %k6, %zmm31{%k3}            # c521
        addl      %esi, %esi                                    # c525
        addl      %edx, %edx                                    # c525
        kmov      %esi, %k2                                     # c529
        kmov      %k4, %r10d                                    # c529
        vpadcd    _const_21(%rip), %k2, %zmm11{%k1}             # c533
        vpaddsetcd _const_23(%rip), %k2, %zmm13{%k3}            # c537
        vmovdqa64 %zmm11, -11072(%rbp)                          # c541
        kmov      %edx, %k4                                     # c545
        addl      %eax, %eax                                    # c545
        vpadcd    _const_20(%rip), %k4, %zmm10{%k1}             # c549
        vpaddsetcd _const_22(%rip), %k4, %zmm12{%k3}            # c553
        vmovdqa64 %zmm10, -11008(%rbp)                          # c557
        kmov      %eax, %k7                                     # c561
        addl      %r8d, %r8d                                    # c561
        vpadcd    _const_13(%rip), %k7, %zmm3{%k1}              # c565
        kmov      %r8d, %k7                                     # c569
        kmov      %k2, %r8d                                     # c569
        vpadcd    _const_16(%rip), %k7, %zmm6{%k1}              # c573
        vmovdqa64 %zmm3, -10560(%rbp)                           # c577
        vmovdqa64 %zmm6, -10752(%rbp)                           # c581
        addl      %r8d, %r8d                                    # c585
        kmov      %k4, %edi                                     # c585
        kmov      %r8d, %k4                                     # c589
        kmov      %k6, %r15d                                    # c589
        vpadcd    _const_23(%rip), %k4, %zmm13{%k1}             # c593
        vpaddsetcd _const_25(%rip), %k4, %zmm30{%k3}            # c597
        vmovdqa64 %zmm13, -11200(%rbp)                          # c601
        addl      %r15d, %r15d                                  # c605
        addl      %r10d, %r10d                                  # c605
        kmov      %r15d, %k2                                    # c609
        kmov      %r10d, %k7                                    # c609
        vpadcd    _const_24(%rip), %k2, %zmm31{%k1}             # c613
        vpadcd    _const_19(%rip), %k7, %zmm9{%k1}              # c617
        vmovdqa64 %zmm31, -11264(%rbp)                          # c621
        vmovdqa64 %zmm9, -10944(%rbp)                           # c625
        addl      %edi, %edi                                    # c629
        vmovaps   %zmm31, -6400(%rbp)                           # c629
        vmovaps   _const_5(%rip), %zmm31                        # c633
        vpermd    %zmm15, %zmm31, %zmm15                        # c637
        kmov      %k4, %r11d                                    # c637
        vpermd    %zmm16, %zmm31, %zmm16                        # c641
        kmov      %edi, %k7                                     # c641
        vpadcd    _const_22(%rip), %k7, %zmm12{%k1}             # c645
        vpermd    %zmm17, %zmm31, %zmm17                        # c649
        addl      %r11d, %r11d                                  # c649
        vpermd    %zmm18, %zmm31, %zmm18                        # c653
        kmov      %r11d, %k7                                    # c653
        vpadcd    _const_25(%rip), %k7, %zmm30{%k1}             # c657
        vpermd    %zmm19, %zmm31, %zmm19                        # c661
        movq      -10368(%rbp), %r15                            # c661
        vmovdqa64 %zmm30, -11328(%rbp)                          # c665
        vpermd    %zmm20, %zmm31, %zmm20                        # c669
        vmovaps   %zmm30, -6336(%rbp)                           # c669
        vpermd    %zmm21, %zmm31, %zmm21                        # c673
        movq      -10432(%rbp), %r8                             # c673
        vpermd    %zmm22, %zmm31, %zmm22                        # c677
        movq      -10560(%rbp), %r11                            # c677
        vpermd    %zmm23, %zmm31, %zmm23                        # c681
        movq      -10688(%rbp), %rdx                            # c681
        vpermd    %zmm24, %zmm31, %zmm24                        # c685
        movq      -10304(%rbp), %r10                            # c685
        vpermd    %zmm25, %zmm31, %zmm25                        # c689
        movq      -10624(%rbp), %rax                            # c689
        vpermd    %zmm26, %zmm31, %zmm26                        # c693
        lea       (%rcx,%r15,8), %r15                           # c693
        vpermd    %zmm27, %zmm31, %zmm27                        # c697
        lea       (%rcx,%r8,8), %r8                             # c697
        vpermd    %zmm28, %zmm31, %zmm28                        # c701
        movq      %r15, -1256(%rbp)                             # c701
        vpermd    %zmm29, %zmm31, %zmm29                        # c705
        movq      %r8, -1192(%rbp)                              # c705
        vmovdqa64 %zmm12, -11136(%rbp)                          # c709
        vpermd    -6528(%rbp), %zmm31, %zmm30                   # c713
        lea       (%rcx,%r11,8), %r8                            # c713
        vpermd    -6464(%rbp), %zmm31, %zmm31                   # c717
        lea       (%rcx,%rdx,8), %r15                           # c717
        movq      -10752(%rbp), %r11                            # c721
        lea       (%rcx,%r10,8), %r10                           # c721
        vmovaps   %zmm31, -6272(%rbp)                           # c725
        movq      -10816(%rbp), %rdx                            # c725
        vmovaps   _const_6(%rip), %zmm31                        # c729
        vpermd    %zmm14, %zmm31, %zmm15{%k5}                   # c733
        lea       (%rcx,%rax,8), %rax                           # c733
        vpermd    %zmm0, %zmm31, %zmm16{%k5}                    # c737
        movq      %r10, -1240(%rbp)                             # c737
        vpermd    %zmm1, %zmm31, %zmm17{%k5}                    # c741
        movq      %rax, -1248(%rbp)                             # c741
        vpermd    %zmm2, %zmm31, %zmm18{%k5}                    # c745
        movq      %r15, -1224(%rbp)                             # c745
        vpermd    %zmm3, %zmm31, %zmm19{%k5}                    # c749
        lea       (%rcx,%r11,8), %r10                           # c749
        vmovaps   _const_7(%rip), %zmm14                        # c753
        vpermd    %zmm15, %zmm14, %zmm1                         # c757
        lea       (%rcx,%rdx,8), %rax                           # c757
        vpermd    %zmm16, %zmm14, %zmm2                         # c761
        movq      -10880(%rbp), %r15                            # c761
        vpermd    %zmm4, %zmm31, %zmm20{%k5}                    # c765
        movq      %r10, -1264(%rbp)                             # c765
        vpermd    %zmm5, %zmm31, %zmm21{%k5}                    # c769
        movq      %rax, -1216(%rbp)                             # c769
        vpermd    %zmm6, %zmm31, %zmm22{%k5}                    # c773
        lea       (%rcx,%r15,8), %r11                           # c773
        vpermd    %zmm7, %zmm31, %zmm23{%k5}                    # c777
        movq      -10944(%rbp), %r10                            # c777
        vpermd    %zmm8, %zmm31, %zmm24{%k5}                    # c781
        movq      -11008(%rbp), %rdx                            # c781
        vpermd    %zmm9, %zmm31, %zmm25{%k5}                    # c785
        movq      -11072(%rbp), %rax                            # c785
        vpermd    %zmm10, %zmm31, %zmm26{%k5}                   # c789
        movq      %r11, -1200(%rbp)                             # c789
        vpermd    %zmm11, %zmm31, %zmm27{%k5}                   # c793
        lea       (%rcx,%r10,8), %r15                           # c793
        vpermd    %zmm12, %zmm31, %zmm28{%k5}                   # c797
        lea       (%rcx,%rdx,8), %r11                           # c797
        vpermd    %zmm13, %zmm31, %zmm29{%k5}                   # c801
        lea       (%rcx,%rax,8), %r10                           # c801
        vpermd    -6400(%rbp), %zmm31, %zmm30{%k5}              # c805
        movq      %r10, -1208(%rbp)                             # c805
        vmovaps   -6272(%rbp), %zmm0                            # c809
        vmovaps   %zmm1, -9920(%rbp)                            # c813
        vmovaps   %zmm2, -9856(%rbp)                            # c817
        vpermd    %zmm17, %zmm14, %zmm1                         # c821
        movq      -11136(%rbp), %rdx                            # c821
        vpermd    %zmm18, %zmm14, %zmm2                         # c825
        movq      -11200(%rbp), %rax                            # c825
        vpermd    %zmm19, %zmm14, %zmm3                         # c829
        movq      -10496(%rbp), %rdi                            # c829
        vpermd    %zmm20, %zmm14, %zmm4                         # c833
        lea       (%rcx,%rdx,8), %r10                           # c833
        vpermd    %zmm21, %zmm14, %zmm5                         # c837
        lea       (%rcx,%rax,8), %rdx                           # c837
        vpermd    %zmm22, %zmm14, %zmm6                         # c841
        movq      -11264(%rbp), %rax                            # c841
        vpermd    %zmm23, %zmm14, %zmm7                         # c845
        movq      -11328(%rbp), %rsi                            # c845
        vpermd    %zmm24, %zmm14, %zmm8                         # c849
        lea       (%rcx,%rdi,8), %rdi                           # c849
        vpermd    %zmm25, %zmm14, %zmm9                         # c853
        lea       (%rcx,%rax,8), %rax                           # c853
        vpermd    %zmm26, %zmm14, %zmm10                        # c857
        vpermd    %zmm27, %zmm14, %zmm11                        # c861
        vpermd    %zmm28, %zmm14, %zmm12                        # c865
        vpermd    %zmm29, %zmm14, %zmm13                        # c869
        vpermd    -6336(%rbp), %zmm31, %zmm0{%k5}               # c873
        vpermd    %zmm30, %zmm14, %zmm14                        # c877
                                # LOE rax rdx rsi rdi r8 r9 r10 r11 r12 r13 r14 r15 zmm0 zmm1 zmm2 zmm3 zmm4 zmm5 zmm6 zmm7 zmm8 zm
m9 zmm10 zmm11 zmm12 zmm13 zmm14 zmm15 zmm16 zmm17 zmm18 zmm19 zmm20 zmm21 zmm22 zmm23 zmm24 zmm25 zmm26 zmm27 zmm28 zmm29 zmm30 k1
 k3 k5
..B2.37:                        # Preds ..B2.9 Latency 165
        vmovaps   _const_7(%rip), %zmm31                        # c1
        vpsubd    %zmm1, %zmm17, %zmm17                         # c5
        movq      -1328(%rbp), %rcx                             # c5
        vpsubd    %zmm2, %zmm18, %zmm1                          # c9
        lea       (%rcx,%rsi,8), %rcx                           # c9
        vpsubd    %zmm6, %zmm22, %zmm2                          # c13
        movl      $1, %esi                                      # c13
        vpsubd    %zmm13, %zmm29, %zmm6                         # c17
        kmov      %esi, %k2                                     # c17
        vpermd    %zmm0, %zmm31, %zmm31                         # c21
        kxnor     %k4, %k4                                      # c21
        vpsubd    %zmm3, %zmm19, %zmm18                         # c25
        vpsubd    %zmm5, %zmm21, %zmm3                          # c29
        vpsubd    %zmm14, %zmm30, %zmm5                         # c33
        vpsubd    %zmm31, %zmm0, %zmm30                         # c37
        vpermf32x4 $238, %zmm6, %zmm31                          # c41
        vpsubd    %zmm4, %zmm20, %zmm4                          # c45
        vmovaps   %zmm31, -10048(%rbp)                          # c49
        vpermf32x4 $238, %zmm5, %zmm31                          # c53
        vpsubd    %zmm8, %zmm24, %zmm20                         # c57
        vmovaps   %zmm31, -10112(%rbp)                          # c61
        vpermf32x4 $238, %zmm20, %zmm0                          # c65
        vpermf32x4 $238, %zmm30, %zmm31                         # c69
        vpsubd    -9920(%rbp), %zmm15, %zmm15                   # c73
        vpsubd    -9856(%rbp), %zmm16, %zmm16                   # c77
        vpsubd    %zmm7, %zmm23, %zmm21                         # c81
        vpsubd    %zmm9, %zmm25, %zmm19                         # c85
        vpsubd    %zmm10, %zmm26, %zmm9                         # c89
        vpsubd    %zmm11, %zmm27, %zmm8                         # c93
        vpsubd    %zmm12, %zmm28, %zmm7                         # c97
        vmovaps   %zmm0, -9792(%rbp)                            # c101
        vpermf32x4 $238, -9984(%rbp), %zmm29                    # c105
        vpermf32x4 $238, %zmm15, %zmm28                         # c109
        vpermf32x4 $238, %zmm16, %zmm27                         # c113
        vpermf32x4 $238, %zmm17, %zmm26                         # c117
        vpermf32x4 $238, %zmm1, %zmm25                          # c121
        vpermf32x4 $238, %zmm18, %zmm24                         # c125
        vpermf32x4 $238, %zmm4, %zmm23                          # c129
        vpermf32x4 $238, %zmm3, %zmm22                          # c133
        vpermf32x4 $238, %zmm2, %zmm14                          # c137
        vpermf32x4 $238, %zmm21, %zmm13                         # c141
        vpermf32x4 $238, %zmm19, %zmm12                         # c145
        vpermf32x4 $238, %zmm9, %zmm11                          # c149
        vpermf32x4 $238, %zmm8, %zmm10                          # c153
        vpermf32x4 $238, %zmm7, %zmm0                           # c157
        vmovaps   %zmm31, -10176(%rbp)                          # c161
                                # LOE rax rdx rcx rdi r8 r9 r10 r11 r12 r13 r14 r15 zmm0 zmm1 zmm2 zmm3 zmm4 zmm5 zmm6 zmm7 zmm8 zm
m9 zmm10 zmm11 zmm12 zmm13 zmm14 zmm15 zmm16 zmm17 zmm18 zmm19 zmm20 zmm21 zmm22 zmm23 zmm24 zmm25 zmm26 zmm27 zmm28 zmm29 zmm30 k1
 k2 k3 k4 k5
..B2.41:                        # Preds ..B2.37 Latency 145
        vmovaps   %zmm23, -8256(%rbp)                           # c1
        vmovaps   %zmm24, -8320(%rbp)                           # c5
        vmovaps   %zmm25, -8384(%rbp)                           # c9
        vmovaps   %zmm26, -8448(%rbp)                           # c13
        vmovaps   %zmm27, -8512(%rbp)                           # c17
        vmovaps   %zmm28, -8576(%rbp)                           # c21
        vmovaps   %zmm29, -8640(%rbp)                           # c25
        vmovaps   %zmm30, -8704(%rbp)                           # c29
        vmovaps   %zmm5, -8768(%rbp)                            # c33
        vmovaps   %zmm6, -8832(%rbp)                            # c37
        vmovaps   %zmm7, -8896(%rbp)                            # c41
        vmovaps   %zmm8, -8960(%rbp)                            # c45
        vmovaps   %zmm9, -9024(%rbp)                            # c49
        vmovaps   %zmm19, -9088(%rbp)                           # c53
        vmovaps   %zmm20, -9152(%rbp)                           # c57
        vmovaps   %zmm21, -9216(%rbp)                           # c61
        vmovaps   %zmm2, -9280(%rbp)                            # c65
        vmovaps   %zmm3, -9344(%rbp)                            # c69
        vmovaps   %zmm4, -9408(%rbp)                            # c73
        vmovaps   %zmm18, -9472(%rbp)                           # c77
        vmovaps   %zmm1, -9536(%rbp)                            # c81
        vmovaps   %zmm17, -9600(%rbp)                           # c85
        vmovaps   %zmm16, -9664(%rbp)                           # c89
        vmovaps   %zmm15, -9728(%rbp)                           # c93
        vmovaps   %zmm0, -7808(%rbp)                            # c97
        vmovaps   %zmm10, -7872(%rbp)                           # c101
        vmovaps   %zmm11, -7936(%rbp)                           # c105
        vmovaps   %zmm12, -8000(%rbp)                           # c109
        vmovaps   %zmm13, -8064(%rbp)                           # c113
        vmovaps   %zmm14, -8128(%rbp)                           # c117
        vmovaps   %zmm22, -8192(%rbp)                           # c121
        movb      %dl, %dl                                      # c121
        movq      %rcx, -1168(%rbp)                             # c125
        movq      %rax, -1160(%rbp)                             # c125
        movq      %rdx, -1152(%rbp)                             # c129
        movq      %r10, -1144(%rbp)                             # c129
        movq      %r11, -1136(%rbp)                             # c133
        movq      %r15, -1128(%rbp)                             # c133
        movq      %r8, -1120(%rbp)                              # c137
        movq      %rdi, -1112(%rbp)                             # c137
        movq      %r12, -1104(%rbp)                             # c141
        movq      %r14, -432(%rbp)                              # c141
        movq      %r13, -448(%rbp)                              # c145
        jmp       ..B2.10       # Prob 100%                     # c145
                                # LOE r9 k1 k2 k3 k4
..B2.14:                        # Preds ..B2.13 Latency 1
..CL20:
..B2.10:                        # Preds ..B2.14 ..B2.41 Latency 1649
        vmovaps   -10176(%rbp), %zmm10                          # c1
        vmovaps   -8704(%rbp), %zmm4                            # c5
        vmovaps   -10112(%rbp), %zmm6                           # c9
        vmovaps   -8768(%rbp), %zmm26                           # c13
        vmovaps   -10048(%rbp), %zmm3                           # c17
        vmovaps   -8832(%rbp), %zmm2                            # c21
        vmovaps   -7808(%rbp), %zmm22                           # c25
        lea       (,%r9,8), %r11                                # c25
        vmovaps   -8896(%rbp), %zmm5                            # c29
        vmovaps   -7872(%rbp), %zmm18                           # c33
        lea       (%r11,%r11,2), %r12                           # c33
        vmovaps   -9088(%rbp), %zmm25                           # c37
        vmovaps   -9792(%rbp), %zmm28                           # c41
        vmovaps   -9152(%rbp), %zmm24                           # c45
        vmovaps   -8064(%rbp), %zmm15                           # c49
        movq      %r12, %rax                                    # c49
        vmovaps   -9216(%rbp), %zmm16                           # c53
        vmovaps   -8128(%rbp), %zmm27                           # c57
        lea       8(%r12), %r11                                 # c57
        vmovaps   -9280(%rbp), %zmm14                           # c61
        vcvtps2pd _const_0(%rip){1to8}, %zmm31{%k2}             # c65
        vcvtps2pd _const_1(%rip){1to8}, %zmm0{%k2}              # c69
        vcvtps2pd _const_2(%rip){1to8}, %zmm29{%k2}             # c73
        orq       $6, %rax                                      # c77
        lea       16(%r12), %r13                                # c77
        movq      %rax, -960(%rbp)                              # c81
        movq      %r11, %rax                                    # c81
        orq       $3, %rax                                      # c85
        movq      %r12, %rcx                                    # c85
        movq      %rax, -984(%rbp)                              # c89
        movq      %r11, %rax                                    # c89
        orq       $2, %rax                                      # c93
        orq       $5, %rcx                                      # c93
        movq      %rax, -1000(%rbp)                             # c97
        movq      %r13, %rax                                    # c97
        orq       $3, %rax                                      # c101
        movq      %rcx, -1056(%rbp)                             # c101
        movq      %rax, -1008(%rbp)                             # c105
        movq      %r13, %rax                                    # c105
        lea       12(%r12), %rcx                                # c109
        orq       $2, %rax                                      # c109
        movq      %rax, -1016(%rbp)                             # c113
        movq      %rcx, %rax                                    # c113
        movq      %r12, %rdx                                    # c117
        orq       $3, %rax                                      # c117
        orq       $7, %rdx                                      # c121
        movq      %rax, -1032(%rbp)                             # c121
        movq      %rcx, %rax                                    # c125
        movq      %rdx, -968(%rbp)                              # c125
        lea       20(%r12), %rdx                                # c129
        orq       $2, %rax                                      # c129
        movq      %rax, -992(%rbp)                              # c133
        movq      %rdx, %rax                                    # c133
        orq       $3, %rax                                      # c137
        kmov      %k4, %k5                                      # c137
        movq      %rax, -1024(%rbp)                             # c141
        movq      %rdx, %rax                                    # c141
        orq       $1, %rdx                                      # c145
        kmov      %k4, %k6                                      # c145
        movq      %rdx, -1080(%rbp)                             # c149
        kmov      %k4, %k7                                      # c149
        movq      -1168(%rbp), %rdx                             # c153
        movq      %r9, %r15                                     # c153
..L4:                                                           #
        vgatherdpd (%rdx,%zmm10,8), %zmm13{%k5}                 #
        jkzd      ..L3, %k5     # Prob 50%                      #
        vgatherdpd (%rdx,%zmm10,8), %zmm13{%k5}                 #
        jknzd     ..L4, %k5     # Prob 50%                      #
..L3:                                                           #
..L6:                                                           #
        vgatherdpd (%rdx,%zmm4,8), %zmm12{%k6}                  #
        jkzd      ..L5, %k6     # Prob 50%                      #
        vgatherdpd (%rdx,%zmm4,8), %zmm12{%k6}                  #
        jknzd     ..L6, %k6     # Prob 50%                      #
..L5:                                                           #
        shlq      $5, %r15                                      # c173
        kmov      %k4, %k5                                      # c173
        movq      -1160(%rbp), %rdx                             # c177
        kmov      %k4, %k6                                      # c177
..L8:                                                           #
        vgatherdpd (%rdx,%zmm6,8), %zmm11{%k7}                  #
        jkzd      ..L7, %k7     # Prob 50%                      #
        vgatherdpd (%rdx,%zmm6,8), %zmm11{%k7}                  #
        jknzd     ..L8, %k7     # Prob 50%                      #
..L7:                                                           #
..L10:                                                          #
        vgatherdpd (%rdx,%zmm26,8), %zmm10{%k5}                 #
        jkzd      ..L9, %k5     # Prob 50%                      #
        vgatherdpd (%rdx,%zmm26,8), %zmm10{%k5}                 #
        jknzd     ..L10, %k5    # Prob 50%                      #
..L9:                                                           #
        kmov      %k4, %k7                                      # c197
        orq       $2, %rax                                      # c197
        vmovaps   -8960(%rbp), %zmm26                           # c201
        movq      -1152(%rbp), %rdx                             # c205
        kmov      %k4, %k5                                      # c205
..L12:                                                          #
        vgatherdpd (%rdx,%zmm3,8), %zmm9{%k6}                   #
        jkzd      ..L11, %k6    # Prob 50%                      #
        vgatherdpd (%rdx,%zmm3,8), %zmm9{%k6}                   #
        jknzd     ..L12, %k6    # Prob 50%                      #
..L11:                                                          #
..L14:                                                          #
        vgatherdpd (%rdx,%zmm2,8), %zmm8{%k5}                   #
        jkzd      ..L13, %k5    # Prob 50%                      #
        vgatherdpd (%rdx,%zmm2,8), %zmm8{%k5}                   #
        jknzd     ..L14, %k5    # Prob 50%                      #
..L13:                                                          #
        vmovaps   -7936(%rbp), %zmm3                            # c225
        kmov      %k4, %k6                                      # c225
        vmovaps   -9024(%rbp), %zmm2                            # c229
        movq      -1144(%rbp), %rdx                             # c233
        kmov      %k4, %k5                                      # c233
..L16:                                                          #
        vgatherdpd (%rdx,%zmm22,8), %zmm7{%k7}                  #
        jkzd      ..L15, %k7    # Prob 50%                      #
        vgatherdpd (%rdx,%zmm22,8), %zmm7{%k7}                  #
        jknzd     ..L16, %k7    # Prob 50%                      #
..L15:                                                          #
..L18:                                                          #
        vgatherdpd (%rdx,%zmm5,8), %zmm6{%k5}                   #
        jkzd      ..L17, %k5    # Prob 50%                      #
        vgatherdpd (%rdx,%zmm5,8), %zmm6{%k5}                   #
        jknzd     ..L18, %k5    # Prob 50%                      #
..L17:                                                          #
        kmov      %k4, %k7                                      # c253
        movq      %r15, %r8                                     # c253
        movq      -1208(%rbp), %rdx                             # c257
        kmov      %k4, %k5                                      # c257
..L20:                                                          #
        vgatherdpd (%rdx,%zmm18,8), %zmm5{%k6}                  #
        jkzd      ..L19, %k6    # Prob 50%                      #
        vgatherdpd (%rdx,%zmm18,8), %zmm5{%k6}                  #
        jknzd     ..L20, %k6    # Prob 50%                      #
..L19:                                                          #
..L22:                                                          #
        vgatherdpd (%rdx,%zmm26,8), %zmm4{%k5}                  #
        jkzd      ..L21, %k5    # Prob 50%                      #
        vgatherdpd (%rdx,%zmm26,8), %zmm4{%k5}                  #
        jknzd     ..L22, %k5    # Prob 50%                      #
..L21:                                                          #
        vmovaps   -8000(%rbp), %zmm18                           # c277
        kmov      %k4, %k6                                      # c277
        movq      -1136(%rbp), %rdx                             # c281
        kmov      %k4, %k5                                      # c281
..L24:                                                          #
        vgatherdpd (%rdx,%zmm3,8), %zmm17{%k7}                  #
        jkzd      ..L23, %k7    # Prob 50%                      #
        vgatherdpd (%rdx,%zmm3,8), %zmm17{%k7}                  #
        jknzd     ..L24, %k7    # Prob 50%                      #
..L23:                                                          #
..L26:                                                          #
        vgatherdpd (%rdx,%zmm2,8), %zmm22{%k5}                  #
        jkzd      ..L25, %k5    # Prob 50%                      #
        vgatherdpd (%rdx,%zmm2,8), %zmm22{%k5}                  #
        jknzd     ..L26, %k5    # Prob 50%                      #
..L25:                                                          #
        vmovaps   -8192(%rbp), %zmm3                            # c301
        kmov      %k4, %k7                                      # c301
        vmovaps   -9344(%rbp), %zmm2                            # c305
        movq      -1128(%rbp), %rdx                             # c309
        kmov      %k4, %k5                                      # c309
..L28:                                                          #
        vgatherdpd (%rdx,%zmm18,8), %zmm1{%k6}                  #
        jkzd      ..L27, %k6    # Prob 50%                      #
        vgatherdpd (%rdx,%zmm18,8), %zmm1{%k6}                  #
        jknzd     ..L28, %k6    # Prob 50%                      #
..L27:                                                          #
..L30:                                                          #
        vgatherdpd (%rdx,%zmm25,8), %zmm20{%k5}                 #
        jkzd      ..L29, %k5    # Prob 50%                      #
        vgatherdpd (%rdx,%zmm25,8), %zmm20{%k5}                 #
        jknzd     ..L30, %k5    # Prob 50%                      #
..L29:                                                          #
        kmov      %k4, %k6                                      # c329
        vmovaps   %zmm1, -6208(%rbp)                            # c329
        vmovaps   -8256(%rbp), %zmm1                            # c333
        movq      -1200(%rbp), %rdx                             # c337
        vmovaps   %zmm20, -6144(%rbp)                           # c337
..L32:                                                          #
        vgatherdpd (%rdx,%zmm28,8), %zmm21{%k7}                 #
        jkzd      ..L31, %k7    # Prob 50%                      #
        vgatherdpd (%rdx,%zmm28,8), %zmm21{%k7}                 #
        jknzd     ..L32, %k7    # Prob 50%                      #
..L31:                                                          #
        vmovaps   -9408(%rbp), %zmm20                           # c349
        vmovaps   -8320(%rbp), %zmm28                           # c353
        kmov      %k4, %k5                                      # c357
        vmovaps   %zmm21, -6080(%rbp)                           # c357
..L34:                                                          #
        vgatherdpd (%rdx,%zmm24,8), %zmm30{%k5}                 #
        jkzd      ..L33, %k5    # Prob 50%                      #
        vgatherdpd (%rdx,%zmm24,8), %zmm30{%k5}                 #
        jknzd     ..L34, %k5    # Prob 50%                      #
..L33:                                                          #
        kmov      %k4, %k7                                      # c369
        vmovaps   %zmm30, -6016(%rbp)                           # c369
        vmovaps   -9472(%rbp), %zmm24                           # c373
        movq      -1216(%rbp), %rdx                             # c377
        kmov      %k4, %k5                                      # c377
..L36:                                                          #
        vgatherdpd (%rdx,%zmm15,8), %zmm19{%k6}                 #
        jkzd      ..L35, %k6    # Prob 50%                      #
        vgatherdpd (%rdx,%zmm15,8), %zmm19{%k6}                 #
        jknzd     ..L36, %k6    # Prob 50%                      #
..L35:                                                          #
..L38:                                                          #
        vgatherdpd (%rdx,%zmm16,8), %zmm23{%k5}                 #
        jkzd      ..L37, %k5    # Prob 50%                      #
        vgatherdpd (%rdx,%zmm16,8), %zmm23{%k5}                 #
        jknzd     ..L38, %k5    # Prob 50%                      #
..L37:                                                          #
        vmovaps   %zmm19, -5952(%rbp)                           # c397
        kmov      %k4, %k6                                      # c397
        vmovaps   -8384(%rbp), %zmm16                           # c401
        movq      -1264(%rbp), %rdx                             # c405
        vmovaps   %zmm23, -5888(%rbp)                           # c405
..L40:                                                          #
        vgatherdpd (%rdx,%zmm27,8), %zmm19{%k7}                 #
        jkzd      ..L39, %k7    # Prob 50%                      #
        vgatherdpd (%rdx,%zmm27,8), %zmm19{%k7}                 #
        jknzd     ..L40, %k7    # Prob 50%                      #
..L39:                                                          #
        kmov      %k4, %k5                                      # c417
        movq      %rax, -976(%rbp)                              # c417
..L42:                                                          #
        vgatherdpd (%rdx,%zmm14,8), %zmm26{%k5}                 #
        jkzd      ..L41, %k5    # Prob 50%                      #
        vgatherdpd (%rdx,%zmm14,8), %zmm26{%k5}                 #
        jknzd     ..L42, %k5    # Prob 50%                      #
..L41:                                                          #
        kmov      %k4, %k7                                      # c429
        movq      -1312(%rbp), %rax                             # c429
        vmovaps   -9536(%rbp), %zmm14                           # c433
        movq      -1224(%rbp), %rdx                             # c437
        vprefetch0 64(%rax,%r15)                                # c437
..L44:                                                          #
        vgatherdpd (%rdx,%zmm3,8), %zmm18{%k6}                  #
        jkzd      ..L43, %k6    # Prob 50%                      #
        vgatherdpd (%rdx,%zmm3,8), %zmm18{%k6}                  #
        jknzd     ..L44, %k6    # Prob 50%                      #
..L43:                                                          #
        kmov      %k4, %k5                                      # c449
        vprefetch1 128(%rax,%r15)                               # c449
..L46:                                                          #
        vgatherdpd (%rdx,%zmm2,8), %zmm25{%k5}                  #
        jkzd      ..L45, %k5    # Prob 50%                      #
        vgatherdpd (%rdx,%zmm2,8), %zmm25{%k5}                  #
        jknzd     ..L46, %k5    # Prob 50%                      #
..L45:                                                          #
        vmovaps   -8448(%rbp), %zmm3                            # c461
        vmovaps   -9600(%rbp), %zmm2                            # c465
        movq      -1248(%rbp), %rdx                             # c469
        kmov      %k4, %k5                                      # c469
..L48:                                                          #
        vgatherdpd (%rdx,%zmm1,8), %zmm30{%k7}                  #
        jkzd      ..L47, %k7    # Prob 50%                      #
        vgatherdpd (%rdx,%zmm1,8), %zmm30{%k7}                  #
        jknzd     ..L48, %k7    # Prob 50%                      #
..L47:                                                          #
..L50:                                                          #
        vgatherdpd (%rdx,%zmm20,8), %zmm23{%k5}                 #
        jkzd      ..L49, %k5    # Prob 50%                      #
        vgatherdpd (%rdx,%zmm20,8), %zmm23{%k5}                 #
        jknzd     ..L50, %k5    # Prob 50%                      #
..L49:                                                          #
        vmovaps   -8512(%rbp), %zmm1                            # c489
        movq      -1120(%rbp), %rdx                             # c493
        kmov      %k4, %k6                                      # c493
..L52:                                                          #
        vgatherdpd (%rdx,%zmm28,8), %zmm21{%k6}                 #
        jkzd      ..L51, %k6    # Prob 50%                      #
        vgatherdpd (%rdx,%zmm28,8), %zmm21{%k6}                 #
        jknzd     ..L52, %k6    # Prob 50%                      #
..L51:                                                          #
        kmov      %k4, %k5                                      # c505
        vmovaps   %zmm21, -5824(%rbp)                           # c505
..L54:                                                          #
        vgatherdpd (%rdx,%zmm24,8), %zmm15{%k5}                 #
        jkzd      ..L53, %k5    # Prob 50%                      #
        vgatherdpd (%rdx,%zmm24,8), %zmm15{%k5}                 #
        jknzd     ..L54, %k5    # Prob 50%                      #
..L53:                                                          #
        vmovaps   -9664(%rbp), %zmm21                           # c517
        movq      -1112(%rbp), %rdx                             # c521
        vmovaps   %zmm15, -5760(%rbp)                           # c521
        kmov      %k4, %k7                                      # c525
        kmov      %k4, %k5                                      # c525
..L56:                                                          #
        vgatherdpd (%rdx,%zmm16,8), %zmm27{%k7}                 #
        jkzd      ..L55, %k7    # Prob 50%                      #
        vgatherdpd (%rdx,%zmm16,8), %zmm27{%k7}                 #
        jknzd     ..L56, %k7    # Prob 50%                      #
..L55:                                                          #
..L58:                                                          #
        vgatherdpd (%rdx,%zmm14,8), %zmm20{%k5}                 #
        jkzd      ..L57, %k5    # Prob 50%                      #
        vgatherdpd (%rdx,%zmm14,8), %zmm20{%k5}                 #
        jknzd     ..L58, %k5    # Prob 50%                      #
..L57:                                                          #
        kmov      %k4, %k6                                      # c545
        vmovaps   %zmm27, -5696(%rbp)                           # c545
        vmovaps   -8576(%rbp), %zmm27                           # c549
        vmovaps   -9728(%rbp), %zmm14                           # c553
        movq      -1192(%rbp), %rdx                             # c557
        kmov      %k4, %k5                                      # c557
..L60:                                                          #
        vgatherdpd (%rdx,%zmm3,8), %zmm15{%k6}                  #
        jkzd      ..L59, %k6    # Prob 50%                      #
        vgatherdpd (%rdx,%zmm3,8), %zmm15{%k6}                  #
        jknzd     ..L60, %k6    # Prob 50%                      #
..L59:                                                          #
..L62:                                                          #
        vgatherdpd (%rdx,%zmm2,8), %zmm16{%k5}                  #
        jkzd      ..L61, %k5    # Prob 50%                      #
        vgatherdpd (%rdx,%zmm2,8), %zmm16{%k5}                  #
        jknzd     ..L62, %k5    # Prob 50%                      #
..L61:                                                          #
        kmov      %k4, %k7                                      # c577
        kmov      %k4, %k6                                      # c577
        movq      -1256(%rbp), %rdx                             # c581
        kmov      %k4, %k5                                      # c581
..L64:                                                          #
        vgatherdpd (%rdx,%zmm1,8), %zmm28{%k7}                  #
        jkzd      ..L63, %k7    # Prob 50%                      #
        vgatherdpd (%rdx,%zmm1,8), %zmm28{%k7}                  #
        jknzd     ..L64, %k7    # Prob 50%                      #
..L63:                                                          #
..L66:                                                          #
        vgatherdpd (%rdx,%zmm21,8), %zmm24{%k5}                 #
        jkzd      ..L65, %k5    # Prob 50%                      #
        vgatherdpd (%rdx,%zmm21,8), %zmm24{%k5}                 #
        jknzd     ..L66, %k5    # Prob 50%                      #
..L65:                                                          #
        kmov      %k4, %k7                                      # c601
        vmovaps   %zmm28, -5632(%rbp)                           # c601
        vmovaps   -9984(%rbp), %zmm28                           # c605
        movq      -1240(%rbp), %rdx                             # c609
        vmovaps   %zmm24, -5568(%rbp)                           # c609
..L68:                                                          #
        vgatherdpd (%rdx,%zmm27,8), %zmm3{%k6}                  #
        jkzd      ..L67, %k6    # Prob 50%                      #
        vgatherdpd (%rdx,%zmm27,8), %zmm3{%k6}                  #
        jknzd     ..L68, %k6    # Prob 50%                      #
..L67:                                                          #
        kmov      %k4, %k5                                      # c621
        vmovaps   %zmm3, -5504(%rbp)                            # c621
        vmovaps   -8640(%rbp), %zmm3                            # c625
..L70:                                                          #
        vgatherdpd (%rdx,%zmm14,8), %zmm2{%k5}                  #
        jkzd      ..L69, %k5    # Prob 50%                      #
        vgatherdpd (%rdx,%zmm14,8), %zmm2{%k5}                  #
        jknzd     ..L70, %k5    # Prob 50%                      #
..L69:                                                          #
        orq       $24, %r8                                      # c637
        vmovaps   %zmm2, -5440(%rbp)                            # c637
        vbroadcastsd (%r8,%rax), %zmm24                         # c641
        kmov      %k4, %k5                                      # c641
        movq      -1232(%rbp), %rdx                             # c645
        movq      %r15, %r14                                    # c645
..L72:                                                          #
        vgatherdpd (%rdx,%zmm3,8), %zmm1{%k7}                   #
        jkzd      ..L71, %k7    # Prob 50%                      #
        vgatherdpd (%rdx,%zmm3,8), %zmm1{%k7}                   #
        jknzd     ..L72, %k7    # Prob 50%                      #
..L71:                                                          #
..L74:                                                          #
        vgatherdpd (%rdx,%zmm28,8), %zmm21{%k5}                 #
        jkzd      ..L73, %k5    # Prob 50%                      #
        vgatherdpd (%rdx,%zmm28,8), %zmm21{%k5}                 #
        jknzd     ..L74, %k5    # Prob 50%                      #
..L73:                                                          #
        vmulpd    (%rax,%r15){1to8}, %zmm29, %zmm3{%k2}         # c665
        vmovaps   %zmm1, -5376(%rbp)                            # c665
        vsubpd    (%rax,%r15){1to8}, %zmm31, %zmm28{%k2}        # c669
        vmovaps   %zmm21, -5312(%rbp)                           # c669
        vmulpd    (%rax,%r15){1to8}, %zmm0, %zmm21{%k2}         # c673
        vmovaps   %zmm24, -6656(%rbp)                           # c673
        movq      %r15, %r10                                    # c677
        orq       $16, %r14                                     # c677
        vaddpd    (%r14,%rax){1to8}, %zmm31, %zmm27{%k2}        # c681
        orq       $8, %r10                                      # c681
        vsubpd    (%r14,%rax){1to8}, %zmm31, %zmm1{%k2}         # c685
        movq      %r12, %rsi                                    # c685
        vsubpd    (%r10,%rax){1to8}, %zmm28, %zmm28{%k2}        # c689
        movq      %r12, %rdi                                    # c689
        vmulpd    (%r10,%rax){1to8}, %zmm29, %zmm24{%k2}        # c693
        orq       $1, %rsi                                      # c693
        vmulpd    (%r10,%rax){1to8}, %zmm0, %zmm2{%k2}          # c697
        orq       $2, %rdi                                      # c697
        vmulpd    %zmm29, %zmm27, %zmm14{%k2}                   # c701
        movq      -1320(%rbp), %r8                              # c701
        vmulpd    %zmm0, %zmm27, %zmm27{%k2}                    # c705
        vprefetch0 192(%r8,%r12,8)                              # c705
        vmulpd    %zmm29, %zmm1, %zmm31{%k2}                    # c709
        vprefetch1 384(%r8,%r12,8)                              # c709
        vmulpd    %zmm0, %zmm1, %zmm1{%k2}                      # c713
        movq      %rsi, -952(%rbp)                              # c713
        vmulpd    %zmm29, %zmm28, %zmm29{%k2}                   # c717
        movq      %rdi, -944(%rbp)                              # c717
        vmulpd    %zmm0, %zmm28, %zmm28{%k2}                    # c721
        movq      %r12, %rdi                                    # c721
        vmovaps   _const_35(%rip), %zmm0                        # c725
        vpermd    %zmm24, %zmm0, %zmm24                         # c729
        movq      %r12, %rsi                                    # c729
        vpermd    %zmm2, %zmm0, %zmm2                           # c733
        orq       $3, %rdi                                      # c733
        vpermd    %zmm3, %zmm0, %zmm3                           # c737
        orq       $4, %rsi                                      # c737
        vpermd    %zmm21, %zmm0, %zmm21                         # c741
        orq       $1, %r11                                      # c741
        vpermd    %zmm14, %zmm0, %zmm14                         # c745
        orq       $1, %r13                                      # c745
        vpermd    %zmm27, %zmm0, %zmm27                         # c749
        orq       $1, %rcx                                      # c749
        vpermd    %zmm31, %zmm0, %zmm31                         # c753
        movq      -1056(%rbp), %rdx                             # c753
        vpermd    %zmm1, %zmm0, %zmm1                           # c757
        movq      -1080(%rbp), %rax                             # c757
        vmulpd    %zmm24, %zmm8, %zmm0                          # c761
        vmulpd    %zmm14, %zmm8, %zmm8                          # c765
        movb      %al, %al                                      # c765
        vmovaps   %zmm0, -5248(%rbp)                            # c769
        vmulpd    %zmm24, %zmm9, %zmm0                          # c773
        vmovaps   %zmm8, -3328(%rbp)                            # c773
        vmulpd    %zmm14, %zmm9, %zmm9                          # c777
        vmovaps   -5952(%rbp), %zmm8                            # c781
        vmovaps   %zmm0, -5184(%rbp)                            # c781
        vmulpd    %zmm24, %zmm10, %zmm0                         # c785
        vmovaps   %zmm9, -3264(%rbp)                            # c785
        vmulpd    %zmm14, %zmm10, %zmm10                        # c789
        vmovaps   -5888(%rbp), %zmm9                            # c793
        vmovaps   %zmm0, -5120(%rbp)                            # c793
        vmulpd    %zmm24, %zmm11, %zmm0                         # c797
        vmovaps   %zmm10, -3200(%rbp)                           # c797
        vmulpd    %zmm14, %zmm11, %zmm11                        # c801
        vmulpd    -5888(%rbp), %zmm27, %zmm10                   # c805
        vmovaps   %zmm0, -5056(%rbp)                            # c805
        vmulpd    %zmm24, %zmm12, %zmm0                         # c809
        vmovaps   %zmm11, -3136(%rbp)                           # c809
        vmulpd    %zmm24, %zmm13, %zmm24                        # c813
        vmovaps   %zmm10, -2944(%rbp)                           # c813
        vmulpd    %zmm14, %zmm12, %zmm12                        # c817
        vmovaps   %zmm0, -4992(%rbp)                            # c817
        vmulpd    %zmm2, %zmm23, %zmm0                          # c821
        vmovaps   %zmm24, -4928(%rbp)                           # c821
        vmulpd    %zmm2, %zmm30, %zmm24                         # c825
        vmovaps   %zmm12, -3072(%rbp)                           # c825
        vmulpd    %zmm14, %zmm13, %zmm13                        # c829
        vmovaps   %zmm0, -4864(%rbp)                            # c829
        vmulpd    %zmm2, %zmm25, %zmm0                          # c833
        vmovaps   %zmm24, -4800(%rbp)                           # c833
        vmulpd    %zmm2, %zmm18, %zmm24                         # c837
        vmovaps   %zmm13, -3008(%rbp)                           # c837
        vmulpd    %zmm31, %zmm23, %zmm23                        # c841
        vmovaps   %zmm0, -4736(%rbp)                            # c841
        vmulpd    %zmm2, %zmm26, %zmm0                          # c845
        vmovaps   %zmm24, -4672(%rbp)                           # c845
        vmulpd    %zmm2, %zmm19, %zmm2                          # c849
        vmulpd    %zmm3, %zmm17, %zmm24                         # c853
        vmovaps   %zmm0, -4608(%rbp)                            # c853
        vmulpd    %zmm3, %zmm22, %zmm0                          # c857
        vmovaps   %zmm2, -4544(%rbp)                            # c857
        vbroadcastsd (%r8,%r12,8), %zmm2                        # c861
        vmovaps   %zmm24, -4416(%rbp)                           # c861
        vmulpd    %zmm3, %zmm6, %zmm24                          # c865
        vmovaps   %zmm0, -4480(%rbp)                            # c865
        vmulpd    %zmm3, %zmm5, %zmm0                           # c869
        vmovaps   %zmm2, -6720(%rbp)                            # c869
        vmulpd    %zmm3, %zmm4, %zmm2                           # c873
        vmovaps   %zmm24, -4224(%rbp)                           # c873
        vmulpd    %zmm3, %zmm7, %zmm3                           # c877
        vmovaps   %zmm0, -4288(%rbp)                            # c877
        vmulpd    %zmm14, %zmm22, %zmm22                        # c881
        vmovaps   %zmm2, -4352(%rbp)                            # c881
        vmulpd    %zmm21, %zmm15, %zmm2                         # c885
        vmovaps   %zmm3, -4160(%rbp)                            # c885
        vmulpd    %zmm14, %zmm6, %zmm6                          # c889
        vmovaps   %zmm22, -3712(%rbp)                           # c889
        vmulpd    %zmm21, %zmm16, %zmm3                         # c893
        vmovaps   %zmm2, -4032(%rbp)                            # c893
        vmulpd    -5696(%rbp), %zmm21, %zmm24                   # c897
        vmovaps   %zmm6, -3456(%rbp)                            # c897
        vmulpd    -5760(%rbp), %zmm21, %zmm2                    # c901
        vmovaps   %zmm3, -4096(%rbp)                            # c901
        vmulpd    %zmm14, %zmm4, %zmm4                          # c905
        vmovaps   %zmm24, -3904(%rbp)                           # c905
        vmulpd    %zmm14, %zmm5, %zmm22                         # c909
        vmovaps   %zmm2, -3840(%rbp)                            # c909
        vmulpd    %zmm21, %zmm20, %zmm0                         # c913
        vmovaps   %zmm4, -3584(%rbp)                            # c913
        vmulpd    %zmm14, %zmm7, %zmm7                          # c917
        vmovaps   %zmm22, -3520(%rbp)                           # c917
        vmulpd    -6016(%rbp), %zmm27, %zmm5                    # c921
        vmovaps   %zmm0, -3968(%rbp)                            # c921
        vmulpd    -6208(%rbp), %zmm27, %zmm6                    # c925
        vmovaps   %zmm7, -3392(%rbp)                            # c925
        vmovaps   -5696(%rbp), %zmm3                            # c929
        vmovaps   %zmm5, -2816(%rbp)                            # c929
        vmovaps   -5760(%rbp), %zmm24                           # c933
        vmovaps   %zmm6, -2624(%rbp)                            # c933
        vmovaps   -5824(%rbp), %zmm2                            # c937
        vmulpd    -5824(%rbp), %zmm21, %zmm21                   # c941
        vmulpd    -5952(%rbp), %zmm27, %zmm22                   # c945
        vmulpd    -6144(%rbp), %zmm27, %zmm4                    # c949
        vmovaps   %zmm21, -3776(%rbp)                           # c949
        vmulpd    -6080(%rbp), %zmm27, %zmm0                    # c953
        vmovaps   %zmm22, -2880(%rbp)                           # c953
        vmulpd    %zmm31, %zmm16, %zmm5                         # c957
        vmovaps   %zmm4, -2688(%rbp)                            # c957
        vmulpd    %zmm31, %zmm15, %zmm6                         # c961
        vmovaps   %zmm0, -2752(%rbp)                            # c961
        vmulpd    %zmm31, %zmm20, %zmm7                         # c965
        vmulpd    %zmm31, %zmm3, %zmm16                         # c969
        vmulpd    %zmm31, %zmm24, %zmm15                        # c973
        vmulpd    %zmm31, %zmm2, %zmm20                         # c977
        vmulpd    %zmm31, %zmm30, %zmm30                        # c981
        vmulpd    %zmm31, %zmm25, %zmm25                        # c985
        vmulpd    %zmm31, %zmm18, %zmm18                        # c989
        vmulpd    %zmm31, %zmm26, %zmm26                        # c993
        vmulpd    %zmm31, %zmm19, %zmm31                        # c997
        vmulpd    %zmm14, %zmm17, %zmm17                        # c1001
        vmulpd    -5312(%rbp), %zmm1, %zmm4                     # c1005
        vmovaps   %zmm31, -2560(%rbp)                           # c1005
        vmulpd    -5376(%rbp), %zmm1, %zmm21                    # c1009
        vmovaps   %zmm17, -3648(%rbp)                           # c1009
        vmulpd    -5440(%rbp), %zmm1, %zmm22                    # c1013
        vmulpd    -5504(%rbp), %zmm1, %zmm27                    # c1017
        vmulpd    -5568(%rbp), %zmm1, %zmm2                     # c1021
        vmulpd    -5632(%rbp), %zmm1, %zmm14                    # c1025
        vmovaps   _const_35(%rip), %zmm1                        # c1029
        vmovaps   -6016(%rbp), %zmm10                           # c1033
        vmovaps   -6080(%rbp), %zmm11                           # c1037
        vmovaps   -6144(%rbp), %zmm13                           # c1041
        vmovaps   -6208(%rbp), %zmm12                           # c1045
        vmovaps   -5312(%rbp), %zmm19                           # c1049
        vmovaps   -5376(%rbp), %zmm24                           # c1053
        vmovaps   -5440(%rbp), %zmm3                            # c1057
        vmovaps   -5504(%rbp), %zmm31                           # c1061
        vmovaps   -5632(%rbp), %zmm0                            # c1065
        vpermd    %zmm29, %zmm1, %zmm29                         # c1069
        vpermd    %zmm28, %zmm1, %zmm28                         # c1073
        vmovaps   -5568(%rbp), %zmm17                           # c1077
        vmulpd    %zmm29, %zmm9, %zmm9                          # c1081
        vmulpd    %zmm29, %zmm8, %zmm8                          # c1085
        vmulpd    %zmm29, %zmm10, %zmm10                        # c1089
        vmulpd    %zmm29, %zmm11, %zmm11                        # c1093
        vmulpd    %zmm29, %zmm13, %zmm13                        # c1097
        vmulpd    %zmm29, %zmm12, %zmm29                        # c1101
        vmulpd    %zmm28, %zmm19, %zmm12                        # c1105
        vmulpd    %zmm28, %zmm24, %zmm19                        # c1109
        vmulpd    %zmm28, %zmm3, %zmm24                         # c1113
        vmulpd    %zmm28, %zmm31, %zmm3                         # c1117
        vmulpd    %zmm28, %zmm0, %zmm1                          # c1121
        vaddpd    %zmm5, %zmm4, %zmm5                           # c1125
        vaddpd    %zmm23, %zmm4, %zmm0                          # c1129
        vaddpd    %zmm25, %zmm22, %zmm25                        # c1133
        vaddpd    %zmm18, %zmm27, %zmm4                         # c1137
        vmovaps   -2944(%rbp), %zmm18                           # c1141
        vmulpd    %zmm28, %zmm17, %zmm17                        # c1145
        vaddpd    %zmm16, %zmm27, %zmm16                        # c1149
        vaddpd    %zmm15, %zmm2, %zmm15                         # c1153
        vaddpd    %zmm20, %zmm14, %zmm20                        # c1157
        vaddpd    %zmm26, %zmm2, %zmm28                         # c1161
        vaddpd    -2560(%rbp), %zmm14, %zmm27                   # c1165
        vaddpd    -3968(%rbp), %zmm24, %zmm2                    # c1169
        vaddpd    -3904(%rbp), %zmm3, %zmm24                    # c1173
        vaddpd    %zmm18, %zmm5, %zmm14                         # c1177
        vaddpd    -2816(%rbp), %zmm25, %zmm18                   # c1181
        vmovaps   -2688(%rbp), %zmm25                           # c1185
        vaddpd    %zmm6, %zmm21, %zmm6                          # c1189
        vaddpd    %zmm7, %zmm22, %zmm7                          # c1193
        vaddpd    %zmm30, %zmm21, %zmm31                        # c1197
        vaddpd    -4096(%rbp), %zmm12, %zmm23                   # c1201
        vaddpd    -4032(%rbp), %zmm19, %zmm26                   # c1205
        vaddpd    -3840(%rbp), %zmm17, %zmm19                   # c1209
        vaddpd    -3776(%rbp), %zmm1, %zmm3                     # c1213
        vaddpd    -2944(%rbp), %zmm0, %zmm22                    # c1217
        vmovaps   -2880(%rbp), %zmm5                            # c1221
        vmovaps   -2816(%rbp), %zmm0                            # c1225
        vmovaps   -2752(%rbp), %zmm1                            # c1229
        vaddpd    %zmm25, %zmm15, %zmm17                        # c1233
        vmovaps   -2624(%rbp), %zmm12                           # c1237
        vaddpd    -4672(%rbp), %zmm24, %zmm15                   # c1241
        vaddpd    -2880(%rbp), %zmm31, %zmm30                   # c1245
        vaddpd    %zmm5, %zmm6, %zmm5                           # c1249
        vaddpd    %zmm0, %zmm7, %zmm7                           # c1253
        vaddpd    -2752(%rbp), %zmm4, %zmm0                     # c1257
        vaddpd    %zmm1, %zmm16, %zmm1                          # c1261
        vaddpd    -2688(%rbp), %zmm28, %zmm31                   # c1265
        vaddpd    -2624(%rbp), %zmm27, %zmm25                   # c1269
        vaddpd    %zmm12, %zmm20, %zmm12                        # c1273
        vaddpd    -4864(%rbp), %zmm23, %zmm20                   # c1277
        vaddpd    -4800(%rbp), %zmm26, %zmm26                   # c1281
        vaddpd    -4736(%rbp), %zmm2, %zmm16                    # c1285
        vaddpd    -4544(%rbp), %zmm3, %zmm23                    # c1289
        vaddpd    %zmm11, %zmm15, %zmm11                        # c1293
        vaddpd    -4608(%rbp), %zmm19, %zmm21                   # c1297
        vaddpd    -3712(%rbp), %zmm14, %zmm6                    # c1301
        vaddpd    -3648(%rbp), %zmm5, %zmm5                     # c1305
        vaddpd    -3584(%rbp), %zmm7, %zmm4                     # c1309
        vaddpd    -3520(%rbp), %zmm1, %zmm3                     # c1313
        vaddpd    -3456(%rbp), %zmm17, %zmm2                    # c1317
        vaddpd    -3392(%rbp), %zmm12, %zmm1                    # c1321
        vaddpd    %zmm9, %zmm20, %zmm9                          # c1325
        vaddpd    %zmm8, %zmm26, %zmm8                          # c1329
        vaddpd    %zmm10, %zmm16, %zmm20                        # c1333
        vaddpd    %zmm29, %zmm23, %zmm28                        # c1337
        vaddpd    -3328(%rbp), %zmm22, %zmm10                   # c1341
        vaddpd    -3264(%rbp), %zmm30, %zmm26                   # c1345
        vaddpd    -3200(%rbp), %zmm18, %zmm12                   # c1349
        vaddpd    -3136(%rbp), %zmm0, %zmm18                    # c1353
        vaddpd    -3072(%rbp), %zmm31, %zmm22                   # c1357
        vaddpd    -3008(%rbp), %zmm25, %zmm16                   # c1361
        vaddpd    -4288(%rbp), %zmm11, %zmm11                   # c1365
        vaddpd    %zmm13, %zmm21, %zmm17                        # c1369
        vaddpd    -4480(%rbp), %zmm9, %zmm29                    # c1373
        vaddpd    -4416(%rbp), %zmm8, %zmm31                    # c1377
        vaddpd    -4352(%rbp), %zmm20, %zmm25                   # c1381
        vaddpd    -4160(%rbp), %zmm28, %zmm0                    # c1385
        vmulpd    %zmm12, %zmm6, %zmm30                         # c1389
        vmulpd    %zmm4, %zmm10, %zmm13                         # c1393
        vmulpd    %zmm18, %zmm5, %zmm8                          # c1397
        vmulpd    %zmm3, %zmm26, %zmm9                          # c1401
        vmulpd    %zmm22, %zmm6, %zmm15                         # c1405
        vmulpd    %zmm22, %zmm4, %zmm21                         # c1409
        vmulpd    %zmm2, %zmm10, %zmm19                         # c1413
        vmulpd    %zmm2, %zmm12, %zmm24                         # c1417
        vmulpd    %zmm16, %zmm5, %zmm28                         # c1421
        vmulpd    %zmm16, %zmm3, %zmm7                          # c1425
        vmulpd    %zmm1, %zmm26, %zmm14                         # c1429
        vmulpd    %zmm1, %zmm18, %zmm27                         # c1433
        vaddpd    -5056(%rbp), %zmm11, %zmm11                   # c1437
        vaddpd    -4224(%rbp), %zmm17, %zmm20                   # c1441
        vaddpd    -5184(%rbp), %zmm31, %zmm17                   # c1445
        vaddpd    -5120(%rbp), %zmm25, %zmm23                   # c1449
        vaddpd    -4928(%rbp), %zmm0, %zmm25                    # c1453
        vsubpd    %zmm13, %zmm30, %zmm31                        # c1457
        vsubpd    %zmm9, %zmm8, %zmm0                           # c1461
        vsubpd    %zmm15, %zmm19, %zmm8                         # c1465
        vsubpd    %zmm24, %zmm21, %zmm9                         # c1469
        vsubpd    %zmm28, %zmm14, %zmm13                        # c1473
        vmovaps   %zmm8, -2496(%rbp)                            # c1473
        vsubpd    %zmm27, %zmm7, %zmm15                         # c1477
        vmovaps   %zmm9, -2432(%rbp)                            # c1477
        vmulpd    %zmm11, %zmm16, %zmm7                         # c1481
        vmovaps   %zmm13, -2368(%rbp)                           # c1481
        vaddpd    -5248(%rbp), %zmm29, %zmm29                   # c1485
        vmovaps   %zmm15, -2304(%rbp)                           # c1485
        vaddpd    -4992(%rbp), %zmm20, %zmm20                   # c1489
        vmovaps   %zmm7, -2240(%rbp)                            # c1489
        vmulpd    %zmm29, %zmm22, %zmm28                        # c1493
        vmulpd    %zmm29, %zmm2, %zmm13                         # c1497
        vmulpd    %zmm29, %zmm12, %zmm30                        # c1501
        vmulpd    %zmm4, %zmm29, %zmm19                         # c1505
        vmulpd    %zmm17, %zmm16, %zmm21                        # c1509
        vmulpd    %zmm17, %zmm1, %zmm9                          # c1513
        vmulpd    %zmm17, %zmm18, %zmm29                        # c1517
        vmulpd    %zmm3, %zmm17, %zmm17                         # c1521
        vmulpd    %zmm23, %zmm22, %zmm8                         # c1525
        vmulpd    %zmm2, %zmm23, %zmm15                         # c1529
        vmulpd    %zmm23, %zmm6, %zmm22                         # c1533
        vmulpd    %zmm23, %zmm10, %zmm24                        # c1537
        vmulpd    %zmm1, %zmm11, %zmm27                         # c1541
        vmulpd    %zmm11, %zmm5, %zmm23                         # c1545
        vmulpd    %zmm11, %zmm26, %zmm7                         # c1549
        vmulpd    %zmm20, %zmm6, %zmm16                         # c1553
        vmulpd    %zmm20, %zmm4, %zmm11                         # c1557
        vmulpd    %zmm20, %zmm10, %zmm10                        # c1561
        vmulpd    %zmm20, %zmm12, %zmm12                        # c1565
        vmulpd    %zmm25, %zmm5, %zmm14                         # c1569
        vmulpd    %zmm25, %zmm3, %zmm20                         # c1573
        vmulpd    %zmm25, %zmm26, %zmm26                        # c1577
        vmulpd    %zmm25, %zmm18, %zmm25                        # c1581
        vsubpd    %zmm22, %zmm19, %zmm18                        # c1585
        vsubpd    %zmm13, %zmm16, %zmm16                        # c1589
        vsubpd    %zmm11, %zmm15, %zmm11                        # c1593
        vsubpd    %zmm10, %zmm28, %zmm13                        # c1597
        vsubpd    %zmm8, %zmm12, %zmm12                         # c1601
        vsubpd    %zmm9, %zmm14, %zmm10                         # c1605
        vsubpd    %zmm20, %zmm27, %zmm9                         # c1609
        vsubpd    %zmm26, %zmm21, %zmm8                         # c1613
        movq      $0, -1040(%rbp)                               # c1617
        vsubpd    %zmm30, %zmm24, %zmm19                        # c1621
        vsubpd    %zmm23, %zmm17, %zmm17                        # c1625
        vsubpd    %zmm29, %zmm7, %zmm7                          # c1629
        vmovaps   -2240(%rbp), %zmm20                           # c1633
        vmovaps   -2304(%rbp), %zmm21                           # c1637
        vmovaps   -2368(%rbp), %zmm22                           # c1641
        vmovaps   -2432(%rbp), %zmm14                           # c1645
        vmovaps   -2496(%rbp), %zmm15                           # c1649
                                # LOE rax rdx rcx rsi rdi r9 r11 r12 r13 xmm14 xmm15    ymm14 ymm15    zmm0 zmm1 zmm2 zmm3 zmm4 zmm
5 zmm6 zmm7 zmm8 zmm9 zmm10 zmm11 zmm12 zmm13 zmm14 zmm15 zmm16 zmm17 zmm18 zmm19 zmm20 zmm21 zmm22 zmm25 zmm31 k1 k2 k3 k4
..B2.38:                        # Preds ..B2.10 Latency 269
        vsubpd    %zmm20, %zmm25, %zmm20                        # c1
        vmulpd    %zmm13, %zmm4, %zmm4                          # c5
        vmulpd    %zmm12, %zmm6, %zmm28                         # c9
        vmulpd    %zmm19, %zmm2, %zmm30                         # c13
        vmulpd    %zmm7, %zmm1, %zmm24                          # c17
        vmulpd    %zmm8, %zmm3, %zmm27                          # c21
        vmulpd    %zmm20, %zmm5, %zmm3                          # c25
        vaddpd    %zmm4, %zmm28, %zmm1                          # c29
        vaddpd    %zmm27, %zmm3, %zmm3                          # c33
        vaddpd    %zmm30, %zmm1, %zmm28                         # c37
        vpxorq    %zmm2, %zmm2, %zmm2                           # c41
        vpbroadcastd _const_3(%rip), %zmm1                      # c45
        vmovapd   _const_34(%rip), %zmm27                       # c49
        vbroadcasti64x4 _const_3(%rip), %zmm29                  # c53
        vfixupnanpd %zmm1, %zmm27, %zmm2                        # c57
        vcmpeqpd  %zmm2, %zmm2, %k7                             # c61
        vcmpneqpd %zmm29{aaaa}, %zmm28, %k0                     # c65
        vmovapd   %zmm29{bbbb}, %zmm6                           # c69
        vmovaps   %zmm6, %zmm27                                 # c73
        kor       %k0, %k7                                      # c73
        vgetmantpd $0, %zmm28, %zmm27{%k7}                      # c77
        vbroadcasti32x4 _const_3(%rip), %zmm26                  # c81
        vcvtpd2ps {rz-sae}, %zmm27, %zmm23                      # c85
        vfixupnanpd %zmm1, %zmm28, %zmm2                        # c89
        vmovaps   %zmm26{bbbb}, %zmm1                           # c93
        vrcp23ps  %zmm23, %zmm1{%k7}                            # c97
        vgetexppd %zmm28, %zmm30                                # c101
        vcvtps2pd %zmm1, %zmm23                                 # c105
        vgetexppd _const_34(%rip), %zmm1                        # c109
        vbroadcasti32x4 _const_3(%rip), %zmm26                  # c113
        vcmpeqpd  %zmm2, %zmm2, %k5                             # c117
        vmovaps   %zmm28, %zmm5                                 # c121
        vpbroadcastd _const_3(%rip), %zmm25                     # c125
        vsubpd    {rn-sae}, %zmm30, %zmm1, %zmm2                # c129
        kmov      %k5, %k6                                      # c129
        vfixupnanpd %zmm25, %zmm28, %zmm5                       # c133
        kandn     %k4, %k6                                      # c133
        vpminsd   %zmm26{aaaa}, %zmm2, %zmm25                   # c137
        vpminud   %zmm26{bbbb}, %zmm25, %zmm30                  # c141
        vmovapd   %zmm29{cccc}, %zmm2                           # c145
        vaddpd    {rn-sae}, %zmm2, %zmm30, %zmm2                # c149
        vaddpd    %zmm24, %zmm3, %zmm3                          # c153
        vmovaps   %zmm23, %zmm24                                # c157
        vpslld    $20, %zmm2, %zmm26                            # c161
        vbroadcasti64x4 _const_3(%rip), %zmm30                  # c165
        vfnmadd213pd {rn-sae}, %zmm6, %zmm27, %zmm24            # c169
        vpsrad    $1, %zmm26, %zmm25                            # c173
        vcmplepd  %zmm29{dddd}, %zmm2, %k7{%k5}                 # c177
        vgetmantpd $0, _const_34(%rip), %zmm2                   # c181
        vfmadd231pd {rn-sae}, %zmm24, %zmm24, %zmm24            # c185
        vpandq    %zmm30{aaaa}, %zmm25, %zmm25                  # c189
        vmovdqa64 %zmm29{bbbb}, %zmm30                          # c193
        vmovapd   _const_34(%rip), %zmm29                       # c197
        vmulpd    {rn-sae}, %zmm23, %zmm2, %zmm4{%k5}           # c201
        vfmadd132pd {rn-sae}, %zmm23, %zmm23, %zmm24            # c205
        vmulpd    {rn}, %zmm5, %zmm29, %zmm4{%k6}               # c209
        vmovaps   %zmm2, %zmm5                                  # c213
        vfnmadd231pd {rn-sae}, %zmm4, %zmm27, %zmm5{%k5}        # c217
        vmovaps   %zmm4, %zmm23                                 # c221
        vfnmadd231pd {rn-sae}, %zmm24, %zmm27, %zmm6{%k5}       # c225
        vfmadd231pd {rn-sae}, %zmm24, %zmm5, %zmm23{%k5}        # c229
        vfmadd231pd {rn-sae}, %zmm24, %zmm6, %zmm24{%k5}        # c233
        vmovaps   %zmm2, %zmm6                                  # c237
        vpsubd    %zmm25, %zmm26, %zmm26                        # c241
        vfnmadd231pd {rn-sae}, %zmm23, %zmm27, %zmm6{%k5}       # c245
        vpaddd    %zmm30, %zmm26, %zmm26                        # c249
        vbroadcasti64x4 _const_3(%rip), %zmm30                  # c253
        vfmadd231pd %zmm24, %zmm6, %zmm23{%k5}                  # c257
        vpandq    %zmm30{dddd}, %zmm26, %zmm26                  # c261
        vpaddd    %zmm25, %zmm23, %zmm5                         # c265
        vmulpd    %zmm26, %zmm5, %zmm4{%k5}                     # c269
        jknzd     ..B2.30, %k7  # Prob 5%                       # c269
                                # LOE rax rdx rcx rsi rdi r9 r11 r12 r13 xmm14 xmm15   ymm14 ymm15   zmm0 zmm1 zmm2 zmm3 zmm4 zmm5 
zmm7 zmm8 zmm9 zmm10 zmm11 zmm12 zmm13 zmm14 zmm15 zmm16 zmm17 zmm18 zmm19 zmm20 zmm21 zmm22 zmm23 zmm26 zmm27 zmm28 zmm31 k1 k2 k3
 k4 k7
..B2.29:                        # Preds ..B2.32 ..B2.31 ..B2.38 Latency 221
        vmulpd    -6656(%rbp), %zmm28, %zmm5                    # c1
        vpxorq    %zmm26, %zmm26, %zmm26                        # c5
        vpbroadcastd _const_3(%rip), %zmm30                     # c9
        vmovapd   _const_34(%rip), %zmm28                       # c13
        vbroadcasti64x4 _const_3(%rip), %zmm24                  # c17
        vfixupnanpd %zmm30, %zmm28, %zmm26                      # c21
        vmovaps   %zmm5, -6592(%rbp)                            # c21
        vcmpeqpd  %zmm26, %zmm26, %k7                           # c25
        vcmpneqpd %zmm24{aaaa}, %zmm3, %k0                      # c29
        vmovaps   %zmm3, %zmm23                                 # c33
        vpbroadcastd _const_3(%rip), %zmm28                     # c37
        vmovapd   %zmm24{bbbb}, %zmm27                          # c41
        kor       %k0, %k7                                      # c41
        vfixupnanpd %zmm28, %zmm3, %zmm23                       # c45
        vmovaps   %zmm27, %zmm28                                # c49
        vgetmantpd $0, %zmm3, %zmm28{%k7}                       # c53
        vbroadcasti32x4 _const_3(%rip), %zmm25                  # c57
        vfixupnanpd %zmm30, %zmm3, %zmm26                       # c61
        vgetexppd %zmm3, %zmm30                                 # c65
        vcvtpd2ps {rz-sae}, %zmm28, %zmm6                       # c69
        vmovaps   %zmm25{bbbb}, %zmm29                          # c73
        vbroadcasti32x4 _const_3(%rip), %zmm25                  # c77
        vsubpd    {rn-sae}, %zmm30, %zmm1, %zmm1                # c81
        vrcp23ps  %zmm6, %zmm29{%k7}                            # c85
        vpminsd   %zmm25{aaaa}, %zmm1, %zmm1                    # c89
        vcvtps2pd %zmm29, %zmm6                                 # c93
        vpminud   %zmm25{bbbb}, %zmm1, %zmm29                   # c97
        vmovapd   %zmm24{cccc}, %zmm30                          # c101
        vaddpd    {rn-sae}, %zmm30, %zmm29, %zmm29              # c105
        vcmpeqpd  %zmm26, %zmm26, %k5                           # c109
        vmovaps   %zmm6, %zmm26                                 # c113
        vpslld    $20, %zmm29, %zmm25                           # c117
        kmov      %k5, %k6                                      # c117
        vbroadcasti64x4 _const_3(%rip), %zmm30                  # c121
        vfnmadd213pd {rn-sae}, %zmm27, %zmm28, %zmm26           # c125
        kandn     %k4, %k6                                      # c125
        vpsrad    $1, %zmm25, %zmm1                             # c129
        vfmadd231pd {rn-sae}, %zmm26, %zmm26, %zmm26            # c133
        vpandq    %zmm30{aaaa}, %zmm1, %zmm1                    # c137
        vmovdqa64 %zmm24{bbbb}, %zmm30                          # c141
        vcmplepd  %zmm24{dddd}, %zmm29, %k7{%k5}                # c145
        vmovapd   _const_34(%rip), %zmm24                       # c149
        vmulpd    {rn-sae}, %zmm6, %zmm2, %zmm5{%k5}            # c153
        vfmadd132pd {rn-sae}, %zmm6, %zmm6, %zmm26              # c157
        vmulpd    {rn}, %zmm23, %zmm24, %zmm5{%k6}              # c161
        vmovaps   %zmm2, %zmm23                                 # c165
        vfnmadd231pd {rn-sae}, %zmm5, %zmm28, %zmm23{%k5}       # c169
        vmovaps   %zmm5, %zmm6                                  # c173
        vfnmadd231pd {rn-sae}, %zmm26, %zmm28, %zmm27{%k5}      # c177
        vfmadd231pd {rn-sae}, %zmm26, %zmm23, %zmm6{%k5}        # c181
        vfmadd231pd {rn-sae}, %zmm26, %zmm27, %zmm26{%k5}       # c185
        vmovaps   %zmm2, %zmm27                                 # c189
        vpsubd    %zmm1, %zmm25, %zmm25                         # c193
        vfnmadd231pd {rn-sae}, %zmm6, %zmm28, %zmm27{%k5}       # c197
        vpaddd    %zmm30, %zmm25, %zmm25                        # c201
        vbroadcasti64x4 _const_3(%rip), %zmm30                  # c205
        vfmadd231pd %zmm26, %zmm27, %zmm6{%k5}                  # c209
        vpandq    %zmm30{dddd}, %zmm25, %zmm25                  # c213
        vpaddd    %zmm1, %zmm6, %zmm1                           # c217
        vmulpd    %zmm25, %zmm1, %zmm5{%k5}                     # c221
        jknzd     ..B2.34, %k7  # Prob 5%                       # c221
                                # LOE rax rdx rcx rsi rdi r9 r11 r12 r13 xmm14 xmm15   ymm14 ymm15   zmm0 zmm1 zmm2 zmm3 zmm4 zmm5 
zmm6 zmm7 zmm8 zmm9 zmm10 zmm11 zmm12 zmm13 zmm14 zmm15 zmm16 zmm17 zmm18 zmm19 zmm20 zmm21 zmm22 zmm25 zmm28 zmm31 k1 k2 k3 k4 k7
..B2.33:                        # Preds ..B2.36 ..B2.35 ..B2.29 Latency 73
        vmulpd    -6656(%rbp), %zmm3, %zmm24                    # c1
        vmulpd    %zmm4, %zmm16, %zmm23                         # c5
        vmulpd    %zmm4, %zmm11, %zmm16                         # c9
        vmulpd    %zmm4, %zmm31, %zmm6                          # c13
        vmulpd    %zmm4, %zmm15, %zmm3                          # c17
        vmulpd    %zmm4, %zmm14, %zmm2                          # c21
        vmulpd    %zmm4, %zmm18, %zmm29                         # c25
        vmulpd    %zmm4, %zmm19, %zmm11                         # c29
        vmulpd    %zmm4, %zmm13, %zmm19                         # c33
        vmulpd    %zmm4, %zmm12, %zmm18                         # c37
        vmulpd    %zmm5, %zmm10, %zmm15                         # c41
        vmulpd    %zmm5, %zmm9, %zmm14                          # c45
        vmulpd    %zmm5, %zmm0, %zmm13                          # c49
        vmulpd    %zmm5, %zmm22, %zmm12                         # c53
        vmulpd    %zmm5, %zmm21, %zmm10                         # c57
        vmulpd    %zmm5, %zmm17, %zmm9                          # c61
        vmulpd    %zmm5, %zmm7, %zmm1                           # c65
        vmulpd    %zmm5, %zmm8, %zmm4                           # c69
        vmulpd    %zmm5, %zmm20, %zmm0                          # c73
                                # LOE rax rdx rcx rsi rdi r9 r11 r12 r13 zmm0 zmm1 zmm2 zmm3 zmm4 zmm6 zmm9 zmm10 zmm11 zmm12 zmm13
 zmm14 zmm15 zmm16 zmm18 zmm19 zmm23 zmm24 zmm29 k1 k2 k3 k4
..B2.40:                        # Preds ..B2.33 Latency 81
        vmovaps   %zmm4, -7296(%rbp)                            # c1
        movq      -1040(%rbp), %r8                              # c1
        vmovaps   %zmm0, -7424(%rbp)                            # c5
        vmovaps   %zmm9, -6912(%rbp)                            # c9
        vmovaps   %zmm10, -7616(%rbp)                           # c13
        vmovaps   %zmm12, -7104(%rbp)                           # c17
        vmovaps   %zmm13, -6784(%rbp)                           # c21
        vmovaps   %zmm14, -7488(%rbp)                           # c25
        vmovaps   %zmm15, -7040(%rbp)                           # c29
        vmovaps   %zmm18, -7680(%rbp)                           # c33
        vmovaps   %zmm19, -7232(%rbp)                           # c37
        vmovaps   %zmm2, -7360(%rbp)                            # c41
        vmovaps   %zmm3, -6976(%rbp)                            # c45
        vmovaps   %zmm6, -6848(%rbp)                            # c49
        vmovaps   %zmm16, -7552(%rbp)                           # c53
        vmovaps   %zmm23, -7168(%rbp)                           # c57
        vmovaps   %zmm24, -7744(%rbp)                           # c61
        movb      %dl, %dl                                      # c61
        movq      %rax, -1080(%rbp)                             # c65
        movq      %rsi, -1096(%rbp)                             # c65
        movq      %rcx, -1064(%rbp)                             # c69
        movq      -944(%rbp), %rsi                              # c69
        movq      %r13, -1072(%rbp)                             # c73
        movq      %r11, -1088(%rbp)                             # c73
        movq      %rdx, -1056(%rbp)                             # c77
        movq      %rdi, -1048(%rbp)                             # c77
        movq      %r9, -1176(%rbp)                              # c81
                                # LOE rsi r8 r12 zmm1 zmm11 zmm29 k1 k2 k3 k4
..CL21:
..B2.11:                        # Preds ..B2.39 ..B2.40 Latency 1597
        vmovaps   -7104(%rbp), %zmm22                           # c1
        vmovaps   %zmm11, -2112(%rbp)                           # c1
        vmovaps   -6848(%rbp), %zmm23                           # c5
        vmovaps   %zmm29, -2048(%rbp)                           # c5
        vmovaps   -6784(%rbp), %zmm24                           # c9
        vmovaps   %zmm1, -2176(%rbp)                            # c9
        vmovaps   -6976(%rbp), %zmm21                           # c13
        movq      %r8, %r9                                      # c13
        vmovaps   -7360(%rbp), %zmm19                           # c17
        lea       (%r12,%r9,4), %r13                            # c17
        vmovaps   -7616(%rbp), %zmm20                           # c21
        movq      %r13, %rax                                    # c21
        vmovaps   -7680(%rbp), %zmm31                           # c25
        movq      %r13, %rdx                                    # c25
        vmovaps   -7488(%rbp), %zmm26                           # c29
        movq      %r13, %rcx                                    # c29
        vmovaps   -7168(%rbp), %zmm27                           # c33
        lea       16(%r12,%r9,4), %r14                          # c33
        vmovaps   -7040(%rbp), %zmm28                           # c37
        lea       8(%r12,%r9,4), %r15                           # c37
        vmovaps   -7232(%rbp), %zmm5                            # c41
        orq       $2, %rdx                                      # c41
        vmovaps   -7296(%rbp), %zmm16                           # c45
        orq       $1, %rcx                                      # c45
        vmovaps   -6912(%rbp), %zmm30                           # c49
        orq       $3, %rax                                      # c49
        vmovaps   -7552(%rbp), %zmm25                           # c53
        lea       (%r9,%r9), %rdi                               # c53
        movq      -1320(%rbp), %r11                             # c57
        lea       (%rdi,%r9,4), %r10                            # c57
        vbroadcastsd (%r11,%r13,8), %zmm8{%k2}                  # c61
        vprefetch0 (%r11,%r15,8)                                # c61
        vmulpd    (%r11,%rsi,8){1to8}, %zmm26, %zmm18           # c65
        vprefetch1 (%r11,%r14,8)                                # c65
        vmulpd    (%r11,%rsi,8){1to8}, %zmm29, %zmm17           # c69
        lea       1(%r9), %r8                                   # c69
        vmulpd    (%r11,%rsi,8){1to8}, %zmm28, %zmm3            # c73
        vmovaps   %zmm18, -1600(%rbp)                           # c73
        vmulpd    (%r11,%rsi,8){1to8}, %zmm30, %zmm4            # c77
        movq      %r10, %r9                                     # c77
        vmulpd    (%r11,%rsi,8){1to8}, %zmm25, %zmm15           # c81
        orq       $1, %r10                                      # c81
        vmulpd    (%r11,%rdx,8){1to8}, %zmm25, %zmm25           # c85
        movq      -1048(%rbp), %r13                             # c85
        vmulpd    (%r11,%r13,8){1to8}, %zmm22, %zmm0            # c89
        movq      -448(%rbp), %r14                              # c89
        vmulpd    (%r11,%r13,8){1to8}, %zmm23, %zmm14           # c93
        movq      -1104(%rbp), %r15                             # c93
        vmulpd    1688(%r15,%r14){1to8}, %zmm8, %zmm12{%k2}     # c97
        vmulpd    1680(%r15,%r14){1to8}, %zmm8, %zmm6{%k2}      # c101
        vmulpd    1672(%r15,%r14){1to8}, %zmm8, %zmm10{%k2}     # c105
        vmulpd    1664(%r15,%r14){1to8}, %zmm8, %zmm7{%k2}      # c109
        vmulpd    (%r11,%r13,8){1to8}, %zmm24, %zmm13           # c113
        vmovaps   %zmm0, -1792(%rbp)                            # c113
        vmulpd    (%r11,%r13,8){1to8}, %zmm21, %zmm9            # c117
        vmovaps   %zmm14, -1984(%rbp)                           # c117
        vmulpd    (%r11,%r13,8){1to8}, %zmm19, %zmm2            # c121
        vmovaps   %zmm13, -1920(%rbp)                           # c121
        vmulpd    (%r11,%r13,8){1to8}, %zmm20, %zmm8            # c125
        vmovaps   %zmm9, -1856(%rbp)                            # c125
        vmulpd    (%r11,%rdx,8){1to8}, %zmm26, %zmm26           # c129
        vmovaps   %zmm2, -1728(%rbp)                            # c129
        vmulpd    (%r11,%rsi,8){1to8}, %zmm27, %zmm2            # c133
        vmovaps   %zmm8, -1664(%rbp)                            # c133
        vmulpd    (%r11,%rdx,8){1to8}, %zmm27, %zmm27           # c137
        movq      -952(%rbp), %r13                              # c137
        vmulpd    (%r11,%r13,8){1to8}, %zmm31, %zmm0            # c141
        vmulpd    (%r11,%r13,8){1to8}, %zmm11, %zmm14           # c145
        vmulpd    (%r11,%r13,8){1to8}, %zmm5, %zmm8             # c149
        vmovaps   %zmm0, -1536(%rbp)                            # c149
        vmovaps   -7424(%rbp), %zmm0                            # c153
        vmulpd    (%r11,%r13,8){1to8}, %zmm16, %zmm9            # c157
        vmulpd    (%r11,%r13,8){1to8}, %zmm1, %zmm13            # c161
        vmulpd    (%r11,%r13,8){1to8}, %zmm0, %zmm18            # c165
        movq      -1184(%rbp), %r13                             # c165
        vmulpd    (%r11,%rdx,8){1to8}, %zmm28, %zmm28           # c169
        vmulpd    (%r11,%rcx,8){1to8}, %zmm31, %zmm31           # c173
        vmulpd    (%r11,%rcx,8){1to8}, %zmm0, %zmm0             # c177
        vmulpd    (%r11,%rcx,8){1to8}, %zmm5, %zmm5             # c181
        vmulpd    (%r11,%rcx,8){1to8}, %zmm16, %zmm16           # c185
        vmulpd    (%r11,%rdx,8){1to8}, %zmm29, %zmm29           # c189
        vmulpd    (%r11,%rcx,8){1to8}, %zmm11, %zmm11           # c193
        vaddpd    %zmm17, %zmm14, %zmm17                        # c197
        vaddpd    %zmm2, %zmm8, %zmm14                          # c201
        vaddpd    %zmm3, %zmm9, %zmm8                           # c205
        vmulpd    (%r11,%rax,8){1to8}, %zmm19, %zmm19           # c209
        vmulpd    (%r11,%rax,8){1to8}, %zmm20, %zmm20           # c213
        vmulpd    (%r11,%rax,8){1to8}, %zmm21, %zmm21           # c217
        vmulpd    (%r11,%rax,8){1to8}, %zmm22, %zmm22           # c221
        vmulpd    (%r11,%rdx,8){1to8}, %zmm30, %zmm30           # c225
        movq      -1056(%rbp), %rdx                             # c225
        vmulpd    (%r11,%rcx,8){1to8}, %zmm1, %zmm1             # c229
        movq      -960(%rbp), %rcx                              # c229
        vaddpd    %zmm4, %zmm13, %zmm4                          # c233
        vaddpd    -1536(%rbp), %zmm15, %zmm9                    # c237
        vaddpd    -1600(%rbp), %zmm18, %zmm3                    # c241
        vaddpd    %zmm25, %zmm31, %zmm15                        # c245
        vaddpd    %zmm26, %zmm0, %zmm31                         # c249
        vaddpd    %zmm27, %zmm5, %zmm13                         # c253
        vaddpd    %zmm28, %zmm16, %zmm18                        # c257
        vaddpd    %zmm29, %zmm11, %zmm11                        # c261
        vaddpd    -1856(%rbp), %zmm14, %zmm29                   # c265
        vaddpd    -1792(%rbp), %zmm8, %zmm14                    # c269
        vmulpd    (%r11,%rax,8){1to8}, %zmm23, %zmm23           # c273
        vmulpd    (%r11,%rax,8){1to8}, %zmm24, %zmm24           # c277
        vmovaps   %zmm14, -1472(%rbp)                           # c277
        vaddpd    %zmm30, %zmm1, %zmm1                          # c281
        movq      -1096(%rbp), %rax                             # c281
        vaddpd    -1728(%rbp), %zmm9, %zmm16                    # c285
        vaddpd    -1664(%rbp), %zmm3, %zmm9                     # c289
        vaddpd    %zmm19, %zmm15, %zmm3                         # c293
        vaddpd    %zmm20, %zmm31, %zmm15                        # c297
        vmovaps   %zmm9, -1408(%rbp)                            # c297
        vaddpd    %zmm21, %zmm13, %zmm19                        # c301
        vaddpd    %zmm22, %zmm18, %zmm18                        # c305
        vmovaps   _const_35(%rip), %zmm13                       # c309
        vaddpd    -1984(%rbp), %zmm17, %zmm5                    # c313
        vaddpd    -1920(%rbp), %zmm4, %zmm30                    # c317
        vaddpd    %zmm23, %zmm11, %zmm11                        # c321
        vaddpd    %zmm24, %zmm1, %zmm1                          # c325
        vpermd    %zmm12, %zmm13, %zmm28                        # c329
        vpermd    %zmm6, %zmm13, %zmm17                         # c333
        vpermd    %zmm10, %zmm13, %zmm4                         # c337
        vpermd    %zmm7, %zmm13, %zmm8                          # c341
        vmulpd    1280(%r15,%r14), %zmm3, %zmm2                 # c345
        vmulpd    896(%r15,%r14), %zmm3, %zmm13                 # c349
        vmulpd    512(%r15,%r14), %zmm3, %zmm24                 # c353
        vmulpd    128(%r15,%r14), %zmm3, %zmm12                 # c357
        vmulpd    1344(%r15,%r14), %zmm15, %zmm6                # c361
        vmulpd    960(%r15,%r14), %zmm15, %zmm10                # c365
        vmulpd    576(%r15,%r14), %zmm15, %zmm7                 # c369
        vmulpd    192(%r15,%r14), %zmm15, %zmm14                # c373
        vmulpd    1408(%r15,%r14), %zmm19, %zmm23               # c377
        vmulpd    1024(%r15,%r14), %zmm19, %zmm22               # c381
        vmulpd    640(%r15,%r14), %zmm19, %zmm21                # c385
        vmulpd    256(%r15,%r14), %zmm19, %zmm20                # c389
        vmulpd    1472(%r15,%r14), %zmm18, %zmm19               # c393
        vmulpd    1088(%r15,%r14), %zmm18, %zmm0                # c397
        vmulpd    704(%r15,%r14), %zmm18, %zmm31                # c401
        vmulpd    320(%r15,%r14), %zmm18, %zmm18                # c405
        vmulpd    1536(%r15,%r14), %zmm11, %zmm15               # c409
        vmulpd    1152(%r15,%r14), %zmm11, %zmm3                # c413
        vmulpd    768(%r15,%r14), %zmm11, %zmm9                 # c417
        vmulpd    384(%r15,%r14), %zmm11, %zmm11                # c421
        vmulpd    1600(%r15,%r14), %zmm1, %zmm27                # c425
        vmulpd    1216(%r15,%r14), %zmm1, %zmm26                # c429
        vmulpd    832(%r15,%r14), %zmm1, %zmm25                 # c433
        vmulpd    448(%r15,%r14), %zmm1, %zmm1                  # c437
        vaddpd    %zmm23, %zmm2, %zmm2                          # c441
        movq      -1032(%rbp), %r14                             # c441
        vaddpd    %zmm22, %zmm13, %zmm13                        # c445
        movq      -1016(%rbp), %r15                             # c445
        vaddpd    %zmm21, %zmm24, %zmm21                        # c449
        vaddpd    %zmm20, %zmm12, %zmm23                        # c453
        vaddpd    %zmm19, %zmm6, %zmm24                         # c457
        vaddpd    %zmm0, %zmm10, %zmm12                         # c461
        vaddpd    %zmm31, %zmm7, %zmm10                         # c465
        vaddpd    %zmm18, %zmm14, %zmm18                        # c469
        vaddpd    %zmm15, %zmm2, %zmm15                         # c473
        vaddpd    %zmm3, %zmm13, %zmm14                         # c477
        vaddpd    %zmm9, %zmm21, %zmm13                         # c481
        vaddpd    %zmm11, %zmm23, %zmm9                         # c485
        vaddpd    %zmm27, %zmm24, %zmm11                        # c489
        vaddpd    %zmm25, %zmm10, %zmm24                        # c493
        vaddpd    %zmm1, %zmm18, %zmm1                          # c497
        vaddpd    %zmm26, %zmm12, %zmm0                         # c501
        vaddpd    %zmm28, %zmm15, %zmm18                        # c505
        vaddpd    %zmm17, %zmm14, %zmm15                        # c509
        vaddpd    %zmm4, %zmm13, %zmm14                         # c513
        vaddpd    %zmm8, %zmm9, %zmm13                          # c517
        vaddpd    %zmm4, %zmm24, %zmm9                          # c521
        vaddpd    %zmm8, %zmm1, %zmm8                           # c525
        vaddpd    %zmm17, %zmm0, %zmm10                         # c529
        vmulpd    %zmm29, %zmm14, %zmm4                         # c533
        vmulpd    -1472(%rbp), %zmm9, %zmm25                    # c537
        vmulpd    -1408(%rbp), %zmm8, %zmm29                    # c541
        vaddpd    %zmm28, %zmm11, %zmm12                        # c545
        vmovaps   -6720(%rbp), %zmm23                           # c549
        vmulpd    %zmm16, %zmm13, %zmm2                         # c553
        vmulpd    %zmm30, %zmm10, %zmm16                        # c557
        vaddpd    %zmm25, %zmm29, %zmm22                        # c561
        vmulpd    %zmm5, %zmm15, %zmm31                         # c565
        vmulpd    %zmm23, %zmm12, %zmm21                        # c569
        vaddpd    %zmm4, %zmm2, %zmm28                          # c573
        vaddpd    %zmm16, %zmm22, %zmm27                        # c577
        vmulpd    -6720(%rbp), %zmm18, %zmm17                   # c581
        vaddpd    %zmm31, %zmm28, %zmm30                        # c585
        vaddpd    %zmm21, %zmm27, %zmm20                        # c589
        vaddpd    %zmm17, %zmm30, %zmm26                        # c593
        vmulpd    -7744(%rbp), %zmm20, %zmm6                    # c597
        shlq      $7, %r9                                       # c601
        vmovaps   -7424(%rbp), %zmm29                           # c605
        vmovaps   -2048(%rbp), %zmm25                           # c609
        vmovaps   -7168(%rbp), %zmm31                           # c613
        vmulpd    -6592(%rbp), %zmm26, %zmm5                    # c617
        vaddpd    2112(%r9,%r13), %zmm6, %zmm1                  # c621
        vmovaps   -7232(%rbp), %zmm3                            # c625
        vmovaps   -7296(%rbp), %zmm23                           # c629
        vmovdqa64 %zmm1, 2112(%r9,%r13)                         # c629
        vmovaps   -7680(%rbp), %zmm4                            # c633
        vmulpd    (%r11,%rdx,8){1to8}, %zmm29, %zmm19           # c637
        vmulpd    (%r11,%rcx,8){1to8}, %zmm25, %zmm20           # c641
        vmulpd    (%r11,%rcx,8){1to8}, %zmm31, %zmm29           # c645
        vmovaps   -7040(%rbp), %zmm17                           # c649
        vmovaps   -7552(%rbp), %zmm27                           # c653
        vmovaps   -6848(%rbp), %zmm25                           # c657
        vmovaps   -6976(%rbp), %zmm31                           # c661
        vmulpd    (%r11,%rax,8){1to8}, %zmm18, %zmm24           # c665
        vmulpd    (%r11,%rax,8){1to8}, %zmm12, %zmm21           # c669
        movq      -968(%rbp), %rax                              # c669
        vaddpd    2048(%r9,%r13), %zmm5, %zmm7                  # c673
        vmovaps   -2112(%rbp), %zmm11                           # c677
        vmovaps   -2176(%rbp), %zmm1                            # c681
        vmovdqa64 %zmm7, 2048(%r9,%r13)                         # c681
        vmulpd    (%r11,%rdx,8){1to8}, %zmm3, %zmm16            # c685
        vmulpd    (%r11,%rdx,8){1to8}, %zmm23, %zmm3            # c689
        vmulpd    (%r11,%rdx,8){1to8}, %zmm4, %zmm23            # c693
        vmovaps   -6912(%rbp), %zmm28                           # c697
        vmulpd    (%r11,%rcx,8){1to8}, %zmm17, %zmm4            # c701
        vmulpd    (%r11,%rcx,8){1to8}, %zmm27, %zmm6            # c705
        vmovaps   -7488(%rbp), %zmm26                           # c709
        vmulpd    (%r11,%rax,8){1to8}, %zmm25, %zmm17           # c713
        vmulpd    (%r11,%rax,8){1to8}, %zmm31, %zmm25           # c717
        vmovaps   -7104(%rbp), %zmm27                           # c721
        vmovaps   -7360(%rbp), %zmm31                           # c725
        vmulpd    (%r11,%rdx,8){1to8}, %zmm11, %zmm22           # c729
        vmulpd    (%r11,%rdx,8){1to8}, %zmm1, %zmm5             # c733
        movq      -984(%rbp), %rdx                              # c733
        vmulpd    (%r11,%rcx,8){1to8}, %zmm28, %zmm30           # c737
        vmulpd    (%r11,%rcx,8){1to8}, %zmm26, %zmm7            # c741
        movq      -1064(%rbp), %rcx                             # c741
        vmovaps   -6784(%rbp), %zmm28                           # c745
        vmulpd    (%r11,%rax,8){1to8}, %zmm27, %zmm26           # c749
        vmulpd    (%r11,%rax,8){1to8}, %zmm31, %zmm27           # c753
        vmovaps   -7616(%rbp), %zmm31                           # c757
        vmulpd    (%r11,%rax,8){1to8}, %zmm28, %zmm28           # c761
        vmulpd    (%r11,%rax,8){1to8}, %zmm31, %zmm31           # c765
        movq      -1000(%rbp), %rax                             # c765
        vaddpd    %zmm20, %zmm22, %zmm20                        # c769
        vaddpd    %zmm30, %zmm5, %zmm5                          # c773
        vaddpd    %zmm29, %zmm16, %zmm30                        # c777
        vaddpd    %zmm4, %zmm3, %zmm4                           # c781
        vaddpd    %zmm6, %zmm23, %zmm22                         # c785
        vaddpd    %zmm7, %zmm19, %zmm16                         # c789
        vaddpd    %zmm28, %zmm5, %zmm19                         # c793
        vaddpd    %zmm25, %zmm30, %zmm30                        # c797
        vaddpd    %zmm26, %zmm4, %zmm5                          # c801
        vaddpd    %zmm27, %zmm22, %zmm6                         # c805
        vaddpd    %zmm31, %zmm16, %zmm7                         # c809
        vaddpd    %zmm17, %zmm20, %zmm20                        # c813
        vmulpd    %zmm30, %zmm14, %zmm23                        # c817
        vmulpd    %zmm5, %zmm9, %zmm29                          # c821
        vmulpd    %zmm6, %zmm13, %zmm3                          # c825
        vmulpd    %zmm7, %zmm8, %zmm4                           # c829
        vmulpd    %zmm20, %zmm15, %zmm28                        # c833
        vmulpd    %zmm19, %zmm10, %zmm22                        # c837
        vaddpd    %zmm23, %zmm3, %zmm25                         # c841
        vaddpd    %zmm29, %zmm4, %zmm31                         # c845
        vaddpd    %zmm28, %zmm25, %zmm16                        # c849
        vaddpd    %zmm22, %zmm31, %zmm30                        # c853
        vmovaps   -6592(%rbp), %zmm2                            # c857
        vmovaps   -7744(%rbp), %zmm0                            # c861
        vaddpd    %zmm24, %zmm16, %zmm24                        # c865
        vaddpd    %zmm21, %zmm30, %zmm21                        # c869
        vmulpd    %zmm2, %zmm24, %zmm24                         # c873
        vmulpd    %zmm0, %zmm21, %zmm3                          # c877
        shlq      $7, %r10                                      # c881
        vaddpd    2048(%r10,%r13), %zmm24, %zmm23               # c885
        vaddpd    2112(%r10,%r13), %zmm3, %zmm4                 # c889
        vmovaps   -7296(%rbp), %zmm25                           # c893
        vmovdqa64 %zmm23, 2048(%r10,%r13)                       # c893
        vmovaps   -7680(%rbp), %zmm28                           # c897
        vmovdqa64 %zmm4, 2112(%r10,%r13)                        # c897
        vmovaps   -7424(%rbp), %zmm31                           # c901
        vmovaps   -2048(%rbp), %zmm22                           # c905
        vmovaps   -7040(%rbp), %zmm27                           # c909
        vmovaps   -7488(%rbp), %zmm26                           # c913
        vmovaps   -7232(%rbp), %zmm29                           # c917
        vmovaps   -6912(%rbp), %zmm16                           # c921
        vmovaps   -7168(%rbp), %zmm30                           # c925
        vmovaps   -7552(%rbp), %zmm21                           # c929
        vmulpd    (%r11,%rax,8){1to8}, %zmm26, %zmm6            # c933
        movq      -1088(%rbp), %r10                             # c933
        vmulpd    (%r11,%r10,8){1to8}, %zmm25, %zmm3            # c937
        vmulpd    (%r11,%r10,8){1to8}, %zmm28, %zmm17           # c941
        vmulpd    (%r11,%r10,8){1to8}, %zmm31, %zmm25           # c945
        vmulpd    (%r11,%rax,8){1to8}, %zmm22, %zmm28           # c949
        vmulpd    (%r11,%rax,8){1to8}, %zmm27, %zmm22           # c953
        vmovaps   -6784(%rbp), %zmm24                           # c957
        vmovaps   -6976(%rbp), %zmm31                           # c961
        vmovaps   -7104(%rbp), %zmm27                           # c965
        vmovaps   -7360(%rbp), %zmm26                           # c969
        vmulpd    (%r11,%r10,8){1to8}, %zmm11, %zmm23           # c973
        vmulpd    (%r11,%r10,8){1to8}, %zmm1, %zmm5             # c977
        vmulpd    (%r11,%r10,8){1to8}, %zmm29, %zmm29           # c981
        movq      -992(%rbp), %r10                              # c981
        vmulpd    (%r11,%rax,8){1to8}, %zmm16, %zmm16           # c985
        vmulpd    (%r11,%rax,8){1to8}, %zmm30, %zmm30           # c989
        vmulpd    (%r11,%rax,8){1to8}, %zmm21, %zmm4            # c993
        movq      -1008(%rbp), %rax                             # c993
        vmulpd    (%r11,%rdx,8){1to8}, %zmm24, %zmm21           # c997
        vmulpd    (%r11,%rdx,8){1to8}, %zmm31, %zmm24           # c1001
        vmulpd    (%r11,%rdx,8){1to8}, %zmm27, %zmm31           # c1005
        vmulpd    (%r11,%rdx,8){1to8}, %zmm26, %zmm27           # c1009
        vmovaps   -7616(%rbp), %zmm26                           # c1013
        vmovaps   -6848(%rbp), %zmm7                            # c1017
        vmulpd    (%r11,%rdx,8){1to8}, %zmm26, %zmm26           # c1021
        vaddpd    %zmm28, %zmm23, %zmm28                        # c1025
        vaddpd    %zmm16, %zmm5, %zmm16                         # c1029
        vaddpd    %zmm30, %zmm29, %zmm23                        # c1033
        vaddpd    %zmm22, %zmm3, %zmm3                          # c1037
        vaddpd    %zmm4, %zmm17, %zmm17                         # c1041
        vaddpd    %zmm6, %zmm25, %zmm5                          # c1045
        vmulpd    (%r11,%rdx,8){1to8}, %zmm7, %zmm7             # c1049
        vaddpd    %zmm24, %zmm23, %zmm24                        # c1053
        vaddpd    %zmm31, %zmm3, %zmm3                          # c1057
        vaddpd    %zmm27, %zmm17, %zmm23                        # c1061
        vaddpd    %zmm26, %zmm5, %zmm29                         # c1065
        vaddpd    %zmm7, %zmm28, %zmm6                          # c1069
        vaddpd    %zmm21, %zmm16, %zmm4                         # c1073
        vmulpd    %zmm24, %zmm14, %zmm28                        # c1077
        vmulpd    %zmm3, %zmm9, %zmm22                          # c1081
        vmulpd    %zmm23, %zmm13, %zmm25                        # c1085
        vmulpd    %zmm29, %zmm8, %zmm31                         # c1089
        vmulpd    %zmm6, %zmm15, %zmm30                         # c1093
        vmulpd    %zmm4, %zmm10, %zmm27                         # c1097
        vaddpd    %zmm28, %zmm25, %zmm16                        # c1101
        vaddpd    %zmm22, %zmm31, %zmm17                        # c1105
        vmulpd    64(%r11,%r12,8){1to8}, %zmm18, %zmm20         # c1109
        vmulpd    64(%r11,%r12,8){1to8}, %zmm12, %zmm19         # c1113
        vaddpd    %zmm30, %zmm16, %zmm21                        # c1117
        vaddpd    %zmm27, %zmm17, %zmm26                        # c1121
        vaddpd    %zmm20, %zmm21, %zmm20                        # c1125
        vaddpd    %zmm19, %zmm26, %zmm19                        # c1129
        vmovaps   -7680(%rbp), %zmm22                           # c1133
        vmovaps   -6912(%rbp), %zmm27                           # c1137
        vmulpd    %zmm2, %zmm20, %zmm24                         # c1141
        vmulpd    %zmm0, %zmm19, %zmm3                          # c1145
        vmulpd    (%r11,%rcx,8){1to8}, %zmm22, %zmm21           # c1149
        vmovaps   -2048(%rbp), %zmm30                           # c1153
        vmulpd    (%r11,%r10,8){1to8}, %zmm27, %zmm22           # c1157
        vmovaps   -7168(%rbp), %zmm26                           # c1161
        vmovaps   -7488(%rbp), %zmm6                            # c1165
        vmovaps   -6848(%rbp), %zmm7                            # c1169
        vmovaps   -6784(%rbp), %zmm27                           # c1173
        vaddpd    2304(%r9,%r13), %zmm24, %zmm23                # c1177
        vaddpd    2368(%r9,%r13), %zmm3, %zmm4                  # c1181
        vmovaps   -7232(%rbp), %zmm29                           # c1185
        vmovdqa64 %zmm23, 2304(%r9,%r13)                        # c1185
        vmovaps   -7296(%rbp), %zmm31                           # c1189
        vmovdqa64 %zmm4, 2368(%r9,%r13)                         # c1189
        vmovaps   -7424(%rbp), %zmm16                           # c1193
        vmulpd    (%r11,%r10,8){1to8}, %zmm30, %zmm17           # c1197
        vmulpd    (%r11,%r10,8){1to8}, %zmm26, %zmm30           # c1201
        vmovaps   -7040(%rbp), %zmm20                           # c1205
        vmovaps   -7552(%rbp), %zmm19                           # c1209
        vmulpd    (%r11,%r10,8){1to8}, %zmm6, %zmm3             # c1213
        vmulpd    (%r11,%r14,8){1to8}, %zmm7, %zmm6             # c1217
        vmulpd    (%r11,%r14,8){1to8}, %zmm27, %zmm7            # c1221
        vmovaps   -6976(%rbp), %zmm26                           # c1225
        vmovaps   -7360(%rbp), %zmm27                           # c1229
        vmulpd    (%r11,%rcx,8){1to8}, %zmm11, %zmm24           # c1233
        vmulpd    (%r11,%rcx,8){1to8}, %zmm1, %zmm5             # c1237
        vmulpd    (%r11,%rcx,8){1to8}, %zmm29, %zmm29           # c1241
        vmulpd    (%r11,%rcx,8){1to8}, %zmm31, %zmm23           # c1245
        vmulpd    (%r11,%rcx,8){1to8}, %zmm16, %zmm31           # c1249
        vmulpd    (%r11,%r10,8){1to8}, %zmm20, %zmm16           # c1253
        vmulpd    (%r11,%r10,8){1to8}, %zmm19, %zmm4            # c1257
        vmulpd    (%r11,%r14,8){1to8}, %zmm26, %zmm19           # c1261
        vmovaps   -7104(%rbp), %zmm20                           # c1265
        vmulpd    (%r11,%r14,8){1to8}, %zmm27, %zmm26           # c1269
        vmovaps   -7616(%rbp), %zmm27                           # c1273
        vmulpd    (%r11,%r14,8){1to8}, %zmm20, %zmm20           # c1277
        vmulpd    (%r11,%r14,8){1to8}, %zmm27, %zmm27           # c1281
        vaddpd    %zmm17, %zmm24, %zmm24                        # c1285
        vaddpd    %zmm22, %zmm5, %zmm22                         # c1289
        vaddpd    %zmm30, %zmm29, %zmm29                        # c1293
        vaddpd    %zmm16, %zmm23, %zmm5                         # c1297
        vaddpd    %zmm4, %zmm21, %zmm17                         # c1301
        vaddpd    %zmm3, %zmm31, %zmm21                         # c1305
        vaddpd    %zmm6, %zmm24, %zmm3                          # c1309
        vaddpd    %zmm7, %zmm22, %zmm4                          # c1313
        vaddpd    %zmm19, %zmm29, %zmm23                        # c1317
        vaddpd    %zmm20, %zmm5, %zmm5                          # c1321
        vaddpd    %zmm26, %zmm17, %zmm24                        # c1325
        vaddpd    %zmm27, %zmm21, %zmm29                        # c1329
        vmulpd    %zmm3, %zmm15, %zmm16                         # c1333
        vmulpd    %zmm4, %zmm10, %zmm17                         # c1337
        vmulpd    %zmm23, %zmm14, %zmm4                         # c1341
        vmulpd    %zmm5, %zmm9, %zmm31                          # c1345
        vmulpd    %zmm24, %zmm13, %zmm23                        # c1349
        vmulpd    %zmm29, %zmm8, %zmm3                          # c1353
        vaddpd    %zmm4, %zmm23, %zmm22                         # c1357
        vaddpd    %zmm31, %zmm3, %zmm30                         # c1361
        vmulpd    96(%r11,%r12,8){1to8}, %zmm18, %zmm28         # c1365
        vmulpd    96(%r11,%r12,8){1to8}, %zmm12, %zmm25         # c1369
        vaddpd    %zmm16, %zmm22, %zmm27                        # c1373
        vaddpd    %zmm17, %zmm30, %zmm21                        # c1377
        vaddpd    %zmm28, %zmm27, %zmm28                        # c1381
        vaddpd    %zmm25, %zmm21, %zmm25                        # c1385
        vmulpd    %zmm2, %zmm28, %zmm2                          # c1389
        vmulpd    %zmm0, %zmm25, %zmm0                          # c1393
        vaddpd    2432(%r9,%r13), %zmm2, %zmm2                  # c1397
        vaddpd    2496(%r9,%r13), %zmm0, %zmm0                  # c1401
        vmovaps   -7232(%rbp), %zmm29                           # c1405
        vmovdqa64 %zmm2, 2432(%r9,%r13)                         # c1405
        vmovaps   -7424(%rbp), %zmm26                           # c1409
        vmovdqa64 %zmm0, 2496(%r9,%r13)                         # c1409
        vmovaps   -2048(%rbp), %zmm20                           # c1413
        vmovaps   -6912(%rbp), %zmm19                           # c1417
        vmovaps   -7168(%rbp), %zmm5                            # c1421
        vmovapd   2624(%r9,%r13), %zmm4                         # c1425
        vmovapd   2560(%r9,%r13), %zmm3                         # c1429
        vmovaps   -7296(%rbp), %zmm25                           # c1433
        vmovaps   -7680(%rbp), %zmm28                           # c1437
        vmulpd    (%r11,%r15,8){1to8}, %zmm20, %zmm22           # c1441
        movq      -1072(%rbp), %r13                             # c1441
        vmulpd    (%r11,%r13,8){1to8}, %zmm11, %zmm24           # c1445
        vmulpd    (%r11,%r13,8){1to8}, %zmm1, %zmm23            # c1449
        vmulpd    (%r11,%r13,8){1to8}, %zmm29, %zmm29           # c1453
        vmulpd    (%r11,%r13,8){1to8}, %zmm26, %zmm31           # c1457
        vmulpd    (%r11,%r15,8){1to8}, %zmm19, %zmm16           # c1461
        vmulpd    (%r11,%r15,8){1to8}, %zmm5, %zmm30            # c1465
        vmovaps   -7040(%rbp), %zmm6                            # c1469
        vmovaps   -7552(%rbp), %zmm7                            # c1473
        vmovaps   -7488(%rbp), %zmm21                           # c1477
        vmovaps   -6848(%rbp), %zmm26                           # c1481
        vmovaps   -6784(%rbp), %zmm20                           # c1485
        vmovaps   -6976(%rbp), %zmm19                           # c1489
        vmulpd    (%r11,%r13,8){1to8}, %zmm25, %zmm25           # c1493
        vmulpd    (%r11,%r13,8){1to8}, %zmm28, %zmm28           # c1497
        vmulpd    (%r11,%r15,8){1to8}, %zmm6, %zmm17            # c1501
        vmulpd    (%r11,%r15,8){1to8}, %zmm7, %zmm27            # c1505
        vmulpd    (%r11,%r15,8){1to8}, %zmm21, %zmm21           # c1509
        vmulpd    (%r11,%rax,8){1to8}, %zmm26, %zmm26           # c1513
        vmulpd    (%r11,%rax,8){1to8}, %zmm20, %zmm20           # c1517
        vmulpd    (%r11,%rax,8){1to8}, %zmm19, %zmm19           # c1521
        vmovaps   -7104(%rbp), %zmm5                            # c1525
        vmovaps   -7360(%rbp), %zmm6                            # c1529
        vmovaps   -7616(%rbp), %zmm7                            # c1533
        vaddpd    %zmm22, %zmm24, %zmm24                        # c1537
        vaddpd    %zmm16, %zmm23, %zmm22                        # c1541
        vaddpd    %zmm30, %zmm29, %zmm23                        # c1545
        movl      %r8d, %edi                                    # c1549
        vmulpd    128(%r11,%r12,8){1to8}, %zmm18, %zmm2         # c1553
        vmulpd    128(%r11,%r12,8){1to8}, %zmm12, %zmm0         # c1557
        vmulpd    (%r11,%rax,8){1to8}, %zmm5, %zmm5             # c1561
        vmulpd    (%r11,%rax,8){1to8}, %zmm6, %zmm6             # c1565
        vmulpd    (%r11,%rax,8){1to8}, %zmm7, %zmm7             # c1569
        vaddpd    %zmm17, %zmm25, %zmm17                        # c1573
        vaddpd    %zmm27, %zmm28, %zmm16                        # c1577
        vaddpd    %zmm21, %zmm31, %zmm21                        # c1581
        vaddpd    %zmm26, %zmm24, %zmm24                        # c1585
        vaddpd    %zmm20, %zmm22, %zmm20                        # c1589
        vaddpd    %zmm19, %zmm23, %zmm19                        # c1593
        vmovaps   -2048(%rbp), %zmm29                           # c1597
                                # LOE rsi r8 r9 r12 edi xmm1 xmm11  ymm1 ymm11  zmm0 zmm1 zmm2 zmm3 zmm4 zmm5 zmm6 zmm7 zmm8 zmm9 z
mm10 zmm11 zmm12 zmm13 zmm14 zmm15 zmm16 zmm17 zmm18 zmm19 zmm20 zmm21 zmm24 zmm29 k1 k2 k3 k4
..B2.39:                        # Preds ..B2.11 Latency 341
        vaddpd    %zmm5, %zmm17, %zmm25                         # c1
        movq      -1184(%rbp), %r11                             # c1
        vaddpd    %zmm7, %zmm21, %zmm22                         # c5
        movq      -1320(%rbp), %rcx                             # c5
        vaddpd    %zmm6, %zmm16, %zmm23                         # c9
        movq      -1080(%rbp), %rax                             # c9
        vmulpd    %zmm20, %zmm10, %zmm5                         # c13
        movq      -976(%rbp), %rdx                              # c13
        vmulpd    %zmm19, %zmm14, %zmm21                        # c17
        movq      -1024(%rbp), %r10                             # c17
        vmulpd    %zmm25, %zmm9, %zmm19                         # c21
        cmpl      $6, %edi                                      # c21
        vmulpd    %zmm22, %zmm8, %zmm20                         # c25
        vmulpd    %zmm24, %zmm15, %zmm16                        # c29
        vmulpd    %zmm23, %zmm13, %zmm24                        # c33
        vaddpd    %zmm19, %zmm20, %zmm28                        # c37
        vaddpd    %zmm21, %zmm24, %zmm17                        # c41
        vaddpd    %zmm5, %zmm28, %zmm30                         # c45
        vaddpd    %zmm16, %zmm17, %zmm6                         # c49
        vaddpd    %zmm0, %zmm30, %zmm0                          # c53
        vaddpd    %zmm2, %zmm6, %zmm2                           # c57
        vmulpd    -7744(%rbp), %zmm0, %zmm7                     # c61
        vmulpd    -6592(%rbp), %zmm2, %zmm31                    # c65
        vaddpd    %zmm7, %zmm4, %zmm4                           # c69
        vmovaps   -7680(%rbp), %zmm22                           # c73
        vmovaps   -6912(%rbp), %zmm20                           # c77
        vmovdqa64 %zmm4, 2624(%r9,%r11)                         # c77
        vmovaps   -7488(%rbp), %zmm28                           # c81
        vaddpd    %zmm31, %zmm3, %zmm3                          # c85
        vmulpd    160(%rcx,%r12,8){1to8}, %zmm18, %zmm23        # c89
        vmulpd    160(%rcx,%r12,8){1to8}, %zmm12, %zmm4         # c93
        vmovdqa64 %zmm3, 2560(%r9,%r11)                         # c93
        vmovaps   -7232(%rbp), %zmm12                           # c97
        vmovaps   -7296(%rbp), %zmm18                           # c101
        vmulpd    (%rcx,%rax,8){1to8}, %zmm22, %zmm0            # c105
        vmovaps   -7424(%rbp), %zmm21                           # c109
        vmulpd    (%rcx,%rdx,8){1to8}, %zmm20, %zmm31           # c113
        vmovaps   -7168(%rbp), %zmm19                           # c117
        vmovaps   -7040(%rbp), %zmm17                           # c121
        vmovaps   -7552(%rbp), %zmm16                           # c125
        vmulpd    (%rcx,%rdx,8){1to8}, %zmm28, %zmm5            # c129
        vmovaps   -6848(%rbp), %zmm22                           # c133
        vmovaps   -6976(%rbp), %zmm20                           # c137
        vmovaps   -7104(%rbp), %zmm28                           # c141
        vmulpd    (%rcx,%rax,8){1to8}, %zmm11, %zmm26           # c145
        vmulpd    (%rcx,%rax,8){1to8}, %zmm12, %zmm12           # c149
        vmulpd    (%rcx,%rax,8){1to8}, %zmm18, %zmm3            # c153
        vmulpd    (%rcx,%rax,8){1to8}, %zmm21, %zmm2            # c157
        vmulpd    (%rcx,%rdx,8){1to8}, %zmm29, %zmm30           # c161
        vmulpd    (%rcx,%rdx,8){1to8}, %zmm19, %zmm18           # c165
        vmulpd    (%rcx,%rdx,8){1to8}, %zmm17, %zmm7            # c169
        vmulpd    (%rcx,%rdx,8){1to8}, %zmm16, %zmm6            # c173
        vmulpd    (%rcx,%r10,8){1to8}, %zmm22, %zmm16           # c177
        vmovaps   -6784(%rbp), %zmm21                           # c181
        vmulpd    (%rcx,%r10,8){1to8}, %zmm20, %zmm19           # c185
        vmulpd    (%rcx,%r10,8){1to8}, %zmm28, %zmm20           # c189
        vmovaps   -7360(%rbp), %zmm22                           # c193
        vmovaps   -7616(%rbp), %zmm28                           # c197
        vmulpd    (%rcx,%rax,8){1to8}, %zmm1, %zmm27            # c201
        vmulpd    (%rcx,%r10,8){1to8}, %zmm21, %zmm17           # c205
        vmulpd    (%rcx,%r10,8){1to8}, %zmm22, %zmm21           # c209
        vmulpd    (%rcx,%r10,8){1to8}, %zmm28, %zmm22           # c213
        vaddpd    %zmm30, %zmm26, %zmm28                        # c217
        vaddpd    %zmm18, %zmm12, %zmm26                        # c221
        vaddpd    %zmm7, %zmm3, %zmm3                           # c225
        vaddpd    %zmm6, %zmm0, %zmm0                           # c229
        vaddpd    %zmm5, %zmm2, %zmm2                           # c233
        vaddpd    %zmm31, %zmm27, %zmm27                        # c237
        vaddpd    %zmm19, %zmm26, %zmm7                         # c241
        vaddpd    %zmm20, %zmm3, %zmm12                         # c245
        vaddpd    %zmm21, %zmm0, %zmm18                         # c249
        vaddpd    %zmm22, %zmm2, %zmm26                         # c253
        vaddpd    %zmm16, %zmm28, %zmm5                         # c257
        vaddpd    %zmm17, %zmm27, %zmm6                         # c261
        vmulpd    %zmm7, %zmm14, %zmm14                         # c265
        vmulpd    %zmm12, %zmm9, %zmm9                          # c269
        vmulpd    %zmm18, %zmm13, %zmm13                        # c273
        vmulpd    %zmm26, %zmm8, %zmm8                          # c277
        vmulpd    %zmm5, %zmm15, %zmm15                         # c281
        vmulpd    %zmm6, %zmm10, %zmm0                          # c285
        vaddpd    %zmm14, %zmm13, %zmm10                        # c289
        vaddpd    %zmm9, %zmm8, %zmm7                           # c293
        vaddpd    %zmm15, %zmm10, %zmm2                         # c297
        vaddpd    %zmm0, %zmm7, %zmm3                           # c301
        vmovaps   -6592(%rbp), %zmm24                           # c305
        vmovaps   -7744(%rbp), %zmm25                           # c309
        vaddpd    %zmm23, %zmm2, %zmm5                          # c313
        vaddpd    %zmm4, %zmm3, %zmm4                           # c317
        vmulpd    %zmm24, %zmm5, %zmm6                          # c321
        vmulpd    %zmm25, %zmm4, %zmm8                          # c325
        vaddpd    2688(%r9,%r11), %zmm6, %zmm9                  # c329
        vaddpd    2752(%r9,%r11), %zmm8, %zmm10                 # c333
        movb      %al, %al                                      # c333
        vmovdqa64 %zmm9, 2688(%r9,%r11)                         # c337
        vmovdqa64 %zmm10, 2752(%r9,%r11)                        # c341
        jne       ..B2.11       # Prob 0%                       # c341
                                # LOE rsi r8 r12 zmm1 zmm11 zmm29 k1 k2 k3 k4
..B2.12:                        # Preds ..B2.39 Latency 1
        movq      -1176(%rbp), %r9                              # c1
                                # LOE r9 k1 k2 k3 k4
..CL22:
..B2.13:                        # Preds ..B2.12 Latency 5
        incq      %r9                                           # c1
        cmpl      $6, %r9d                                      # c5
        jne       ..B2.14       # Prob 100%                     # c5
                                # LOE r9 k1 k2 k3 k4
..CL23:
..B2.15:                        # Preds ..B2.13                 # Infreq Latency 1079
        movq      -1104(%rbp), %r12                             # c1
        movl      $255, %eax                                    # c1
        movq      -448(%rbp), %r13                              # c5
        kmov      %eax, %k5                                     # c5
        vmovaps   (%r12,%r13), %zmm0                            # c9
        movl      $1, %eax                                      # c9
        movl      80(%r12,%r13), %r14d                          # c13
        kmov      %eax, %k2                                     # c13
        imull     $42, %r14d, %edx                              # c17
        vpackstorelps %zmm0, -384(%rbp){%k2}                    # c27
        movq      1696(%r12,%r13), %r12                         # c27
        vmovaps   (%r12), %zmm1                                 # c31
        vmovaps   64(%r12), %zmm2                               # c35
        movl      %edx, %r12d                                   # c35
        addl      -384(%rbp), %r12d                             # c39
        movb      %al, %al                                      # c39
        movl      -384(%rbp), %r11d                             # c43
        movslq    %r12d, %r12                                   # c43
        lea       416(%rdx,%r11), %r10d                         # c47
        lea       544(%rdx,%r11), %r14d                         # c47
        movl      %r10d, -880(%rbp)                             # c51
        lea       400(%rdx,%r11), %r10d                         # c51
        movl      %r10d, -872(%rbp)                             # c55
        lea       384(%rdx,%r11), %r10d                         # c55
        movl      %r10d, -864(%rbp)                             # c59
        lea       368(%rdx,%r11), %r10d                         # c59
        movl      %r10d, -856(%rbp)                             # c63
        lea       352(%rdx,%r11), %r10d                         # c63
        movl      %r10d, -848(%rbp)                             # c67
        lea       336(%rdx,%r11), %r10d                         # c67
        movl      %r10d, -840(%rbp)                             # c71
        lea       320(%rdx,%r11), %r10d                         # c71
        movl      %r10d, -832(%rbp)                             # c75
        lea       304(%rdx,%r11), %r10d                         # c75
        movl      %r10d, -824(%rbp)                             # c79
        lea       288(%rdx,%r11), %r10d                         # c79
        movl      %r10d, -816(%rbp)                             # c83
        lea       272(%rdx,%r11), %r10d                         # c83
        movl      %r10d, -808(%rbp)                             # c87
        lea       256(%rdx,%r11), %r10d                         # c87
        movl      %r10d, -800(%rbp)                             # c91
        lea       240(%rdx,%r11), %r10d                         # c91
        movslq    %r14d, %r14                                   # c95
        movl      %r10d, -792(%rbp)                             # c95
        lea       224(%rdx,%r11), %r10d                         # c99
        movq      %r14, -680(%rbp)                              # c99
        lea       560(%rdx,%r11), %r13d                         # c103
        movl      %r10d, -784(%rbp)                             # c103
        lea       208(%rdx,%r11), %r10d                         # c107
        movslq    %r13d, %r13                                   # c107
        movl      -872(%rbp), %r14d                             # c111
        movl      %r10d, -776(%rbp)                             # c111
        movslq    %r14d, %r14                                   # c115
        lea       192(%rdx,%r11), %r10d                         # c115
        movq      %r14, -608(%rbp)                              # c119
        lea       528(%rdx,%r11), %r15d                         # c119
        movl      -840(%rbp), %r14d                             # c123
        movq      %r13, -688(%rbp)                              # c123
        movl      %r10d, -768(%rbp)                             # c127
        movslq    %r14d, %r14                                   # c127
        movl      -880(%rbp), %r13d                             # c131
        movq      %r14, -576(%rbp)                              # c131
        movslq    %r13d, %r13                                   # c135
        lea       176(%rdx,%r11), %r10d                         # c135
        movl      -768(%rbp), %r14d                             # c139
        movq      %r13, -616(%rbp)                              # c139
        movslq    %r14d, %r14                                   # c143
        movl      %r10d, -760(%rbp)                             # c143
        movq      %r14, -504(%rbp)                              # c147
        lea       160(%rdx,%r11), %r10d                         # c147
        movl      -848(%rbp), %r13d                             # c151
        movslq    %r15d, %r15                                   # c151
        movslq    %r13d, %r13                                   # c155
        movq      -432(%rbp), %r14                              # c155
        movq      %r13, -584(%rbp)                              # c159
        lea       512(%rdx,%r11), %eax                          # c159
        movl      -776(%rbp), %r13d                             # c163
        vpackstoreld %zmm1, (%r14,%r12,8)                       # c163
        movslq    %r13d, %r13                                   # c167
        vpackstoreld %zmm2, 64(%r14,%r12,8)                     # c167
        movq      %r13, -512(%rbp)                              # c171
        movq      -448(%rbp), %r13                              # c171
        vpackstorehd %zmm1, 64(%r14,%r12,8)                     # c175
        movslq    %eax, %rax                                    # c175
        vpackstorehd %zmm2, 128(%r14,%r12,8)                    # c179
        movb      %al, %al                                      # c179
        movq      -1104(%rbp), %r12                             # c183
        movl      %r10d, -752(%rbp)                             # c183
        lea       144(%rdx,%r11), %r10d                         # c187
        movq      %r15, -672(%rbp)                              # c187
        movl      %r10d, -744(%rbp)                             # c191
        lea       128(%rdx,%r11), %r10d                         # c191
        movl      -856(%rbp), %r15d                             # c195
        movl      %r10d, -736(%rbp)                             # c195
        movslq    %r15d, %r15                                   # c199
        lea       112(%rdx,%r11), %r10d                         # c199
        movq      %r15, -592(%rbp)                              # c203
        lea       496(%rdx,%r11), %r9d                          # c203
        movq      1704(%r12,%r13), %r15                         # c207
        movl      %r10d, -728(%rbp)                             # c207
        vmovaps   (%r15), %zmm3                                 # c211
        lea       96(%rdx,%r11), %r10d                          # c211
        vmovaps   64(%r15), %zmm4                               # c215
        movl      %r10d, -720(%rbp)                             # c215
        lea       80(%rdx,%r11), %r10d                          # c219
        lea       480(%rdx,%r11), %ecx                          # c219
        movl      %r10d, -712(%rbp)                             # c223
        lea       64(%rdx,%r11), %r10d                          # c223
        movl      %r10d, -704(%rbp)                             # c227
        lea       48(%rdx,%r11), %r10d                          # c227
        lea       464(%rdx,%r11), %esi                          # c231
        lea       448(%rdx,%r11), %edi                          # c231
        lea       432(%rdx,%r11), %r8d                          # c235
        movl      %r10d, -696(%rbp)                             # c235
        lea       32(%rdx,%r11), %r10d                          # c239
        lea       16(%rdx,%r11), %r11d                          # c239
        movslq    %r11d, %r11                                   # c243
        movslq    %r10d, %r10                                   # c243
        vpackstoreld %zmm3, (%r14,%r11,8)                       # c247
        movl      -864(%rbp), %edx                              # c247
        vpackstoreld %zmm4, 64(%r14,%r11,8)                     # c251
        movslq    %edx, %rdx                                    # c251
        vpackstorehd %zmm3, 64(%r14,%r11,8)                     # c255
        movslq    %r8d, %r8                                     # c255
        vpackstorehd %zmm4, 128(%r14,%r11,8)                    # c259
        movslq    %edi, %rdi                                    # c259
        movq      1712(%r12,%r13), %r11                         # c263
        movq      %rdx, -600(%rbp)                              # c263
        vmovaps   (%r11), %zmm5                                 # c267
        movq      %r8, -624(%rbp)                               # c267
        vmovaps   64(%r11), %zmm6                               # c271
        movq      %rdi, -632(%rbp)                              # c271
        vpackstoreld %zmm5, (%r14,%r10,8)                       # c275
        movl      -784(%rbp), %edx                              # c275
        vpackstoreld %zmm6, 64(%r14,%r10,8)                     # c279
        movslq    %edx, %rdx                                    # c279
        vpackstorehd %zmm5, 64(%r14,%r10,8)                     # c283
        movslq    %esi, %rsi                                    # c283
        vpackstorehd %zmm6, 128(%r14,%r10,8)                    # c287
        movslq    %ecx, %rcx                                    # c287
        movq      1720(%r12,%r13), %r10                         # c291
        movq      %rdx, -520(%rbp)                              # c291
        vmovaps   (%r10), %zmm7                                 # c295
        movq      %rsi, -640(%rbp)                              # c295
        vmovaps   64(%r10), %zmm8                               # c299
        movq      %rcx, -648(%rbp)                              # c299
        movslq    %r9d, %r9                                     # c303
        movq      %rax, -664(%rbp)                              # c303
        movq      %r9, -656(%rbp)                               # c307
        movl      -696(%rbp), %edx                              # c307
        movslq    %edx, %rdx                                    # c311
        movl      -792(%rbp), %r8d                              # c311
        vpackstoreld %zmm7, (%r14,%rdx,8)                       # c315
        movslq    %r8d, %r8                                     # c315
        vpackstoreld %zmm8, 64(%r14,%rdx,8)                     # c319
        movl      -800(%rbp), %edi                              # c319
        vpackstorehd %zmm7, 64(%r14,%rdx,8)                     # c323
        movslq    %edi, %rdi                                    # c323
        vpackstorehd %zmm8, 128(%r14,%rdx,8)                    # c327
        movb      %al, %al                                      # c327
        movq      1728(%r12,%r13), %rdx                         # c331
        movq      %r8, -528(%rbp)                               # c331
        vmovaps   (%rdx), %zmm9                                 # c335
        movq      %rdi, -536(%rbp)                              # c335
        vmovaps   64(%rdx), %zmm10                              # c339
        movb      %al, %al                                      # c339
        movl      -704(%rbp), %r8d                              # c343
        movl      -712(%rbp), %edi                              # c343
        movslq    %r8d, %r8                                     # c347
        movslq    %edi, %rdi                                    # c347
        vpackstoreld %zmm9, (%r14,%r8,8)                        # c351
        movl      -808(%rbp), %esi                              # c351
        vpackstoreld %zmm10, 64(%r14,%r8,8)                     # c355
        movslq    %esi, %rsi                                    # c355
        vpackstorehd %zmm9, 64(%r14,%r8,8)                      # c359
        vpackstorehd %zmm10, 128(%r14,%r8,8)                    # c363
        movb      %al, %al                                      # c363
        movq      1736(%r12,%r13), %r8                          # c367
        movq      %rsi, -544(%rbp)                              # c367
        vmovaps   (%r8), %zmm11                                 # c371
        vmovaps   64(%r8), %zmm12                               # c375
        movb      %al, %al                                      # c375
        vpackstoreld %zmm11, (%r14,%rdi,8)                      # c379
        movl      -720(%rbp), %esi                              # c379
        vpackstoreld %zmm12, 64(%r14,%rdi,8)                    # c383
        movslq    %esi, %rsi                                    # c383
        vpackstorehd %zmm11, 64(%r14,%rdi,8)                    # c387
        vpackstorehd %zmm12, 128(%r14,%rdi,8)                   # c391
        movb      %al, %al                                      # c391
        movq      1744(%r12,%r13), %rdi                         # c395
        movq      1784(%r12,%r13), %r11                         # c395
        vmovaps   (%rdi), %zmm13                                # c399
        vmovaps   64(%rdi), %zmm14                              # c403
        vmovaps   (%r11), %zmm23                                # c407
        vpackstoreld %zmm13, (%r14,%rsi,8)                      # c407
        vmovaps   64(%r11), %zmm24                              # c411
        vpackstoreld %zmm14, 64(%r14,%rsi,8)                    # c411
        vpackstorehd %zmm13, 64(%r14,%rsi,8)                    # c415
        vpackstorehd %zmm14, 128(%r14,%rsi,8)                   # c419
        movb      %al, %al                                      # c419
        movq      1752(%r12,%r13), %rsi                         # c423
        movq      1808(%r12,%r13), %rdi                         # c423
        vmovaps   (%rsi), %zmm15                                # c427
        vmovaps   64(%rsi), %zmm16                              # c431
        vmovaps   (%rdi), %zmm29                                # c435
        vmovaps   64(%rdi), %zmm30                              # c439
        movb      %al, %al                                      # c439
        movl      -816(%rbp), %ecx                              # c443
        movl      -824(%rbp), %r9d                              # c443
        movslq    %ecx, %rcx                                    # c447
        movslq    %r9d, %r9                                     # c447
        movq      %rcx, -552(%rbp)                              # c451
        movl      -728(%rbp), %ecx                              # c451
        movslq    %ecx, %rcx                                    # c455
        movq      %r9, -560(%rbp)                               # c455
        vpackstoreld %zmm15, (%r14,%rcx,8)                      # c459
        movl      -752(%rbp), %r9d                              # c459
        vpackstoreld %zmm16, 64(%r14,%rcx,8)                    # c463
        movslq    %r9d, %r9                                     # c463
        vpackstorehd %zmm15, 64(%r14,%rcx,8)                    # c467
        vpackstorehd %zmm16, 128(%r14,%rcx,8)                   # c471
        movb      %al, %al                                      # c471
        movq      1760(%r12,%r13), %rcx                         # c475
        movq      %r9, -488(%rbp)                               # c475
        vmovaps   (%rcx), %zmm17                                # c479
        vmovaps   64(%rcx), %zmm18                              # c483
        movb      %al, %al                                      # c483
        movl      -736(%rbp), %r9d                              # c487
        movl      -832(%rbp), %eax                              # c487
        movslq    %r9d, %r9                                     # c491
        movslq    %eax, %rax                                    # c491
        vpackstoreld %zmm17, (%r14,%r9,8)                       # c495
        movq      1800(%r12,%r13), %rcx                         # c495
        vmovaps   (%rcx), %zmm27                                # c499
        vpackstoreld %zmm18, 64(%r14,%r9,8)                     # c499
        vmovaps   64(%rcx), %zmm28                              # c503
        vpackstorehd %zmm17, 64(%r14,%r9,8)                     # c503
        vpackstorehd %zmm18, 128(%r14,%r9,8)                    # c507
        movb      %dl, %dl                                      # c507
        movq      1768(%r12,%r13), %r9                          # c511
        movq      %rax, -568(%rbp)                              # c511
        vmovaps   (%r9), %zmm19                                 # c515
        vmovaps   64(%r9), %zmm20                               # c519
        movb      %al, %al                                      # c519
        movl      -760(%rbp), %eax                              # c523
        movq      1816(%r12,%r13), %r9                          # c523
        vmovaps   (%r9), %zmm31                                 # c527
        movslq    %eax, %rax                                    # c527
        vmovaps   64(%r9), %zmm0                                # c531
        movq      %rax, -496(%rbp)                              # c531
        movl      -744(%rbp), %eax                              # c535
        movq      1824(%r12,%r13), %r11                         # c535
        vmovaps   (%r11), %zmm1                                 # c539
        movslq    %eax, %rax                                    # c539
        vmovaps   64(%r11), %zmm2                               # c543
        vpackstoreld %zmm19, (%r14,%rax,8)                      # c543
        vpackstoreld %zmm20, 64(%r14,%rax,8)                    # c547
        movq      1840(%r12,%r13), %rcx                         # c547
        vmovaps   (%rcx), %zmm5                                 # c551
        vpackstorehd %zmm19, 64(%r14,%rax,8)                    # c551
        vmovaps   64(%rcx), %zmm6                               # c555
        vpackstorehd %zmm20, 128(%r14,%rax,8)                   # c555
        movq      1776(%r12,%r13), %rax                         # c559
        movq      1848(%r12,%r13), %rdi                         # c559
        vmovaps   (%rax), %zmm21                                # c563
        vmovaps   64(%rax), %zmm22                              # c567
        vmovaps   (%rdi), %zmm7                                 # c571
        vmovaps   64(%rdi), %zmm8                               # c575
        movb      %al, %al                                      # c575
        movq      1792(%r12,%r13), %rax                         # c579
        movq      1864(%r12,%r13), %r11                         # c579
        vmovaps   (%rax), %zmm25                                # c583
        vmovaps   64(%rax), %zmm26                              # c587
        vmovaps   (%r11), %zmm11                                # c591
        vmovaps   64(%r11), %zmm12                              # c595
        movb      %al, %al                                      # c595
        movq      1832(%r12,%r13), %rax                         # c599
        movq      1856(%r12,%r13), %r9                          # c599
        vmovaps   (%rax), %zmm3                                 # c603
        vmovaps   64(%rax), %zmm4                               # c607
        vmovaps   (%r9), %zmm9                                  # c611
        vmovaps   64(%r9), %zmm10                               # c615
        movb      %al, %al                                      # c615
        movq      -488(%rbp), %r10                              # c619
        movq      1872(%r12,%r13), %rax                         # c619
        vmovaps   (%rax), %zmm13                                # c623
        vpackstoreld %zmm21, (%r14,%r10,8)                      # c623
        vmovaps   64(%rax), %zmm14                              # c627
        vpackstoreld %zmm22, 64(%r14,%r10,8)                    # c627
        movq      1880(%r12,%r13), %rcx                         # c631
        vpackstorehd %zmm21, 64(%r14,%r10,8)                    # c631
        vmovaps   (%rcx), %zmm15                                # c635
        vpackstorehd %zmm22, 128(%r14,%r10,8)                   # c635
        vmovaps   64(%rcx), %zmm16                              # c639
        movb      %al, %al                                      # c639
        movq      1888(%r12,%r13), %rdi                         # c643
        movq      1896(%r12,%r13), %r9                          # c643
        vmovaps   (%rdi), %zmm17                                # c647
        vmovaps   64(%rdi), %zmm18                              # c651
        vmovaps   (%r9), %zmm19                                 # c655
        vmovaps   64(%r9), %zmm20                               # c659
        movb      %al, %al                                      # c659
        movq      -496(%rbp), %r15                              # c663
        movq      -504(%rbp), %rdx                              # c663
        movq      -512(%rbp), %rsi                              # c667
        vpackstoreld %zmm23, (%r14,%r15,8)                      # c667
        movq      -520(%rbp), %r8                               # c671
        vpackstoreld %zmm24, 64(%r14,%r15,8)                    # c671
        movq      -528(%rbp), %r10                              # c675
        vpackstoreld %zmm25, (%r14,%rdx,8)                      # c675
        vpackstoreld %zmm26, 64(%r14,%rdx,8)                    # c679
        movq      1904(%r12,%r13), %r11                         # c679
        vmovaps   (%r11), %zmm21                                # c683
        vpackstoreld %zmm27, (%r14,%rsi,8)                      # c683
        vmovaps   64(%r11), %zmm22                              # c687
        vpackstoreld %zmm28, 64(%r14,%rsi,8)                    # c687
        vpackstoreld %zmm29, (%r14,%r8,8)                       # c691
        movq      1912(%r12,%r13), %rax                         # c691
        vpackstoreld %zmm30, 64(%r14,%r8,8)                     # c695
        movq      1920(%r12,%r13), %rcx                         # c695
        vpackstoreld %zmm31, (%r14,%r10,8)                      # c699
        movq      1928(%r12,%r13), %rdi                         # c699
        vpackstoreld %zmm0, 64(%r14,%r10,8)                     # c703
        movq      64(%r12,%r13), %r9                            # c703
        vpackstorehd %zmm23, 64(%r14,%r15,8)                    # c707
        vmovaps   (%rax), %zmm23                                # c711
        vpackstorehd %zmm24, 128(%r14,%r15,8)                   # c711
        vmovaps   64(%rax), %zmm24                              # c715
        vpackstorehd %zmm25, 64(%r14,%rdx,8)                    # c715
        vmovaps   (%rcx), %zmm25                                # c719
        vpackstorehd %zmm26, 128(%r14,%rdx,8)                   # c719
        vmovaps   64(%rcx), %zmm26                              # c723
        vpackstorehd %zmm27, 64(%r14,%rsi,8)                    # c723
        vmovaps   (%rdi), %zmm27                                # c727
        vpackstorehd %zmm28, 128(%r14,%rsi,8)                   # c727
        vmovaps   64(%rdi), %zmm28                              # c731
        vpackstorehd %zmm29, 64(%r14,%r8,8)                     # c731
        vmovaps   (%r9), %zmm29                                 # c735
        vpackstorehd %zmm30, 128(%r14,%r8,8)                    # c735
        vmovaps   64(%r9), %zmm30                               # c739
        vpackstorehd %zmm31, 64(%r14,%r10,8)                    # c739
        vpackstorehd %zmm0, 128(%r14,%r10,8)                    # c743
        movb      %al, %al                                      # c743
        movq      -536(%rbp), %r15                              # c747
        movq      -544(%rbp), %rdx                              # c747
        movq      -552(%rbp), %rsi                              # c751
        vpackstoreld %zmm1, (%r14,%r15,8)                       # c751
        movq      -560(%rbp), %r8                               # c755
        vpackstoreld %zmm2, 64(%r14,%r15,8)                     # c755
        movq      -568(%rbp), %r10                              # c759
        vpackstoreld %zmm3, (%r14,%rdx,8)                       # c759
        vpackstoreld %zmm4, 64(%r14,%rdx,8)                     # c763
        movq      1936(%r12,%r13), %r11                         # c763
        vmovaps   (%r11), %zmm31                                # c767
        vpackstoreld %zmm5, (%r14,%rsi,8)                       # c767
        vmovaps   64(%r11), %zmm0                               # c771
        vpackstoreld %zmm6, 64(%r14,%rsi,8)                     # c771
        vpackstoreld %zmm7, (%r14,%r8,8)                        # c775
        movq      1944(%r12,%r13), %rax                         # c775
        vpackstoreld %zmm8, 64(%r14,%r8,8)                      # c779
        movq      1952(%r12,%r13), %rcx                         # c779
        vpackstoreld %zmm9, (%r14,%r10,8)                       # c783
        movq      1960(%r12,%r13), %rdi                         # c783
        vpackstoreld %zmm10, 64(%r14,%r10,8)                    # c787
        movq      1968(%r12,%r13), %r9                          # c787
        vpackstorehd %zmm1, 64(%r14,%r15,8)                     # c791
        vmovaps   (%rax), %zmm1                                 # c795
        vpackstorehd %zmm2, 128(%r14,%r15,8)                    # c795
        vmovaps   64(%rax), %zmm2                               # c799
        vpackstorehd %zmm3, 64(%r14,%rdx,8)                     # c799
        vmovaps   (%rcx), %zmm3                                 # c803
        vpackstorehd %zmm4, 128(%r14,%rdx,8)                    # c803
        vmovaps   64(%rcx), %zmm4                               # c807
        vpackstorehd %zmm5, 64(%r14,%rsi,8)                     # c807
        vmovaps   (%rdi), %zmm5                                 # c811
        vpackstorehd %zmm6, 128(%r14,%rsi,8)                    # c811
        vmovaps   64(%rdi), %zmm6                               # c815
        vpackstorehd %zmm7, 64(%r14,%r8,8)                      # c815
        vmovaps   (%r9), %zmm7                                  # c819
        vpackstorehd %zmm8, 128(%r14,%r8,8)                     # c819
        vmovaps   64(%r9), %zmm8                                # c823
        vpackstorehd %zmm9, 64(%r14,%r10,8)                     # c823
        vpackstorehd %zmm10, 128(%r14,%r10,8)                   # c827
        movb      %al, %al                                      # c827
        movq      -576(%rbp), %r15                              # c831
        movq      -584(%rbp), %rdx                              # c831
        movq      -592(%rbp), %rsi                              # c835
        vpackstoreld %zmm11, (%r14,%r15,8)                      # c835
        movq      -600(%rbp), %r8                               # c839
        vpackstoreld %zmm12, 64(%r14,%r15,8)                    # c839
        movq      -608(%rbp), %r10                              # c843
        vpackstoreld %zmm13, (%r14,%rdx,8)                      # c843
        vpackstoreld %zmm14, 64(%r14,%rdx,8)                    # c847
        movq      72(%r12,%r13), %rax                           # c847
        vpackstoreld %zmm15, (%r14,%rsi,8)                      # c851
        incq      %rax                                          # c851
        vpackstoreld %zmm16, 64(%r14,%rsi,8)                    # c855
        movl      -264(%rbp), %r11d                             # c855
        vpackstoreld %zmm17, (%r14,%r8,8)                       # c859
        cmpl      -1272(%rbp), %eax                             # c859
        vpackstoreld %zmm18, 64(%r14,%r8,8)                     # c863
        vpackstoreld %zmm19, (%r14,%r10,8)                      # c867
        vpackstoreld %zmm20, 64(%r14,%r10,8)                    # c871
        vpackstorehd %zmm11, 64(%r14,%r15,8)                    # c875
        vpackstorehd %zmm12, 128(%r14,%r15,8)                   # c879
        vpackstorehd %zmm13, 64(%r14,%rdx,8)                    # c883
        vpackstorehd %zmm14, 128(%r14,%rdx,8)                   # c887
        vpackstorehd %zmm15, 64(%r14,%rsi,8)                    # c891
        vpackstorehd %zmm16, 128(%r14,%rsi,8)                   # c895
        vpackstorehd %zmm17, 64(%r14,%r8,8)                     # c899
        vpackstorehd %zmm18, 128(%r14,%r8,8)                    # c903
        vpackstorehd %zmm19, 64(%r14,%r10,8)                    # c907
        vpackstorehd %zmm20, 128(%r14,%r10,8)                   # c911
        movb      %dl, %dl                                      # c911
        movq      -616(%rbp), %r15                              # c915
        movq      %rax, -328(%rbp)                              # c915
        vpackstoreld %zmm21, (%r14,%r15,8)                      # c919
        movq      -624(%rbp), %rdx                              # c919
        movq      -632(%rbp), %rsi                              # c923
        vpackstoreld %zmm22, 64(%r14,%r15,8)                    # c923
        movq      -640(%rbp), %r8                               # c927
        vpackstoreld %zmm23, (%r14,%rdx,8)                      # c927
        movq      -648(%rbp), %r10                              # c931
        vpackstoreld %zmm24, 64(%r14,%rdx,8)                    # c931
        vpackstoreld %zmm25, (%r14,%rsi,8)                      # c935
        vpackstoreld %zmm26, 64(%r14,%rsi,8)                    # c939
        vpackstoreld %zmm27, (%r14,%r8,8)                       # c943
        vpackstoreld %zmm28, 64(%r14,%r8,8)                     # c947
        vpackstoreld %zmm29, (%r14,%r10,8)                      # c951
        vpackstoreld %zmm30, 64(%r14,%r10,8)                    # c955
        vpackstorehd %zmm21, 64(%r14,%r15,8)                    # c959
        vpackstorehd %zmm22, 128(%r14,%r15,8)                   # c963
        vpackstorehd %zmm23, 64(%r14,%rdx,8)                    # c967
        vpackstorehd %zmm24, 128(%r14,%rdx,8)                   # c971
        vpackstorehd %zmm25, 64(%r14,%rsi,8)                    # c975
        vpackstorehd %zmm26, 128(%r14,%rsi,8)                   # c979
        vpackstorehd %zmm27, 64(%r14,%r8,8)                     # c983
        vpackstorehd %zmm28, 128(%r14,%r8,8)                    # c987
        vpackstorehd %zmm29, 64(%r14,%r10,8)                    # c991
        vpackstorehd %zmm30, 128(%r14,%r10,8)                   # c995
        movb      %al, %al                                      # c995
        movq      -656(%rbp), %r15                              # c999
        movl      %r11d, -1344(%rbp)                            # c999
        vpackstoreld %zmm31, (%r14,%r15,8)                      # c1003
        movq      -664(%rbp), %rdx                              # c1003
        movq      -672(%rbp), %rsi                              # c1007
        vpackstoreld %zmm0, 64(%r14,%r15,8)                     # c1007
        movq      -680(%rbp), %r8                               # c1011
        vpackstoreld %zmm1, (%r14,%rdx,8)                       # c1011
        movq      -688(%rbp), %r10                              # c1015
        vpackstoreld %zmm2, 64(%r14,%rdx,8)                     # c1015
        vpackstoreld %zmm3, (%r14,%rsi,8)                       # c1019
        vpackstoreld %zmm4, 64(%r14,%rsi,8)                     # c1023
        vpackstoreld %zmm5, (%r14,%r8,8)                        # c1027
        vpackstoreld %zmm6, 64(%r14,%r8,8)                      # c1031
        vpackstoreld %zmm7, (%r14,%r10,8)                       # c1035
        vpackstoreld %zmm8, 64(%r14,%r10,8)                     # c1039
        vpackstorehd %zmm31, 64(%r14,%r15,8)                    # c1043
        vpackstorehd %zmm0, 128(%r14,%r15,8)                    # c1047
        vpackstorehd %zmm1, 64(%r14,%rdx,8)                     # c1051
        vpackstorehd %zmm2, 128(%r14,%rdx,8)                    # c1055
        vpackstorehd %zmm3, 64(%r14,%rsi,8)                     # c1059
        vpackstorehd %zmm4, 128(%r14,%rsi,8)                    # c1063
        vpackstorehd %zmm5, 64(%r14,%r8,8)                      # c1067
        vpackstorehd %zmm6, 128(%r14,%r8,8)                     # c1071
        vpackstorehd %zmm7, 64(%r14,%r10,8)                     # c1075
        vpackstorehd %zmm8, 128(%r14,%r10,8)                    # c1079
        je        ..B2.18       # Prob 0%                       # c1079
                                # LOE r11 r12 r13 r14 r11d r11b k1 k3 k5
..B2.16:                        # Preds ..B2.15                 # Infreq Latency 1
..CL24:
..B2.17:                        # Preds ..B2.16                 # Infreq Latency 13
        movq      64(%r12,%r13), %rax                           # c1
        movl      -264(%rbp), %edx                              # c1
        movq      %rax, -320(%rbp)                              # c5
        movq      %r12, %rax                                    # c5
        movq      -256(%rbp), %rcx                              # c9
        movl      %edx, -1344(%rbp)                             # c9
        jmp       ..B2.5        # Prob 0%                       # c13
                                # LOE rax rcx r13 r14 k1 k3 k5
..B2.18:                        # Preds ..B2.15                 # Infreq Latency 17
        movq      %r14, -432(%rbp)                              # c1
        movq      -256(%rbp), %r14                              # c1
        movq      %r13, -448(%rbp)                              # c5
        movl      -1272(%rbp), %r15d                            # c5
        movq      -304(%rbp), %r9                               # c9
        movq      -296(%rbp), %rdx                              # c9
        movq      -288(%rbp), %rdi                              # c13
        movq      -280(%rbp), %rsi                              # c13
        movq      -272(%rbp), %r8                               # c17
        movq      -424(%rbp), %r13                              # c17
                                # LOE rdx rsi rdi r8 r9 r11 r12 r13 r14 r11d r15d r11b k1 k3 k5
..CL15:
..B2.19:                        # Preds ..B2.18 ..B2.26         # Infreq Latency 1
        cmpq      %r13, %r14                                    # c1
        jae       ..B2.25       # Prob 0%                       # c1
                                # LOE rdx rsi rdi r8 r9 r11 r12 r13 r14 r11d r15d r11b k1 k3 k5
..B2.20:                        # Preds ..B2.19                 # Infreq Latency 1
..CL26:
..B2.21:                        # Preds ..B2.20                 # Infreq Latency 13
        lea       1(%r14), %rcx                                 # c1
        lea       6656(%r12), %rax                              # c1
        movl      %r11d, %r10d                                  # c5
        movq      %rcx, %r14                                    # c5
        movq      %rax, %r12                                    # c9
        cmpl      $1, %r11d                                     # c9
        je        ..B2.23       # Prob 0%                       # c13
                                # LOE rax rdx rcx rsi rdi r8 r9 r10 r12 r13 r14 r10d r11d r15d r10b k1 k3 k5
..B2.22:                        # Preds ..B2.21                 # Infreq Latency 13
        movq      %rax, -472(%rbp)                              # c1
        movq      -376(%rbp), %r10                              # c1
        movq      %rcx, -336(%rbp)                              # c5
        movq      -368(%rbp), %rax                              # c5
        movq      -360(%rbp), %rcx                              # c9
        movq      -352(%rbp), %r13                              # c9
        movq      -344(%rbp), %r11                              # c13
        jmp       ..B2.2        # Prob 0%                       # c13
                                # LOE rax rdx rcx rsi rdi r8 r9 r10 r11 r13 k1 k3 k5
..B2.23:                        # Preds ..B2.21                 # Infreq Latency 18
        movl      %r10d, -264(%rbp)                             # c1
        movq      -448(%rbp), %r13                              # c1
        movq      %r14, -256(%rbp)                              # c5
        movq      -432(%rbp), %r14                              # c5
        movl      %r15d, -1272(%rbp)                            # c9
        movq      %r9, -304(%rbp)                               # c9
        movq      %rdx, -296(%rbp)                              # c13
        movq      %rdi, -288(%rbp)                              # c13
        movq      %rsi, -280(%rbp)                              # c17
        movq      %r8, -272(%rbp)                               # c17
        jmp       ..B2.9        # Prob 100%                     # c17
                                # LOE r12 r13 r14 k1 k3 k5
..B2.24:                        # Preds ..B2.7                  # Infreq Latency 26
        movq      %rax, -472(%rbp)                              # c1
        movq      -304(%rbp), %r9                               # c1
        movq      %r14, -432(%rbp)                              # c5
        movq      -296(%rbp), %rdx                              # c5
        movq      %r13, -448(%rbp)                              # c9
        movq      -288(%rbp), %rdi                              # c9
        movq      %rcx, -336(%rbp)                              # c13
        movq      -280(%rbp), %rsi                              # c13
        movq      -272(%rbp), %r8                               # c17
        movq      -376(%rbp), %r10                              # c17
        movq      -368(%rbp), %rax                              # c21
        movq      -360(%rbp), %rcx                              # c21
        movq      -352(%rbp), %r13                              # c25
        movq      -344(%rbp), %r11                              # c25
        jmp       ..B2.2        # Prob 100%                     # c25
                                # LOE rax rdx rcx rsi rdi r8 r9 r10 r11 r13 k1 k3 k5
..CL25:
..B2.25:                        # Preds ..B2.19                 # Infreq Latency 9
        movq      -1280(%rbp), %r12                             # c1
        movq      -1288(%rbp), %r13                             # c1
        movq      -1296(%rbp), %r14                             # c5
        movq      -1304(%rbp), %r15                             # c5
        movq      %rbp, %rsp                                    # c9
        popq      %rbp                                          #
        movq      %rbx, %rsp                                    #
        popq      %rbx                                          #
        ret                                                     #
                                # LOE
..B2.26:                        # Preds ..B2.2                  # Infreq Latency 14
        movq      %r10, -376(%rbp)                              # c1
        movq      %r13, -352(%rbp)                              # c1
        movq      %rax, -368(%rbp)                              # c5
        movq      -424(%rbp), %r13                              # c5
        movq      %rcx, -360(%rbp)                              # c9
        movq      %r11, -344(%rbp)                              # c9
        movl      -1344(%rbp), %r11d                            # c13
        jmp       ..B2.19       # Prob 100%                     # c13
                                # LOE rdx rsi rdi r8 r9 r11 r12 r13 r14 r11d r15d r11b k1 k3 k5
..B2.30:                        # Preds ..B2.38                 # Infreq Latency 29
        vbroadcasti64x4 _const_3(%rip), %zmm25                  # c1
        vgetmantpd $0, %zmm4, %zmm6                             # c5
        vcmpneqpd %zmm25{aaaa}, %zmm4, %k5{%k7}                 # c9
        vgetmantpd $0, %zmm5, %zmm24                            # c13
        vcmpneqpd %zmm24, %zmm6, %k6{%k5}                       # c17
        vmovaps   %zmm2, %zmm6                                  # c21
        vfnmadd231pd {rn-sae}, %zmm23, %zmm27, %zmm6{%k5}       # c25
        vcmpneqpd %zmm25{aaaa}, %zmm6, %k0{%k5}                 # c29
        jkzd      ..B2.31, %k6  # Prob 95%                      # c29
                                # LOE rax rdx rcx rsi rdi r9 r11 r12 r13 xmm14 xmm15   ymm14 ymm15   zmm0 zmm1 zmm2 zmm3 zmm4 zmm5 
zmm6 zmm7 zmm8 zmm9 zmm10 zmm11 zmm12 zmm13 zmm14 zmm15 zmm16 zmm17 zmm18 zmm19 zmm20 zmm21 zmm22 zmm26 zmm27 zmm28 zmm31 k0 k1 k2 
k3 k4 k5 k6
..B2.32:                        # Preds ..B2.30                 # Infreq Latency 54
        vbroadcasti32x4 _const_3(%rip), %zmm23                  # c1
        vbroadcasti64x4 _const_3(%rip), %zmm24                  # c5
        kand      %k0, %k6                                      # c9
        vpxorq    %zmm27, %zmm6, %zmm6{%k6}                     # c13
        vpsubd    %zmm23{dddd}, %zmm5, %zmm27                   # c17
        vpandq    %zmm24{cccc}, %zmm6, %zmm6{%k6}               # c21
        vpandq    %zmm24{dddd}, %zmm27, %zmm27{%k6}             # c25
        vporq     %zmm6, %zmm27, %zmm27{%k6}                    # c29
        vpsubrd   %zmm23{cccc}, %zmm26, %zmm6                   # c33
        vpandq    %zmm24{dddd}, %zmm6, %zmm6{%k6}               # c37
        vmulpd    {rn-sae}, %zmm6, %zmm4, %zmm25                # c41
        vsubpd    {rn-sae}, %zmm25, %zmm5, %zmm5                # c45
        vaddpd    {rn-sae}, %zmm27, %zmm5, %zmm29               # c49
        vfmadd231pd %zmm29, %zmm26, %zmm4{%k6}                  # c53
        jmp       ..B2.29       # Prob 100%                     # c53
                                # LOE rax rdx rcx rsi rdi r9 r11 r12 r13 xmm14 xmm15   ymm14 ymm15   zmm0 zmm1 zmm2 zmm3 zmm4 zmm7 
zmm8 zmm9 zmm10 zmm11 zmm12 zmm13 zmm14 zmm15 zmm16 zmm17 zmm18 zmm19 zmm20 zmm21 zmm22 zmm28 zmm31 k1 k2 k3 k4
..B2.31:                        # Preds ..B2.30                 # Infreq Latency 26
        vbroadcasti64x4 _const_3(%rip), %zmm23                  # c1
        vbroadcasti64x4 _const_3(%rip), %zmm6                   # c5
        vmovdqa64 %zmm23{aaaa}, %zmm5                           # c9
        kandn     %k0, %k6                                      # c9
        vpandq    %zmm23{dddd}, %zmm4, %zmm5{%k6}               # c13
        vcmpeqpd  %zmm6{aaaa}, %zmm5, %k5{%k5}                  # c17
        vmovdqa64 %zmm23{bbbb}, %zmm24                          # c21
        vfmadd231pd {rn}, %zmm24, %zmm24, %zmm4{%k5}            # c25
        jmp       ..B2.29       # Prob 100%                     # c25
                                # LOE rax rdx rcx rsi rdi r9 r11 r12 r13 xmm14 xmm15   ymm14 ymm15   zmm0 zmm1 zmm2 zmm3 zmm4 zmm7 
zmm8 zmm9 zmm10 zmm11 zmm12 zmm13 zmm14 zmm15 zmm16 zmm17 zmm18 zmm19 zmm20 zmm21 zmm22 zmm28 zmm31 k1 k2 k3 k4
..B2.34:                        # Preds ..B2.29                 # Infreq Latency 29
        vbroadcasti64x4 _const_3(%rip), %zmm26                  # c1
        vgetmantpd $0, %zmm5, %zmm23                            # c5
        vcmpneqpd %zmm26{aaaa}, %zmm5, %k5{%k7}                 # c9
        vgetmantpd $0, %zmm1, %zmm24                            # c13
        vfnmadd231pd {rn-sae}, %zmm6, %zmm28, %zmm2{%k5}        # c17
        vcmpneqpd %zmm24, %zmm23, %k6{%k5}                      # c21
        vcmpneqpd %zmm26{aaaa}, %zmm2, %k0{%k5}                 # c25
        movb      %al, %al                                      # c25
        jkzd      ..B2.35, %k6  # Prob 95%                      # c29
                                # LOE rax rdx rcx rsi rdi r9 r11 r12 r13 xmm14 xmm15   ymm14 ymm15   zmm0 zmm1 zmm2 zmm3 zmm4 zmm5 
zmm7 zmm8 zmm9 zmm10 zmm11 zmm12 zmm13 zmm14 zmm15 zmm16 zmm17 zmm18 zmm19 zmm20 zmm21 zmm22 zmm25 zmm28 zmm31 k0 k1 k2 k3 k4 k5 k6
..B2.36:                        # Preds ..B2.34                 # Infreq Latency 50
        vbroadcasti32x4 _const_3(%rip), %zmm6                   # c1
        vbroadcasti64x4 _const_3(%rip), %zmm23                  # c5
        vpsubd    %zmm6{dddd}, %zmm1, %zmm26                    # c9
        kand      %k0, %k6                                      # c9
        vpxorq    %zmm28, %zmm2, %zmm2{%k6}                     # c13
        vpandq    %zmm23{cccc}, %zmm2, %zmm2{%k6}               # c17
        vpandq    %zmm23{dddd}, %zmm26, %zmm26{%k6}             # c21
        vporq     %zmm2, %zmm26, %zmm26{%k6}                    # c25
        vpsubrd   %zmm6{cccc}, %zmm25, %zmm2                    # c29
        vpandq    %zmm23{dddd}, %zmm2, %zmm2{%k6}               # c33
        vmulpd    {rn-sae}, %zmm2, %zmm5, %zmm24                # c37
        vsubpd    {rn-sae}, %zmm24, %zmm1, %zmm1                # c41
        vaddpd    {rn-sae}, %zmm26, %zmm1, %zmm27               # c45
        vfmadd231pd %zmm27, %zmm25, %zmm5{%k6}                  # c49
        jmp       ..B2.33       # Prob 100%                     # c49
                                # LOE rax rdx rcx rsi rdi r9 r11 r12 r13 xmm14 xmm15   ymm14 ymm15   zmm0 zmm3 zmm4 zmm5 zmm7 zmm8 
zmm9 zmm10 zmm11 zmm12 zmm13 zmm14 zmm15 zmm16 zmm17 zmm18 zmm19 zmm20 zmm21 zmm22 zmm31 k1 k2 k3 k4
..B2.35:                        # Preds ..B2.34                 # Infreq Latency 26
        vbroadcasti64x4 _const_3(%rip), %zmm6                   # c1
        vbroadcasti64x4 _const_3(%rip), %zmm2                   # c5
        vmovdqa64 %zmm6{aaaa}, %zmm1                            # c9
        kandn     %k0, %k6                                      # c9
        vpandq    %zmm6{dddd}, %zmm5, %zmm1{%k6}                # c13
        vcmpeqpd  %zmm2{aaaa}, %zmm1, %k5{%k5}                  # c17
        vmovdqa64 %zmm6{bbbb}, %zmm23                           # c21
        vfmadd231pd {rn}, %zmm23, %zmm23, %zmm5{%k5}            # c25
        jmp       ..B2.33       # Prob 100%                     # c25
        .align    16,0x90
                                # LOE rax rdx rcx rsi rdi r9 r11 r12 r13 xmm14 xmm15   ymm14 ymm15   zmm0 zmm3 zmm4 zmm5 zmm7 zmm8 
zmm9 zmm10 zmm11 zmm12 zmm13 zmm14 zmm15 zmm16 zmm17 zmm18 zmm19 zmm20 zmm21 zmm22 zmm31 k1 k2 k3 k4
# mark_end;
	.type	__Vectorized_.tmr_ocl_num_int_el,@function
	.size	__Vectorized_.tmr_ocl_num_int_el,.-__Vectorized_.tmr_ocl_num_int_el
	.section .note.GNU-stack, ""
[
