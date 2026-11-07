# TDCSRX RX 0.3 — validated Thumb disassembly

## Source verification

- file_size: 4140
- blob_sha1: af2142508e75b9ef2d2d2c4963be33ace5ee8523
- header_version: 1
- abi_major: 1
- api_min: 3
- code_size: 4076
- code_crc32: 0x640d979a
- entry_off: 0
- flags: 0x00000001
- name: TDCS RX
- version: 0.3
- link_vma: 0x20000280
- required_caps: 0x00000000
- asset_size: 0
- asset_crc: 0

## Callers of known routines

- target 0x01d4: 0x020c, 0x021e
- target 0x0204: 0x08a0
- target 0x0314: 0x0b1a
- target 0x061c: 0x08f2
- target 0x0950: 0x0d0a
- target 0x0ad4: 0x0102, 0x0c92
- target 0x0bf4: 0x0d40

## Selected instruction windows

### 0x0000..0x0090 — entry / radio initialization

    0000  20000280  f0 b5       push     {r4, r5, r6, r7, lr}
    0002  20000282  4f 4f       ldr      r7, [pc, #0x13c]
    0004  20000284  13 22       movs     r2, #0x13
    0006  20000286  3b 00       movs     r3, r7
    0008  20000288  b7 33       adds     r3, #0xb7
    000a  2000028a  1a 70       strb     r2, [r3]
    000c  2000028c  3a 00       movs     r2, r7
    000e  2000028e  4d 4b       ldr      r3, [pc, #0x134]
    0010  20000290  99 6b       ldr      r1, [r3, #0x38]
    0012  20000292  c0 32       adds     r2, #0xc0
    0014  20000294  11 60       str      r1, [r2]
    0016  20000296  3a 00       movs     r2, r7
    0018  20000298  59 69       ldr      r1, [r3, #0x14]
    001a  2000029a  c4 32       adds     r2, #0xc4
    001c  2000029c  11 60       str      r1, [r2]
    001e  2000029e  a0 21       movs     r1, #0xa0
    0020  200002a0  c9 05       lsls     r1, r1, #0x17
    0022  200002a2  0a 68       ldr      r2, [r1]
    0024  200002a4  87 b0       sub      sp, #0x1c
    0026  200002a6  04 00       movs     r4, r0
    0028  200002a8  38 60       str      r0, [r7]
    002a  200002aa  01 92       str      r2, [sp, #4]
    002c  200002ac  46 48       ldr      r0, [pc, #0x118]
    002e  200002ae  47 4d       ldr      r5, [pc, #0x11c]
    0030  200002b0  2a 68       ldr      r2, [r5]
    0032  200002b2  01 9e       ldr      r6, [sp, #4]
    0034  200002b4  04 92       str      r2, [sp, #0x10]
    0036  200002b6  02 68       ldr      r2, [r0]
    0038  200002b8  02 92       str      r2, [sp, #8]
    003a  200002ba  aa 68       ldr      r2, [r5, #8]
    003c  200002bc  05 92       str      r2, [sp, #0x14]
    003e  200002be  c0 22       movs     r2, #0xc0
    0040  200002c0  92 00       lsls     r2, r2, #2
    0042  200002c2  32 43       orrs     r2, r6
    0044  200002c4  0a 60       str      r2, [r1]
    0046  200002c6  80 22       movs     r2, #0x80
    0048  200002c8  02 9e       ldr      r6, [sp, #8]
    004a  200002ca  92 05       lsls     r2, r2, #0x16
    004c  200002cc  32 43       orrs     r2, r6
    004e  200002ce  02 60       str      r2, [r0]
    0050  200002d0  3f 22       movs     r2, #0x3f
    0052  200002d2  2a 60       str      r2, [r5]
    0054  200002d4  80 22       movs     r2, #0x80
    0056  200002d6  12 01       lsls     r2, r2, #4
    0058  200002d8  aa 60       str      r2, [r5, #8]
    005a  200002da  01 22       movs     r2, #1
    005c  200002dc  6a 60       str      r2, [r5, #4]
    005e  200002de  e0 22       movs     r2, #0xe0
    0060  200002e0  59 69       ldr      r1, [r3, #0x14]
    0062  200002e2  d2 01       lsls     r2, r2, #7
    0064  200002e4  0a 43       orrs     r2, r1
    0066  200002e6  5a 61       str      r2, [r3, #0x14]
    0068  200002e8  9a 6b       ldr      r2, [r3, #0x38]
    006a  200002ea  1f 21       movs     r1, #0x1f
    006c  200002ec  16 00       movs     r6, r2
    006e  200002ee  04 22       movs     r2, #4
    0070  200002f0  8e 43       bics     r6, r1
    0072  200002f2  32 43       orrs     r2, r6
    0074  200002f4  00 26       movs     r6, #0
    0076  200002f6  9a 63       str      r2, [r3, #0x38]
    0078  200002f8  23 1d       adds     r3, r4, #4
    007a  200002fa  db 6f       ldr      r3, [r3, #0x7c]
    007c  200002fc  98 47       blx      r3
    007e  200002fe  34 4b       ldr      r3, [pc, #0xd0]
    0080  20000300  38 20       movs     r0, #0x38
    0082  20000302  1e 70       strb     r6, [r3]
    0084  20000304  3b 00       movs     r3, r7
    0086  20000306  be 33       adds     r3, #0xbe
    0088  20000308  1e 70       strb     r6, [r3]
    008a  2000030a  3b 00       movs     r3, r7
    008c  2000030c  b4 33       adds     r3, #0xb4
    008e  2000030e  1e 80       strh     r6, [r3]

### 0x0090..0x0220 — sync / BCH decode

    0090  20000310  3b 00       movs     r3, r7
    0092  20000312  b6 33       adds     r3, #0xb6
    0094  20000314  1e 70       strb     r6, [r3]
    0096  20000316  23 6d       ldr      r3, [r4, #0x50]
    0098  20000318  2e 49       ldr      r1, [pc, #0xb8]
    009a  2000031a  98 47       blx      r3
    009c  2000031c  23 6d       ldr      r3, [r4, #0x50]
    009e  2000031e  39 20       movs     r0, #0x39
    00a0  20000320  2d 49       ldr      r1, [pc, #0xb4]
    00a2  20000322  98 47       blx      r3
    00a4  20000324  2d 49       ldr      r1, [pc, #0xb4]
    00a6  20000326  23 6d       ldr      r3, [r4, #0x50]
    00a8  20000328  43 20       movs     r0, #0x43
    00aa  2000032a  98 47       blx      r3
    00ac  2000032c  23 6d       ldr      r3, [r4, #0x50]
    00ae  2000032e  2b 20       movs     r0, #0x2b
    00b0  20000330  03 93       str      r3, [sp, #0xc]
    00b2  20000332  e3 6c       ldr      r3, [r4, #0x4c]
    00b4  20000334  98 47       blx      r3
    00b6  20000336  2a 49       ldr      r1, [pc, #0xa8]
    00b8  20000338  03 9b       ldr      r3, [sp, #0xc]
    00ba  2000033a  01 40       ands     r1, r0
    00bc  2000033c  2b 20       movs     r0, #0x2b
    00be  2000033e  98 47       blx      r3
    00c0  20000340  23 6d       ldr      r3, [r4, #0x50]
    00c2  20000342  73 20       movs     r0, #0x73
    00c4  20000344  03 93       str      r3, [sp, #0xc]
    00c6  20000346  e3 6c       ldr      r3, [r4, #0x4c]
    00c8  20000348  98 47       blx      r3
    00ca  2000034a  10 23       movs     r3, #0x10
    00cc  2000034c  98 43       bics     r0, r3
    00ce  2000034e  81 b2       uxth     r1, r0
    00d0  20000350  03 9b       ldr      r3, [sp, #0xc]
    00d2  20000352  73 20       movs     r0, #0x73
    00d4  20000354  98 47       blx      r3
    00d6  20000356  e3 6c       ldr      r3, [r4, #0x4c]
    00d8  20000358  30 20       movs     r0, #0x30
    00da  2000035a  98 47       blx      r3
    00dc  2000035c  31 00       movs     r1, r6
    00de  2000035e  23 6d       ldr      r3, [r4, #0x50]
    00e0  20000360  03 90       str      r0, [sp, #0xc]
    00e2  20000362  30 20       movs     r0, #0x30
    00e4  20000364  98 47       blx      r3
    00e6  20000366  03 99       ldr      r1, [sp, #0xc]
    00e8  20000368  23 6d       ldr      r3, [r4, #0x50]
    00ea  2000036a  30 20       movs     r0, #0x30
    00ec  2000036c  98 47       blx      r3
    00ee  2000036e  18 4b       ldr      r3, [pc, #0x60]
    00f0  20000370  18 78       ldrb     r0, [r3]
    00f2  20000372  e3 6d       ldr      r3, [r4, #0x5c]
    00f4  20000374  98 47       blx      r3
    00f6  20000376  a3 6d       ldr      r3, [r4, #0x58]
    00f8  20000378  01 20       movs     r0, #1
    00fa  2000037a  98 47       blx      r3
    00fc  2000037c  32 20       movs     r0, #0x32
    00fe  2000037e  23 6b       ldr      r3, [r4, #0x30]
    0100  20000380  98 47       blx      r3
    0102  20000382  00 f0 e7 fc bl       #0x20000d54
    0106  20000386  3b 00       movs     r3, r7
    0108  20000388  01 22       movs     r2, #1
    010a  2000038a  bc 33       adds     r3, #0xbc
    010c  2000038c  1a 70       strb     r2, [r3]
    010e  2000038e  00 f0 d3 fd bl       #0x20000f38
    0112  20000392  a0 21       movs     r1, #0xa0
    0114  20000394  04 9b       ldr      r3, [sp, #0x10]
    0116  20000396  0c 48       ldr      r0, [pc, #0x30]
    0118  20000398  2b 60       str      r3, [r5]
    011a  2000039a  05 9b       ldr      r3, [sp, #0x14]
    011c  2000039c  c9 05       lsls     r1, r1, #0x17
    011e  2000039e  ab 60       str      r3, [r5, #8]
    0120  200003a0  02 9b       ldr      r3, [sp, #8]
    0122  200003a2  03 60       str      r3, [r0]
    0124  200003a4  01 9b       ldr      r3, [sp, #4]
    0126  200003a6  30 00       movs     r0, r6
    0128  200003a8  0b 60       str      r3, [r1]
    012a  200003aa  a3 6b       ldr      r3, [r4, #0x38]
    012c  200003ac  98 47       blx      r3
    012e  200003ae  30 00       movs     r0, r6
    0130  200003b0  a3 6d       ldr      r3, [r4, #0x58]
    0132  200003b2  98 47       blx      r3
    0134  200003b4  30 00       movs     r0, r6
    0136  200003b6  e3 6d       ldr      r3, [r4, #0x5c]
    0138  200003b8  98 47       blx      r3
    013a  200003ba  07 b0       add      sp, #0x1c
    013c  200003bc  f0 bd       pop      {r4, r5, r6, r7, pc}
    013e  200003be  c0 46       mov      r8, r8
    0140  200003c0  a4 11       asrs     r4, r4, #6
    0142  200003c2  00 20       movs     r0, #0
    0144  200003c4  00 24       movs     r4, #0
    0146  200003c6  01 40       ands     r1, r0
    0148  200003c8  3c 10       asrs     r4, r7, #0x20
    014a  200003ca  02 40       ands     r2, r0
    014c  200003cc  00 74       strb     r0, [r0, #0x10]
    014e  200003ce  00 40       ands     r0, r0
    0150  200003d0  61 12       asrs     r1, r4, #9
    0152  200003d2  00 20       movs     r0, #0
    0154  200003d4  78 2a       cmp      r2, #0x78
    0156  200003d6  00 00       movs     r0, r0
    0158  200003d8  ba 02       lsls     r2, r7, #0xa
    015a  200003da  00 00       movs     r0, r0
    015c  200003dc  28 36       adds     r6, #0x28
    015e  200003de  00 00       movs     r0, r0
    0160  200003e0  ff f8       .hword   0xff, 0xf8
    0162  200003e2  00 00       movs     r0, r0
    0164  200003e4  08 4a       ldr      r2, [pc, #0x20]
    0166  200003e6  43 08       lsrs     r3, r0, #1
    0168  200003e8  13 40       ands     r3, r2
    016a  200003ea  c0 1a       subs     r0, r0, r3
    016c  200003ec  03 00       movs     r3, r0
    016e  200003ee  07 4a       ldr      r2, [pc, #0x1c]
    0170  200003f0  80 08       lsrs     r0, r0, #2
    0172  200003f2  10 40       ands     r0, r2
    0174  200003f4  13 40       ands     r3, r2
    0176  200003f6  1b 18       adds     r3, r3, r0
    0178  200003f8  18 09       lsrs     r0, r3, #4
    017a  200003fa  c0 18       adds     r0, r0, r3
    017c  200003fc  04 4b       ldr      r3, [pc, #0x10]
    017e  200003fe  03 40       ands     r3, r0
    0180  20000400  04 48       ldr      r0, [pc, #0x10]
    0182  20000402  58 43       muls     r0, r3, r0
    0184  20000404  00 0e       lsrs     r0, r0, #0x18
    0186  20000406  70 47       bx       lr
    0188  20000408  55 55       strb     r5, [r2, r5]
    018a  2000040a  55 55       strb     r5, [r2, r5]
    018c  2000040c  33 33       adds     r3, #0x33
    018e  2000040e  33 33       adds     r3, #0x33
    0190  20000410  0f 0f       lsrs     r7, r1, #0x1c
    0192  20000412  0f 0f       lsrs     r7, r1, #0x1c
    0194  20000414  01 01       lsls     r1, r0, #4
    0196  20000416  01 01       lsls     r1, r0, #4
    0198  20000418  10 b5       push     {r4, lr}
    019a  2000041a  c4 17       asrs     r4, r0, #0x1f
    019c  2000041c  03 19       adds     r3, r0, r4
    019e  2000041e  c8 17       asrs     r0, r1, #0x1f
    01a0  20000420  09 18       adds     r1, r1, r0
    01a2  20000422  63 40       eors     r3, r4
    01a4  20000424  41 40       eors     r1, r0
    01a6  20000426  8b 42       cmp      r3, r1
    01a8  20000428  02 da       bge      #0x20000430
    01aa  2000042a  18 00       movs     r0, r3
    01ac  2000042c  0b 00       movs     r3, r1
    01ae  2000042e  01 00       movs     r1, r0
    01b0  20000430  88 10       asrs     r0, r1, #2
    01b2  20000432  c3 18       adds     r3, r0, r3
    01b4  20000434  c8 10       asrs     r0, r1, #3
    01b6  20000436  11 68       ldr      r1, [r2]
    01b8  20000438  18 18       adds     r0, r3, r0
    01ba  2000043a  81 42       cmp      r1, r0
    01bc  2000043c  07 da       bge      #0x2000044e
    01be  2000043e  43 1a       subs     r3, r0, r1
    01c0  20000440  5b 11       asrs     r3, r3, #5
    01c2  20000442  5b 18       adds     r3, r3, r1
    01c4  20000444  10 2b       cmp      r3, #0x10
    01c6  20000446  00 da       bge      #0x2000044a
    01c8  20000448  10 23       movs     r3, #0x10
    01ca  2000044a  13 60       str      r3, [r2]
    01cc  2000044c  10 bd       pop      {r4, pc}
    01ce  2000044e  8b 12       asrs     r3, r1, #0xa
    01d0  20000450  5b 42       rsbs     r3, r3, #0
    01d2  20000452  f6 e7       b        #0x20000442
    01d4  20000454  19 23       movs     r3, #0x19
    01d6  20000456  01 22       movs     r2, #1
    01d8  20000458  30 b5       push     {r4, r5, lr}
    01da  2000045a  80 01       lsls     r0, r0, #6
    01dc  2000045c  08 49       ldr      r1, [pc, #0x20]
    01de  2000045e  80 09       lsrs     r0, r0, #6
    01e0  20000460  04 00       movs     r4, r0
    01e2  20000462  dc 40       lsrs     r4, r3
    01e4  20000464  14 42       tst      r4, r2
    01e6  20000466  04 d0       beq      #0x20000472
    01e8  20000468  1c 00       movs     r4, r3
    01ea  2000046a  0d 00       movs     r5, r1
    01ec  2000046c  0a 3c       subs     r4, #0xa
    01ee  2000046e  a5 40       lsls     r5, r4
    01f0  20000470  68 40       eors     r0, r5
    01f2  20000472  01 3b       subs     r3, #1
    01f4  20000474  09 2b       cmp      r3, #9
    01f6  20000476  f3 d1       bne      #0x20000460
    01f8  20000478  80 05       lsls     r0, r0, #0x16
    01fa  2000047a  80 0d       lsrs     r0, r0, #0x16
    01fc  2000047c  30 bd       pop      {r4, r5, pc}
    01fe  2000047e  c0 46       mov      r8, r8
    0200  20000480  b9 05       lsls     r1, r7, #0x16
    0202  20000482  00 00       movs     r0, r0
    0204  20000484  f0 b5       push     {r4, r5, r6, r7, lr}
    0206  20000486  91 b0       sub      sp, #0x44
    0208  20000488  05 00       movs     r5, r0
    020a  2000048a  00 26       movs     r6, #0
    020c  2000048c  ff f7 e2 ff bl       #0x20000454
    0210  20000490  01 90       str      r0, [sp, #4]
    0212  20000492  b0 42       cmp      r0, r6
    0214  20000494  24 d0       beq      #0x200004e0
    0216  20000496  01 27       movs     r7, #1
    0218  20000498  03 ac       add      r4, sp, #0xc
    021a  2000049a  38 00       movs     r0, r7
    021c  2000049c  b0 40       lsls     r0, r6
    021e  2000049e  ff f7 d9 ff bl       #0x20000454

### 0x0220..0x0430 — frame assembly / parser support

    0222  200004a2  73 00       lsls     r3, r6, #1
    0224  200004a4  01 36       adds     r6, #1
    0226  200004a6  e0 52       strh     r0, [r4, r3]
    0228  200004a8  1a 2e       cmp      r6, #0x1a
    022a  200004aa  f6 d1       bne      #0x2000049a
    022c  200004ac  00 23       movs     r3, #0
    022e  200004ae  5a 00       lsls     r2, r3, #1
    0230  200004b0  a2 5a       ldrh     r2, [r4, r2]
    0232  200004b2  01 99       ldr      r1, [sp, #4]
    0234  200004b4  8a 42       cmp      r2, r1
    0236  200004b6  04 d1       bne      #0x200004c2
    0238  200004b8  01 20       movs     r0, #1
    023a  200004ba  98 40       lsls     r0, r3
    023c  200004bc  68 40       eors     r0, r5
    023e  200004be  80 0a       lsrs     r0, r0, #0xa
    0240  200004c0  0f e0       b        #0x200004e2
    0242  200004c2  01 33       adds     r3, #1
    0244  200004c4  1a 2b       cmp      r3, #0x1a
    0246  200004c6  f2 d1       bne      #0x200004ae
    0248  200004c8  20 00       movs     r0, r4
    024a  200004ca  19 3b       subs     r3, #0x19
    024c  200004cc  59 1e       subs     r1, r3, #1
    024e  200004ce  1a 00       movs     r2, r3
    0250  200004d0  8c 46       mov      ip, r1
    0252  200004d2  d1 b2       uxtb     r1, r2
    0254  200004d4  1a 29       cmp      r1, #0x1a
    0256  200004d6  07 d1       bne      #0x200004e8
    0258  200004d8  01 33       adds     r3, #1
    025a  200004da  02 34       adds     r4, #2
    025c  200004dc  1b 2b       cmp      r3, #0x1b
    025e  200004de  f5 d1       bne      #0x200004cc
    0260  200004e0  a8 0a       lsrs     r0, r5, #0xa
    0262  200004e2  80 b2       uxth     r0, r0
    0264  200004e4  11 b0       add      sp, #0x44
    0266  200004e6  f0 bd       pop      {r4, r5, r6, r7, pc}
    0268  200004e8  51 00       lsls     r1, r2, #1
    026a  200004ea  27 88       ldrh     r7, [r4]
    026c  200004ec  09 5a       ldrh     r1, [r1, r0]
    026e  200004ee  16 00       movs     r6, r2
    0270  200004f0  79 40       eors     r1, r7
    0272  200004f2  01 9f       ldr      r7, [sp, #4]
    0274  200004f4  01 32       adds     r2, #1
    0276  200004f6  b9 42       cmp      r1, r7
    0278  200004f8  eb d1       bne      #0x200004d2
    027a  200004fa  01 23       movs     r3, #1
    027c  200004fc  62 46       mov      r2, ip
    027e  200004fe  18 00       movs     r0, r3
    0280  20000500  b3 40       lsls     r3, r6
    0282  20000502  90 40       lsls     r0, r2
    0284  20000504  58 40       eors     r0, r3
    0286  20000506  d9 e7       b        #0x200004bc
    0288  20000508  70 b5       push     {r4, r5, r6, lr}
    028a  2000050a  0b 4e       ldr      r6, [pc, #0x2c]
    028c  2000050c  33 68       ldr      r3, [r6]
    028e  2000050e  fc 33       adds     r3, #0xfc
    0290  20000510  5b 68       ldr      r3, [r3, #4]
    0292  20000512  98 47       blx      r3
    0294  20000514  09 4b       ldr      r3, [pc, #0x24]
    0296  20000516  05 00       movs     r5, r0
    0298  20000518  1c 68       ldr      r4, [r3]
    029a  2000051a  33 68       ldr      r3, [r6]
    029c  2000051c  fc 33       adds     r3, #0xfc
    029e  2000051e  5b 68       ldr      r3, [r3, #4]
    02a0  20000520  98 47       blx      r3
    02a2  20000522  03 00       movs     r3, r0
    02a4  20000524  a8 42       cmp      r0, r5
    02a6  20000526  f0 d1       bne      #0x2000050a
    02a8  20000528  05 48       ldr      r0, [pc, #0x14]
    02aa  2000052a  06 4a       ldr      r2, [pc, #0x18]
    02ac  2000052c  58 43       muls     r0, r3, r0
    02ae  2000052e  12 68       ldr      r2, [r2]
    02b0  20000530  00 1b       subs     r0, r0, r4
    02b2  20000532  80 18       adds     r0, r0, r2
    02b4  20000534  70 bd       pop      {r4, r5, r6, pc}
    02b6  20000536  c0 46       mov      r8, r8
    02b8  20000538  a4 11       asrs     r4, r4, #6
    02ba  2000053a  00 20       movs     r0, #0
    02bc  2000053c  18 e0       b        #0x20000570
    02be  2000053e  00 e0       b        #0x20000542
    02c0  20000540  80 bb       cbnz     r0, #0x200005a4
    02c2  20000542  00 00       movs     r0, r0
    02c4  20000544  14 e0       b        #0x20000570
    02c6  20000546  00 e0       b        #0x2000054a
    02c8  20000548  0b 78       ldrb     r3, [r1]
    02ca  2000054a  00 2b       cmp      r3, #0
    02cc  2000054c  00 d1       bne      #0x20000550
    02ce  2000054e  70 47       bx       lr
    02d0  20000550  03 70       strb     r3, [r0]
    02d2  20000552  01 31       adds     r1, #1
    02d4  20000554  01 30       adds     r0, #1
    02d6  20000556  f7 e7       b        #0x20000548
    02d8  20000558  00 22       movs     r2, #0
    02da  2000055a  f0 b5       push     {r4, r5, r6, r7, lr}
    02dc  2000055c  14 00       movs     r4, r2
    02de  2000055e  01 26       movs     r6, #1
    02e0  20000560  0b 4d       ldr      r5, [pc, #0x2c]
    02e2  20000562  00 23       movs     r3, #0
    02e4  20000564  2f 68       ldr      r7, [r5]
    02e6  20000566  02 e0       b        #0x2000056e
    02e8  20000568  01 33       adds     r3, #1
    02ea  2000056a  c9 1b       subs     r1, r1, r7
    02ec  2000056c  db b2       uxtb     r3, r3
    02ee  2000056e  8f 42       cmp      r7, r1
    02f0  20000570  fa d9       bls      #0x20000568
    02f2  20000572  1c 43       orrs     r4, r3
    02f4  20000574  01 d1       bne      #0x2000057a
    02f6  20000576  06 2a       cmp      r2, #6
    02f8  20000578  03 d1       bne      #0x20000582
    02fa  2000057a  34 00       movs     r4, r6
    02fc  2000057c  30 33       adds     r3, #0x30
    02fe  2000057e  03 70       strb     r3, [r0]
    0300  20000580  01 30       adds     r0, #1
    0302  20000582  01 32       adds     r2, #1
    0304  20000584  d2 b2       uxtb     r2, r2
    0306  20000586  04 35       adds     r5, #4
    0308  20000588  07 2a       cmp      r2, #7
    030a  2000058a  ea d1       bne      #0x20000562
    030c  2000058c  f0 bd       pop      {r4, r5, r6, r7, pc}
    030e  2000058e  c0 46       mov      r8, r8
    0310  20000590  6c 11       asrs     r4, r5, #5
    0312  20000592  00 20       movs     r0, #0
    0314  20000594  f0 b5       push     {r4, r5, r6, r7, lr}
    0316  20000596  07 00       movs     r7, r0
    0318  20000598  0e 00       movs     r6, r1
    031a  2000059a  15 00       movs     r5, r2
    031c  2000059c  87 b0       sub      sp, #0x1c
    031e  2000059e  08 2a       cmp      r2, #8
    0320  200005a0  00 d1       bne      #0x200005a4
    0322  200005a2  f3 e0       b        #0x2000078c
    0324  200005a4  2c d8       bhi      #0x20000600
    0326  200005a6  04 2a       cmp      r2, #4
    0328  200005a8  00 d1       bne      #0x200005ac
    032a  200005aa  9a e0       b        #0x200006e2
    032c  200005ac  1d d8       bhi      #0x200005ea
    032e  200005ae  02 2a       cmp      r2, #2
    0330  200005b0  00 d1       bne      #0x200005b4
    0332  200005b2  8e e0       b        #0x200006d2
    0334  200005b4  05 d8       bhi      #0x200005c2
    0336  200005b6  00 2a       cmp      r2, #0
    0338  200005b8  4c d0       beq      #0x20000654
    033a  200005ba  01 2a       cmp      r2, #1
    033c  200005bc  54 d0       beq      #0x20000668
    033e  200005be  07 b0       add      sp, #0x1c
    0340  200005c0  f0 bd       pop      {r4, r5, r6, r7, pc}
    0342  200005c2  03 2a       cmp      r2, #3
    0344  200005c4  fb d1       bne      #0x200005be
    0346  200005c6  0a 1d       adds     r2, r1, #4
    0348  200005c8  2e 21       movs     r1, #0x2e
    034a  200005ca  02 a8       add      r0, sp, #8
    034c  200005cc  33 78       ldrb     r3, [r6]
    034e  200005ce  20 2b       cmp      r3, #0x20
    0350  200005d0  06 d0       beq      #0x200005e0
    0352  200005d2  1c 00       movs     r4, r3
    0354  200005d4  20 3c       subs     r4, #0x20
    0356  200005d6  5e 2c       cmp      r4, #0x5e
    0358  200005d8  00 d9       bls      #0x200005dc
    035a  200005da  0b 00       movs     r3, r1
    035c  200005dc  03 70       strb     r3, [r0]
    035e  200005de  01 30       adds     r0, #1
    0360  200005e0  01 36       adds     r6, #1
    0362  200005e2  96 42       cmp      r6, r2
    0364  200005e4  f2 d1       bne      #0x200005cc
    0366  200005e6  00 24       movs     r4, #0
    0368  200005e8  45 e0       b        #0x20000676
    036a  200005ea  06 2a       cmp      r2, #6
    036c  200005ec  00 d1       bne      #0x200005f0
    036e  200005ee  8e e0       b        #0x2000070e
    0370  200005f0  07 2a       cmp      r2, #7
    0372  200005f2  00 d1       bne      #0x200005f6
    0374  200005f4  8d e0       b        #0x20000712
    0376  200005f6  05 2a       cmp      r2, #5
    0378  200005f8  e1 d1       bne      #0x200005be
    037a  200005fa  89 7d       ldrb     r1, [r1, #0x16]
    037c  200005fc  02 a8       add      r0, sp, #8
    037e  200005fe  17 e0       b        #0x20000630
    0380  20000600  0d 2a       cmp      r2, #0xd
    0382  20000602  25 d0       beq      #0x20000650
    0384  20000604  1b d8       bhi      #0x2000063e
    0386  20000606  0b 2a       cmp      r2, #0xb
    0388  20000608  00 d1       bne      #0x2000060c
    038a  2000060a  d3 e0       b        #0x200007b4
    038c  2000060c  13 d8       bhi      #0x20000636
    038e  2000060e  09 2a       cmp      r2, #9
    0390  20000610  00 d1       bne      #0x20000614
    0392  20000612  c3 e0       b        #0x2000079c
    0394  20000614  0a 2a       cmp      r2, #0xa
    0396  20000616  d2 d1       bne      #0x200005be
    0398  20000618  0c 8a       ldrh     r4, [r1, #0x10]
    039a  2000061a  00 21       movs     r1, #0
    039c  2000061c  09 2c       cmp      r4, #9
    039e  2000061e  00 d9       bls      #0x20000622
    03a0  20000620  c5 e0       b        #0x200007ae
    03a2  20000622  02 a8       add      r0, sp, #8
    03a4  20000624  ff f7 98 ff bl       #0x20000558
    03a8  20000628  2e 23       movs     r3, #0x2e
    03aa  2000062a  21 00       movs     r1, r4
    03ac  2000062c  03 70       strb     r3, [r0]
    03ae  2000062e  01 30       adds     r0, #1
    03b0  20000630  ff f7 92 ff bl       #0x20000558
    03b4  20000634  d7 e7       b        #0x200005e6
    03b6  20000636  0c 2a       cmp      r2, #0xc
    03b8  20000638  c1 d1       bne      #0x200005be
    03ba  2000063a  49 7f       ldrb     r1, [r1, #0x1d]
    03bc  2000063c  de e7       b        #0x200005fc
    03be  2000063e  0f 2a       cmp      r2, #0xf
    03c0  20000640  00 d1       bne      #0x20000644
    03c2  20000642  b9 e0       b        #0x200007b8
    03c4  20000644  10 2a       cmp      r2, #0x10
    03c6  20000646  00 d1       bne      #0x2000064a
    03c8  20000648  b8 e0       b        #0x200007bc
    03ca  2000064a  01 36       adds     r6, #1
    03cc  2000064c  0e 2a       cmp      r2, #0xe
    03ce  2000064e  b6 d1       bne      #0x200005be
    03d0  20000650  f1 7f       ldrb     r1, [r6, #0x1f]
    03d2  20000652  d3 e7       b        #0x200005fc
    03d4  20000654  03 36       adds     r6, #3
    03d6  20000656  f3 7f       ldrb     r3, [r6, #0x1f]
    03d8  20000658  62 4c       ldr      r4, [pc, #0x188]
    03da  2000065a  02 a8       add      r0, sp, #8
    03dc  2000065c  06 2b       cmp      r3, #6
    03de  2000065e  0a d8       bhi      #0x20000676
    03e0  20000660  61 4a       ldr      r2, [pc, #0x184]
    03e2  20000662  9b 00       lsls     r3, r3, #2
    03e4  20000664  9c 58       ldr      r4, [r3, r2]
    03e6  20000666  06 e0       b        #0x20000676
    03e8  20000668  04 36       adds     r6, #4
    03ea  2000066a  f3 7f       ldrb     r3, [r6, #0x1f]
    03ec  2000066c  5d 4c       ldr      r4, [pc, #0x174]
    03ee  2000066e  9a 1e       subs     r2, r3, #2
    03f0  20000670  02 a8       add      r0, sp, #8
    03f2  20000672  04 2a       cmp      r2, #4
    03f4  20000674  f4 d9       bls      #0x20000660
    03f6  20000676  00 23       movs     r3, #0
    03f8  20000678  03 70       strb     r3, [r0]
    03fa  2000067a  5c 4b       ldr      r3, [pc, #0x170]
    03fc  2000067c  58 5d       ldrb     r0, [r3, r5]
    03fe  2000067e  5c 4b       ldr      r3, [pc, #0x170]
    0400  20000680  18 18       adds     r0, r3, r0
    0402  20000682  00 2c       cmp      r4, #0
    0404  20000684  00 d1       bne      #0x20000688
    0406  20000686  02 ac       add      r4, sp, #8
    0408  20000688  00 21       movs     r1, #0
    040a  2000068a  45 5c       ldrb     r5, [r0, r1]
    040c  2000068c  00 2d       cmp      r5, #0
    040e  2000068e  00 d0       beq      #0x20000692
    0410  20000690  a5 e0       b        #0x200007de
    0412  20000692  07 23       movs     r3, #7
    0414  20000694  59 43       muls     r1, r3, r1
    0416  20000696  cb b2       uxtb     r3, r1
    0418  20000698  01 93       str      r3, [sp, #4]
    041a  2000069a  56 4b       ldr      r3, [pc, #0x158]
    041c  2000069c  2a 00       movs     r2, r5
    041e  2000069e  1b 68       ldr      r3, [r3]
    0420  200006a0  29 00       movs     r1, r5
    0422  200006a2  db 6b       ldr      r3, [r3, #0x3c]
    0424  200006a4  1e 00       movs     r6, r3
    0426  200006a6  3b 00       movs     r3, r7
    0428  200006a8  b0 47       blx      r6
    042a  200006aa  52 4b       ldr      r3, [pc, #0x148]
    042c  200006ac  2a 00       movs     r2, r5
    042e  200006ae  1b 68       ldr      r3, [r3]

### 0x05c0..0x0800 — application frame parse / field decode

    05c0  20000840  23 1d       adds     r3, r4, #4
    05c2  20000842  64 c8       ldm      r0!, {r2, r5, r6}
    05c4  20000844  64 c3       stm      r3!, {r2, r5, r6}
    05c6  20000846  64 c8       ldm      r0!, {r2, r5, r6}
    05c8  20000848  64 c3       stm      r3!, {r2, r5, r6}
    05ca  2000084a  64 c8       ldm      r0!, {r2, r5, r6}
    05cc  2000084c  64 c3       stm      r3!, {r2, r5, r6}
    05ce  2000084e  02 68       ldr      r2, [r0]
    05d0  20000850  1a 60       str      r2, [r3]
    05d2  20000852  23 00       movs     r3, r4
    05d4  20000854  b5 33       adds     r3, #0xb5
    05d6  20000856  19 70       strb     r1, [r3]
    05d8  20000858  23 00       movs     r3, r4
    05da  2000085a  b6 33       adds     r3, #0xb6
    05dc  2000085c  19 70       strb     r1, [r3]
    05de  2000085e  23 00       movs     r3, r4
    05e0  20000860  b8 33       adds     r3, #0xb8
    05e2  20000862  1a 78       ldrb     r2, [r3]
    05e4  20000864  00 2a       cmp      r2, #0
    05e6  20000866  04 d1       bne      #0x20000872
    05e8  20000868  10 32       adds     r2, #0x10
    05ea  2000086a  1a 70       strb     r2, [r3]
    05ec  2000086c  23 00       movs     r3, r4
    05ee  2000086e  ba 33       adds     r3, #0xba
    05f0  20000870  19 80       strh     r1, [r3]
    05f2  20000872  23 68       ldr      r3, [r4]
    05f4  20000874  04 33       adds     r3, #4
    05f6  20000876  db 6f       ldr      r3, [r3, #0x7c]
    05f8  20000878  98 47       blx      r3
    05fa  2000087a  f8 bd       pop      {r3, r4, r5, r6, r7, pc}
    05fc  2000087c  2b 00       movs     r3, r5
    05fe  2000087e  2a 1d       adds     r2, r5, #4
    0600  20000880  30 33       adds     r3, #0x30
    0602  20000882  c1 ca       ldm      r2!, {r0, r6, r7}
    0604  20000884  c1 c3       stm      r3!, {r0, r6, r7}
    0606  20000886  c1 ca       ldm      r2!, {r0, r6, r7}
    0608  20000888  c1 c3       stm      r3!, {r0, r6, r7}
    060a  2000088a  c1 ca       ldm      r2!, {r0, r6, r7}
    060c  2000088c  c1 c3       stm      r3!, {r0, r6, r7}
    060e  2000088e  01 39       subs     r1, #1
    0610  20000890  41 ca       ldm      r2!, {r0, r6}
    0612  20000892  41 c3       stm      r3!, {r0, r6}
    0614  20000894  c9 b2       uxtb     r1, r1
    0616  20000896  cf e7       b        #0x20000838
    0618  20000898  a4 11       asrs     r4, r4, #6
    061a  2000089a  00 20       movs     r0, #0
    061c  2000089c  f7 b5       push     {r0, r1, r2, r4, r5, r6, r7, lr}
    061e  2000089e  2c 29       cmp      r1, #0x2c
    0620  200008a0  01 d8       bhi      #0x200008a6
    0622  200008a2  00 20       movs     r0, #0
    0624  200008a4  fe bd       pop      {r1, r2, r3, r4, r5, r6, r7, pc}
    0626  200008a6  03 78       ldrb     r3, [r0]
    0628  200008a8  0c 2b       cmp      r3, #0xc
    062a  200008aa  fa d1       bne      #0x200008a2
    062c  200008ac  43 78       ldrb     r3, [r0, #1]
    062e  200008ae  2b 2b       cmp      r3, #0x2b
    0630  200008b0  f7 d1       bne      #0x200008a2
    0632  200008b2  83 7a       ldrb     r3, [r0, #0xa]
    0634  200008b4  20 2b       cmp      r3, #0x20
    0636  200008b6  f4 d1       bne      #0x200008a2
    0638  200008b8  c3 79       ldrb     r3, [r0, #7]
    063a  200008ba  ff 2b       cmp      r3, #0xff
    063c  200008bc  07 d1       bne      #0x200008ce
    063e  200008be  03 7a       ldrb     r3, [r0, #8]
    0640  200008c0  ff 2b       cmp      r3, #0xff
    0642  200008c2  ee d1       bne      #0x200008a2
    0644  200008c4  43 7a       ldrb     r3, [r0, #9]
    0646  200008c6  ff 2b       cmp      r3, #0xff
    0648  200008c8  eb d1       bne      #0x200008a2
    064a  200008ca  01 26       movs     r6, #1
    064c  200008cc  08 e0       b        #0x200008e0
    064e  200008ce  1f 2b       cmp      r3, #0x1f
    0650  200008d0  e7 d1       bne      #0x200008a2
    0652  200008d2  03 7a       ldrb     r3, [r0, #8]
    0654  200008d4  8c 2b       cmp      r3, #0x8c
    0656  200008d6  e4 d1       bne      #0x200008a2
    0658  200008d8  43 7a       ldrb     r3, [r0, #9]
    065a  200008da  00 26       movs     r6, #0
    065c  200008dc  30 2b       cmp      r3, #0x30
    065e  200008de  e0 d1       bne      #0x200008a2
    0660  200008e0  05 00       movs     r5, r0
    0662  200008e2  2b 35       adds     r5, #0x2b
    0664  200008e4  6b 78       ldrb     r3, [r5, #1]
    0666  200008e6  2c 78       ldrb     r4, [r5]
    0668  200008e8  1b 02       lsls     r3, r3, #8
    066a  200008ea  23 43       orrs     r3, r4
    066c  200008ec  5b ba       rev16    r3, r3
    066e  200008ee  9b b2       uxth     r3, r3
    0670  200008f0  01 93       str      r3, [sp, #4]
    0672  200008f2  04 00       movs     r4, r0
    0674  200008f4  00 23       movs     r3, #0
    0676  200008f6  a5 42       cmp      r5, r4
    0678  200008f8  00 d0       beq      #0x200008fc
    067a  200008fa  a2 e0       b        #0x20000a42
    067c  200008fc  01 99       ldr      r1, [sp, #4]
    067e  200008fe  8b 42       cmp      r3, r1
    0680  20000900  cf d1       bne      #0x200008a2
    0682  20000902  0f 21       movs     r1, #0xf
    0684  20000904  c3 7a       ldrb     r3, [r0, #0xb]
    0686  20000906  03 24       movs     r4, #3
    0688  20000908  0b 40       ands     r3, r1
    068a  2000090a  d1 1c       adds     r1, r2, #3
    068c  2000090c  cb 77       strb     r3, [r1, #0x1f]
    068e  2000090e  07 21       movs     r1, #7
    0690  20000910  03 7b       ldrb     r3, [r0, #0xc]
    0692  20000912  56 4d       ldr      r5, [pc, #0x158]
    0694  20000914  0b 40       ands     r3, r1
    0696  20000916  11 1d       adds     r1, r2, #4
    0698  20000918  cb 77       strb     r3, [r1, #0x1f]
    069a  2000091a  43 7b       ldrb     r3, [r0, #0xd]
    069c  2000091c  23 40       ands     r3, r4
    069e  2000091e  13 76       strb     r3, [r2, #0x18]
    06a0  20000920  83 7b       ldrb     r3, [r0, #0xe]
    06a2  20000922  13 70       strb     r3, [r2]
    06a4  20000924  c3 7b       ldrb     r3, [r0, #0xf]
    06a6  20000926  53 70       strb     r3, [r2, #1]
    06a8  20000928  03 7c       ldrb     r3, [r0, #0x10]
    06aa  2000092a  93 70       strb     r3, [r2, #2]
    06ac  2000092c  43 7c       ldrb     r3, [r0, #0x11]
    06ae  2000092e  d3 70       strb     r3, [r2, #3]
    06b0  20000930  c3 7c       ldrb     r3, [r0, #0x13]
    06b2  20000932  01 7d       ldrb     r1, [r0, #0x14]
    06b4  20000934  1b 02       lsls     r3, r3, #8
    06b6  20000936  09 04       lsls     r1, r1, #0x10
    06b8  20000938  0b 43       orrs     r3, r1
    06ba  2000093a  81 7c       ldrb     r1, [r0, #0x12]
    06bc  2000093c  0b 43       orrs     r3, r1
    06be  2000093e  53 60       str      r3, [r2, #4]
    06c0  20000940  00 21       movs     r1, #0
    06c2  20000942  ab 42       cmp      r3, r5
    06c4  20000944  13 d8       bhi      #0x2000096e
    06c6  20000946  4a 4d       ldr      r5, [pc, #0x128]
    06c8  20000948  01 31       adds     r1, #1
    06ca  2000094a  ab 42       cmp      r3, r5
    06cc  2000094c  0f d8       bhi      #0x2000096e
    06ce  2000094e  49 4d       ldr      r5, [pc, #0x124]
    06d0  20000950  01 31       adds     r1, #1
    06d2  20000952  ab 42       cmp      r3, r5
    06d4  20000954  0b d8       bhi      #0x2000096e
    06d6  20000956  fa 25       movs     r5, #0xfa
    06d8  20000958  21 00       movs     r1, r4
    06da  2000095a  ad 00       lsls     r5, r5, #2
    06dc  2000095c  ab 42       cmp      r3, r5
    06de  2000095e  06 d2       bhs      #0x2000096e
    06e0  20000960  01 31       adds     r1, #1
    06e2  20000962  63 2b       cmp      r3, #0x63
    06e4  20000964  03 d8       bhi      #0x2000096e
    06e6  20000966  01 31       adds     r1, #1
    06e8  20000968  09 2b       cmp      r3, #9
    06ea  2000096a  00 d8       bhi      #0x2000096e
    06ec  2000096c  01 31       adds     r1, #1
    06ee  2000096e  42 4c       ldr      r4, [pc, #0x108]
    06f0  20000970  89 00       lsls     r1, r1, #2
    06f2  20000972  0c 59       ldr      r4, [r1, r4]
    06f4  20000974  00 21       movs     r1, #0
    06f6  20000976  9c 42       cmp      r4, r3
    06f8  20000978  74 d9       bls      #0x20000a64
    06fa  2000097a  09 29       cmp      r1, #9
    06fc  2000097c  00 d1       bne      #0x20000980
    06fe  2000097e  90 e7       b        #0x200008a2
    0700  20000980  43 7d       ldrb     r3, [r0, #0x15]
    0702  20000982  93 75       strb     r3, [r2, #0x16]
    0704  20000984  83 7d       ldrb     r3, [r0, #0x16]
    0706  20000986  d3 75       strb     r3, [r2, #0x17]
    0708  20000988  03 7e       ldrb     r3, [r0, #0x18]
    070a  2000098a  c1 7d       ldrb     r1, [r0, #0x17]
    070c  2000098c  1b 02       lsls     r3, r3, #8
    070e  2000098e  0b 43       orrs     r3, r1
    0710  20000990  93 81       strh     r3, [r2, #0xc]
    0712  20000992  41 7e       ldrb     r1, [r0, #0x19]
    0714  20000994  01 23       movs     r3, #1
    0716  20000996  ff 29       cmp      r1, #0xff
    0718  20000998  07 d1       bne      #0x200009aa
    071a  2000099a  81 7e       ldrb     r1, [r0, #0x1a]
    071c  2000099c  ff 29       cmp      r1, #0xff
    071e  2000099e  04 d1       bne      #0x200009aa
    0720  200009a0  c3 7e       ldrb     r3, [r0, #0x1b]
    0722  200009a2  ff 3b       subs     r3, #0xff
    0724  200009a4  59 1e       subs     r1, r3, #1
    0726  200009a6  8b 41       sbcs     r3, r1
    0728  200009a8  db b2       uxtb     r3, r3
    072a  200009aa  01 21       movs     r1, #1
    072c  200009ac  fc 24       movs     r4, #0xfc
    072e  200009ae  53 76       strb     r3, [r2, #0x19]
    0730  200009b0  c3 7e       ldrb     r3, [r0, #0x1b]
    0732  200009b2  a4 03       lsls     r4, r4, #0xe
    0734  200009b4  db 09       lsrs     r3, r3, #7
    0736  200009b6  93 76       strb     r3, [r2, #0x1a]
    0738  200009b8  c3 7e       ldrb     r3, [r0, #0x1b]
    073a  200009ba  9b 09       lsrs     r3, r3, #6
    073c  200009bc  0b 40       ands     r3, r1
    073e  200009be  d3 76       strb     r3, [r2, #0x1b]
    0740  200009c0  c3 7e       ldrb     r3, [r0, #0x1b]
    0742  200009c2  45 7e       ldrb     r5, [r0, #0x19]
    0744  200009c4  1b 04       lsls     r3, r3, #0x10
    0746  200009c6  23 40       ands     r3, r4
    0748  200009c8  84 7e       ldrb     r4, [r0, #0x1a]
    074a  200009ca  24 02       lsls     r4, r4, #8
    074c  200009cc  2c 43       orrs     r4, r5
    074e  200009ce  23 43       orrs     r3, r4
    0750  200009d0  93 60       str      r3, [r2, #8]
    0752  200009d2  43 7f       ldrb     r3, [r0, #0x1d]
    0754  200009d4  04 7f       ldrb     r4, [r0, #0x1c]
    0756  200009d6  1b 02       lsls     r3, r3, #8
    0758  200009d8  23 43       orrs     r3, r4
    075a  200009da  9b 05       lsls     r3, r3, #0x16
    075c  200009dc  9b 0d       lsrs     r3, r3, #0x16
    075e  200009de  93 82       strh     r3, [r2, #0x14]
    0760  200009e0  03 00       movs     r3, r0
    0762  200009e2  1f 33       adds     r3, #0x1f
    0764  200009e4  5b 78       ldrb     r3, [r3, #1]
    0766  200009e6  c4 7f       ldrb     r4, [r0, #0x1f]
    0768  200009e8  1b 02       lsls     r3, r3, #8
    076a  200009ea  23 43       orrs     r3, r4
    076c  200009ec  04 00       movs     r4, r0
    076e  200009ee  d3 81       strh     r3, [r2, #0xe]
    0770  200009f0  21 34       adds     r4, #0x21
    0772  200009f2  63 78       ldrb     r3, [r4, #1]
    0774  200009f4  25 78       ldrb     r5, [r4]
    0776  200009f6  1b 02       lsls     r3, r3, #8
    0778  200009f8  2b 43       orrs     r3, r5
    077a  200009fa  13 82       strh     r3, [r2, #0x10]
    077c  200009fc  03 1d       adds     r3, r0, #4
    077e  200009fe  db 7f       ldrb     r3, [r3, #0x1f]
    0780  20000a00  13 77       strb     r3, [r2, #0x1c]
    0782  20000a02  43 1d       adds     r3, r0, #5
    0784  20000a04  db 7f       ldrb     r3, [r3, #0x1f]
    0786  20000a06  53 77       strb     r3, [r2, #0x1d]
    0788  20000a08  83 1d       adds     r3, r0, #6
    078a  20000a0a  db 7f       ldrb     r3, [r3, #0x1f]
    078c  20000a0c  93 77       strb     r3, [r2, #0x1e]
    078e  20000a0e  c3 1d       adds     r3, r0, #7
    0790  20000a10  db 7f       ldrb     r3, [r3, #0x1f]
    0792  20000a12  d3 77       strb     r3, [r2, #0x1f]
    0794  20000a14  03 00       movs     r3, r0
    0796  20000a16  08 33       adds     r3, #8
    0798  20000a18  dc 7f       ldrb     r4, [r3, #0x1f]
    079a  20000a1a  53 18       adds     r3, r2, r1
    079c  20000a1c  dc 77       strb     r4, [r3, #0x1f]
    079e  20000a1e  03 00       movs     r3, r0
    07a0  20000a20  28 33       adds     r3, #0x28
    07a2  20000a22  1c 78       ldrb     r4, [r3]
    07a4  20000a24  5b 18       adds     r3, r3, r1
    07a6  20000a26  1b 78       ldrb     r3, [r3]
    07a8  20000a28  2a 30       adds     r0, #0x2a
    07aa  20000a2a  1b 02       lsls     r3, r3, #8
    07ac  20000a2c  23 43       orrs     r3, r4
    07ae  20000a2e  53 82       strh     r3, [r2, #0x12]
    07b0  20000a30  00 78       ldrb     r0, [r0]
    07b2  20000a32  93 1c       adds     r3, r2, #2
    07b4  20000a34  d8 77       strb     r0, [r3, #0x1f]
    07b6  20000a36  53 1d       adds     r3, r2, #5
    07b8  20000a38  06 32       adds     r2, #6
    07ba  20000a3a  de 77       strb     r6, [r3, #0x1f]
    07bc  20000a3c  08 00       movs     r0, r1
    07be  20000a3e  d1 77       strb     r1, [r2, #0x1f]
    07c0  20000a40  30 e7       b        #0x200008a4
    07c2  20000a42  21 78       ldrb     r1, [r4]
    07c4  20000a44  09 02       lsls     r1, r1, #8
    07c6  20000a46  4b 40       eors     r3, r1
    07c8  20000a48  08 21       movs     r1, #8
    07ca  20000a4a  1f b2       sxth     r7, r3
    07cc  20000a4c  5b 00       lsls     r3, r3, #1
    07ce  20000a4e  9b b2       uxth     r3, r3
    07d0  20000a50  00 2f       cmp      r7, #0
    07d2  20000a52  01 da       bge      #0x20000a58
    07d4  20000a54  09 4f       ldr      r7, [pc, #0x24]
    07d6  20000a56  7b 40       eors     r3, r7
    07d8  20000a58  01 39       subs     r1, #1
    07da  20000a5a  c9 b2       uxtb     r1, r1
    07dc  20000a5c  00 29       cmp      r1, #0
    07de  20000a5e  f4 d1       bne      #0x20000a4a
    07e0  20000a60  01 34       adds     r4, #1
    07e2  20000a62  48 e7       b        #0x200008f6
    07e4  20000a64  01 31       adds     r1, #1
    07e6  20000a66  1b 1b       subs     r3, r3, r4
    07e8  20000a68  c9 b2       uxtb     r1, r1
    07ea  20000a6a  84 e7       b        #0x20000976
    07ec  20000a6c  3f 42       tst      r7, r7
    07ee  20000a6e  0f 00       movs     r7, r1
    07f0  20000a70  9f 86       strh     r7, [r3, #0x34]
    07f2  20000a72  01 00       movs     r1, r0
    07f4  20000a74  0f 27       movs     r7, #0xf
    07f6  20000a76  00 00       movs     r0, r0
    07f8  20000a78  6c 11       asrs     r4, r5, #5
    07fa  20000a7a  00 20       movs     r0, #0
    07fc  20000a7c  21 10       asrs     r1, r4, #0x20
    07fe  20000a7e  00 00       movs     r0, r0

### 0x08f0..0x0b30 — audio DSP around recovered offset 0x950

    08f0  20000b70  02 aa       add      r2, sp, #8
    08f2  20000b72  ff f7 93 fe bl       #0x2000089c
    08f6  20000b76  00 28       cmp      r0, #0
    08f8  20000b78  b9 d0       beq      #0x20000aee
    08fa  20000b7a  00 2d       cmp      r5, #0
    08fc  20000b7c  b7 d0       beq      #0x20000aee
    08fe  20000b7e  02 a8       add      r0, sp, #8
    0900  20000b80  a8 47       blx      r5
    0902  20000b82  b4 e7       b        #0x20000aee
    0904  20000b84  9f 25       movs     r5, #0x9f
    0906  20000b86  d4 0d       lsrs     r4, r2, #0x17
    0908  20000b88  60 da       bge      #0x20000c4c
    090a  20000b8a  2b 72       strb     r3, [r5, #8]
    090c  20000b8c  60 da       bge      #0x20000c50
    090e  20000b8e  2b f2 00 ff bl       #0x2022c992
    0912  20000b92  ff 00       lsls     r7, r7, #3
    0914  20000b94  00 0c       lsrs     r0, r0, #0x10
    0916  20000b96  2b 00       movs     r3, r5
    0918  20000b98  02 00       movs     r2, r0
    091a  20000b9a  03 00       movs     r3, r0
    091c  20000b9c  00 21       movs     r1, #0
    091e  20000b9e  09 32       adds     r2, #9
    0920  20000ba0  ff 32       adds     r2, #0xff
    0922  20000ba2  19 70       strb     r1, [r3]
    0924  20000ba4  01 33       adds     r3, #1
    0926  20000ba6  93 42       cmp      r3, r2
    0928  20000ba8  fb d1       bne      #0x20000ba2
    092a  20000baa  80 23       movs     r3, #0x80
    092c  20000bac  1b 02       lsls     r3, r3, #8
    092e  20000bae  03 60       str      r3, [r0]
    0930  20000bb0  10 23       movs     r3, #0x10
    0932  20000bb2  83 62       str      r3, [r0, #0x28]
    0934  20000bb4  43 62       str      r3, [r0, #0x24]
    0936  20000bb6  03 00       movs     r3, r0
    0938  20000bb8  03 4a       ldr      r2, [pc, #0xc]
    093a  20000bba  78 33       adds     r3, #0x78
    093c  20000bbc  1a 80       strh     r2, [r3]
    093e  20000bbe  03 4b       ldr      r3, [pc, #0xc]
    0940  20000bc0  84 30       adds     r0, #0x84
    0942  20000bc2  03 80       strh     r3, [r0]
    0944  20000bc4  70 47       bx       lr
    0946  20000bc6  c0 46       mov      r8, r8
    0948  20000bc8  02 03       lsls     r2, r0, #0xc
    094a  20000bca  00 00       movs     r0, r0
    094c  20000bcc  03 02       lsls     r3, r0, #8
    094e  20000bce  00 00       movs     r0, r0
    0950  20000bd0  f0 b5       push     {r4, r5, r6, r7, lr}
    0952  20000bd2  04 00       movs     r4, r0
    0954  20000bd4  87 b0       sub      sp, #0x1c
    0956  20000bd6  05 92       str      r2, [sp, #0x14]
    0958  20000bd8  02 68       ldr      r2, [r0]
    095a  20000bda  0b 01       lsls     r3, r1, #4
    095c  20000bdc  9b 1a       subs     r3, r3, r2
    095e  20000bde  9b 11       asrs     r3, r3, #6
    0960  20000be0  9b 18       adds     r3, r3, r2
    0962  20000be2  03 60       str      r3, [r0]
    0964  20000be4  1b 11       asrs     r3, r3, #4
    0966  20000be6  c9 1a       subs     r1, r1, r3
    0968  20000be8  83 68       ldr      r3, [r0, #8]
    096a  20000bea  c2 68       ldr      r2, [r0, #0xc]
    096c  20000bec  c8 1a       subs     r0, r1, r3
    096e  20000bee  50 4b       ldr      r3, [pc, #0x140]
    0970  20000bf0  25 69       ldr      r5, [r4, #0x10]
    0972  20000bf2  43 43       muls     r3, r0, r3
    0974  20000bf4  4f 48       ldr      r0, [pc, #0x13c]
    0976  20000bf6  22 61       str      r2, [r4, #0x10]
    0978  20000bf8  50 43       muls     r0, r2, r0
    097a  20000bfa  22 00       movs     r2, r4
    097c  20000bfc  1b 18       adds     r3, r3, r0
    097e  20000bfe  4e 48       ldr      r0, [pc, #0x138]
    0980  20000c00  2c 32       adds     r2, #0x2c
    0982  20000c02  68 43       muls     r0, r5, r0
    0984  20000c04  1b 18       adds     r3, r3, r0
    0986  20000c06  60 68       ldr      r0, [r4, #4]
    0988  20000c08  9b 13       asrs     r3, r3, #0xe
    098a  20000c0a  a0 60       str      r0, [r4, #8]
    098c  20000c0c  61 60       str      r1, [r4, #4]
    098e  20000c0e  e3 60       str      r3, [r4, #0xc]
    0990  20000c10  03 92       str      r2, [sp, #0xc]
    0992  20000c12  12 78       ldrb     r2, [r2]
    0994  20000c14  49 48       ldr      r0, [pc, #0x124]
    0996  20000c16  d6 00       lsls     r6, r2, #3
    0998  20000c18  81 56       ldrsb    r1, [r0, r2]
    099a  20000c1a  2e 36       adds     r6, #0x2e
    099c  20000c1c  59 43       muls     r1, r3, r1
    099e  20000c1e  a5 19       adds     r5, r4, r6
    09a0  20000c20  a7 5f       ldrsh    r7, [r4, r6]
    09a2  20000c22  09 12       asrs     r1, r1, #8
    09a4  20000c24  66 69       ldr      r6, [r4, #0x14]
    09a6  20000c26  cf 1b       subs     r7, r1, r7
    09a8  20000c28  be 19       adds     r6, r7, r6
    09aa  20000c2a  66 61       str      r6, [r4, #0x14]
    09ac  20000c2c  01 96       str      r6, [sp, #4]
    09ae  20000c2e  07 26       movs     r6, #7
    09b0  20000c30  29 80       strh     r1, [r5]
    09b2  20000c32  91 1d       adds     r1, r2, #6
    09b4  20000c34  31 40       ands     r1, r6
    09b6  20000c36  40 56       ldrsb    r0, [r0, r1]
    09b8  20000c38  02 26       movs     r6, #2
    09ba  20000c3a  a9 5f       ldrsh    r1, [r5, r6]
    09bc  20000c3c  58 43       muls     r0, r3, r0
    09be  20000c3e  a6 69       ldr      r6, [r4, #0x18]
    09c0  20000c40  00 12       asrs     r0, r0, #8
    09c2  20000c42  41 1a       subs     r1, r0, r1
    09c4  20000c44  89 19       adds     r1, r1, r6
    09c6  20000c46  a1 61       str      r1, [r4, #0x18]
    09c8  20000c48  68 80       strh     r0, [r5, #2]
    09ca  20000c4a  20 00       movs     r0, r4
    09cc  20000c4c  2d 30       adds     r0, #0x2d
    09ce  20000c4e  02 91       str      r1, [sp, #8]
    09d0  20000c50  04 90       str      r0, [sp, #0x10]
    09d2  20000c52  00 78       ldrb     r0, [r0]
    09d4  20000c54  3a 4e       ldr      r6, [pc, #0xe8]
    09d6  20000c56  04 27       movs     r7, #4
    09d8  20000c58  e9 5f       ldrsh    r1, [r5, r7]
    09da  20000c5a  36 56       ldrsb    r6, [r6, r0]
    09dc  20000c5c  01 32       adds     r2, #1
    09de  20000c5e  5e 43       muls     r6, r3, r6
    09e0  20000c60  36 12       asrs     r6, r6, #8
    09e2  20000c62  71 1a       subs     r1, r6, r1
    09e4  20000c64  8c 46       mov      ip, r1
    09e6  20000c66  e1 69       ldr      r1, [r4, #0x1c]
    09e8  20000c68  90 27       movs     r7, #0x90
    09ea  20000c6a  8c 44       add      ip, r1
    09ec  20000c6c  61 46       mov      r1, ip
    09ee  20000c6e  e1 61       str      r1, [r4, #0x1c]
    09f0  20000c70  0f 21       movs     r1, #0xf
    09f2  20000c72  ae 80       strh     r6, [r5, #4]
    09f4  20000c74  06 1d       adds     r6, r0, #4
    09f6  20000c76  8c 46       mov      ip, r1
    09f8  20000c78  0e 40       ands     r6, r1
    09fa  20000c7a  31 49       ldr      r1, [pc, #0xc4]
    09fc  20000c7c  01 30       adds     r0, #1
    09fe  20000c7e  8e 57       ldrsb    r6, [r1, r6]
    0a00  20000c80  3f 02       lsls     r7, r7, #8
    0a02  20000c82  73 43       muls     r3, r6, r3
    0a04  20000c84  06 21       movs     r1, #6
    0a06  20000c86  6e 5e       ldrsh    r6, [r5, r1]
    0a08  20000c88  1b 12       asrs     r3, r3, #8
    0a0a  20000c8a  21 6a       ldr      r1, [r4, #0x20]
    0a0c  20000c8c  9e 1b       subs     r6, r3, r6
    0a0e  20000c8e  8e 19       adds     r6, r1, r6
    0a10  20000c90  26 62       str      r6, [r4, #0x20]
    0a12  20000c92  07 26       movs     r6, #7
    0a14  20000c94  eb 80       strh     r3, [r5, #6]
    0a16  20000c96  03 9b       ldr      r3, [sp, #0xc]
    0a18  20000c98  32 40       ands     r2, r6
    0a1a  20000c9a  1a 70       strb     r2, [r3]
    0a1c  20000c9c  63 46       mov      r3, ip
    0a1e  20000c9e  22 00       movs     r2, r4
    0a20  20000ca0  18 40       ands     r0, r3
    0a22  20000ca2  04 9b       ldr      r3, [sp, #0x10]
    0a24  20000ca4  02 99       ldr      r1, [sp, #8]
    0a26  20000ca6  18 70       strb     r0, [r3]
    0a28  20000ca8  24 32       adds     r2, #0x24
    0a2a  20000caa  01 98       ldr      r0, [sp, #4]
    0a2c  20000cac  ff f7 b4 fb bl       #0x20000418
    0a30  20000cb0  22 00       movs     r2, r4
    0a32  20000cb2  05 00       movs     r5, r0
    0a34  20000cb4  21 6a       ldr      r1, [r4, #0x20]
    0a36  20000cb6  e0 69       ldr      r0, [r4, #0x1c]
    0a38  20000cb8  28 32       adds     r2, #0x28
    0a3a  20000cba  ff f7 ad fb bl       #0x20000418
    0a3e  20000cbe  a3 6a       ldr      r3, [r4, #0x28]
    0a40  20000cc0  6b 43       muls     r3, r5, r3
    0a42  20000cc2  25 00       movs     r5, r4
    0a44  20000cc4  01 93       str      r3, [sp, #4]
    0a46  20000cc6  63 6a       ldr      r3, [r4, #0x24]
    0a48  20000cc8  88 34       adds     r4, #0x88
    0a4a  20000cca  43 43       muls     r3, r0, r3
    0a4c  20000ccc  70 35       adds     r5, #0x70
    0a4e  20000cce  02 93       str      r3, [sp, #8]
    0a50  20000cd0  03 94       str      r4, [sp, #0xc]
    0a52  20000cd2  01 9b       ldr      r3, [sp, #4]
    0a54  20000cd4  29 7a       ldrb     r1, [r5, #8]
    0a56  20000cd6  02 9a       ldr      r2, [sp, #8]
    0a58  20000cd8  59 43       muls     r1, r3, r1
    0a5a  20000cda  6b 7a       ldrb     r3, [r5, #9]
    0a5c  20000cdc  2e 68       ldr      r6, [r5]
    0a5e  20000cde  53 43       muls     r3, r2, r3
    0a60  20000ce0  cb 1a       subs     r3, r1, r3
    0a62  20000ce2  80 21       movs     r1, #0x80
    0a64  20000ce4  6a 68       ldr      r2, [r5, #4]
    0a66  20000ce6  89 01       lsls     r1, r1, #6
    0a68  20000ce8  50 18       adds     r0, r2, r1
    0a6a  20000cea  2a 68       ldr      r2, [r5]
    0a6c  20000cec  d9 17       asrs     r1, r3, #0x1f
    0a6e  20000cee  d2 17       asrs     r2, r2, #0x1f
    0a70  20000cf0  c9 1a       subs     r1, r1, r3
    0a72  20000cf2  92 1b       subs     r2, r2, r6
    0a74  20000cf4  68 60       str      r0, [r5, #4]
    0a76  20000cf6  c9 0f       lsrs     r1, r1, #0x1f
    0a78  20000cf8  d2 0f       lsrs     r2, r2, #0x1f
    0a7a  20000cfa  8a 42       cmp      r2, r1
    0a7c  20000cfc  03 d0       beq      #0x20000d06
    0a7e  20000cfe  3a 1a       subs     r2, r7, r0
    0a80  20000d00  92 10       asrs     r2, r2, #2
    0a82  20000d02  12 18       adds     r2, r2, r0
    0a84  20000d04  6a 60       str      r2, [r5, #4]
    0a86  20000d06  80 22       movs     r2, #0x80
    0a88  20000d08  2b 60       str      r3, [r5]
    0a8a  20000d0a  6b 68       ldr      r3, [r5, #4]
    0a8c  20000d0c  52 02       lsls     r2, r2, #9
    0a8e  20000d0e  93 42       cmp      r3, r2
    0a90  20000d10  06 db       blt      #0x20000d20
    0a92  20000d12  0c 4a       ldr      r2, [pc, #0x30]
    0a94  20000d14  20 00       movs     r0, r4
    0a96  20000d16  9b 18       adds     r3, r3, r2
    0a98  20000d18  6b 60       str      r3, [r5, #4]
    0a9a  20000d1a  05 9a       ldr      r2, [sp, #0x14]
    0a9c  20000d1c  ff f7 b0 fe bl       #0x20000a80
    0aa0  20000d20  03 9b       ldr      r3, [sp, #0xc]
    0aa2  20000d22  0c 35       adds     r5, #0xc
    0aa4  20000d24  40 34       adds     r4, #0x40
    0aa6  20000d26  9d 42       cmp      r5, r3
    0aa8  20000d28  d3 d1       bne      #0x20000cd2
    0aaa  20000d2a  07 b0       add      sp, #0x1c
    0aac  20000d2c  f0 bd       pop      {r4, r5, r6, r7, pc}
    0aae  20000d2e  c0 46       mov      r8, r8
    0ab0  20000d30  0a 14       asrs     r2, r1, #0x10
    0ab2  20000d32  00 00       movs     r0, r0
    0ab4  20000d34  46 32       adds     r2, #0x46
    0ab6  20000d36  00 00       movs     r0, r0
    0ab8  20000d38  13 e8       .hword   0x13, 0xe8
    0aba  20000d3a  ff ff       .hword   0xff, 0xff
    0abc  20000d3c  a7 10       asrs     r7, r4, #2
    0abe  20000d3e  00 20       movs     r0, #0
    0ac0  20000d40  af 10       asrs     r7, r5, #2
    0ac2  20000d42  00 20       movs     r0, #0
    0ac4  20000d44  00 00       movs     r0, r0
    0ac6  20000d46  ff ff 00 2a vtbl.8   d18, {d15, d16, d17}, d0
    0aca  20000d4a  00 d1       bne      #0x20000d4e
    0acc  20000d4c  70 47       bx       lr
    0ace  20000d4e  01 3a       subs     r2, #1
    0ad0  20000d50  81 54       strb     r1, [r0, r2]
    0ad2  20000d52  f9 e7       b        #0x20000d48
    0ad4  20000d54  e0 22       movs     r2, #0xe0
    0ad6  20000d56  f0 b5       push     {r4, r5, r6, r7, lr}
    0ad8  20000d58  40 4c       ldr      r4, [pc, #0x100]
    0ada  20000d5a  00 21       movs     r1, #0
    0adc  20000d5c  27 68       ldr      r7, [r4]
    0ade  20000d5e  89 b0       sub      sp, #0x24
    0ae0  20000d60  78 68       ldr      r0, [r7, #4]
    0ae2  20000d62  92 00       lsls     r2, r2, #2
    0ae4  20000d64  ff f7 f0 ff bl       #0x20000d48
    0ae8  20000d68  23 00       movs     r3, r4
    0aea  20000d6a  b4 33       adds     r3, #0xb4
    0aec  20000d6c  1b 78       ldrb     r3, [r3]
    0aee  20000d6e  00 2b       cmp      r3, #0
    0af0  20000d70  36 d0       beq      #0x20000de0
    0af2  20000d72  23 00       movs     r3, r4
    0af4  20000d74  2c 25       movs     r5, #0x2c
    0af6  20000d76  00 26       movs     r6, #0
    0af8  20000d78  b5 33       adds     r3, #0xb5
    0afa  20000d7a  1b 78       ldrb     r3, [r3]
    0afc  20000d7c  5d 43       muls     r5, r3, r5
    0afe  20000d7e  04 35       adds     r5, #4
    0b00  20000d80  2d 19       adds     r5, r5, r4
    0b02  20000d82  23 00       movs     r3, r4
    0b04  20000d84  b6 33       adds     r3, #0xb6
    0b06  20000d86  1a 78       ldrb     r2, [r3]
    0b08  20000d88  06 23       movs     r3, #6
    0b0a  20000d8a  5a 43       muls     r2, r3, r2
    0b0c  20000d8c  34 4b       ldr      r3, [pc, #0xd0]
    0b0e  20000d8e  f0 b2       uxtb     r0, r6
    0b10  20000d90  9b 18       adds     r3, r3, r2
    0b12  20000d92  9a 5d       ldrb     r2, [r3, r6]
    0b14  20000d94  10 2a       cmp      r2, #0x10
    0b16  20000d96  02 d8       bhi      #0x20000d9e
    0b18  20000d98  29 00       movs     r1, r5
    0b1a  20000d9a  ff f7 fb fb bl       #0x20000594
    0b1e  20000d9e  01 36       adds     r6, #1
    0b20  20000da0  06 2e       cmp      r6, #6
    0b22  20000da2  ee d1       bne      #0x20000d82
    0b24  20000da4  50 23       movs     r3, #0x50
    0b26  20000da6  02 ad       add      r5, sp, #8
    0b28  20000da8  2b 70       strb     r3, [r5]
    0b2a  20000daa  23 00       movs     r3, r4
    0b2c  20000dac  09 20       movs     r0, #9
    0b2e  20000dae  b6 33       adds     r3, #0xb6

### 0x0b30..0x0e00 — display formatting / application UI

    0b30  20000db0  19 78       ldrb     r1, [r3]
    0b32  20000db2  68 44       add      r0, sp, r0
    0b34  20000db4  01 31       adds     r1, #1
    0b36  20000db6  ff f7 cf fb bl       #0x20000558
    0b3a  20000dba  2f 22       movs     r2, #0x2f
    0b3c  20000dbc  03 00       movs     r3, r0
    0b3e  20000dbe  03 21       movs     r1, #3
    0b40  20000dc0  1a 70       strb     r2, [r3]
    0b42  20000dc2  01 30       adds     r0, #1
    0b44  20000dc4  ff f7 c8 fb bl       #0x20000558
    0b48  20000dc8  00 22       movs     r2, #0
    0b4a  20000dca  07 21       movs     r1, #7
    0b4c  20000dcc  02 70       strb     r2, [r0]
    0b4e  20000dce  40 1b       subs     r0, r0, r5
    0b50  20000dd0  48 43       muls     r0, r1, r0
    0b52  20000dd2  87 39       subs     r1, #0x87
    0b54  20000dd4  09 1a       subs     r1, r1, r0
    0b56  20000dd6  13 00       movs     r3, r2
    0b58  20000dd8  28 00       movs     r0, r5
    0b5a  20000dda  c9 b2       uxtb     r1, r1
    0b5c  20000ddc  fd 6b       ldr      r5, [r7, #0x3c]
    0b5e  20000dde  a8 47       blx      r5
    0b60  20000de0  20 49       ldr      r1, [pc, #0x80]
    0b62  20000de2  02 a8       add      r0, sp, #8
    0b64  20000de4  ff f7 b0 fb bl       #0x20000548
    0b68  20000de8  23 00       movs     r3, r4
    0b6a  20000dea  be 33       adds     r3, #0xbe
    0b6c  20000dec  1b 78       ldrb     r3, [r3]
    0b6e  20000dee  1e 49       ldr      r1, [pc, #0x78]
    0b70  20000df0  00 2b       cmp      r3, #0
    0b72  20000df2  00 d1       bne      #0x20000df6
    0b74  20000df4  1d 49       ldr      r1, [pc, #0x74]
    0b76  20000df6  ff f7 a7 fb bl       #0x20000548
    0b7a  20000dfa  1d 49       ldr      r1, [pc, #0x74]
    0b7c  20000dfc  ff f7 a4 fb bl       #0x20000548
    0b80  20000e00  23 00       movs     r3, r4
    0b82  20000e02  bd 33       adds     r3, #0xbd
    0b84  20000e04  1b 78       ldrb     r3, [r3]
    0b86  20000e06  18 49       ldr      r1, [pc, #0x60]
    0b88  20000e08  00 2b       cmp      r3, #0
    0b8a  20000e0a  00 d1       bne      #0x20000e0e
    0b8c  20000e0c  17 49       ldr      r1, [pc, #0x5c]
    0b8e  20000e0e  ff f7 9b fb bl       #0x20000548
    0b92  20000e12  20 22       movs     r2, #0x20
    0b94  20000e14  03 00       movs     r3, r0
    0b96  20000e16  1a 70       strb     r2, [r3]
    0b98  20000e18  23 00       movs     r3, r4
    0b9a  20000e1a  00 21       movs     r1, #0
    0b9c  20000e1c  b4 33       adds     r3, #0xb4
    0b9e  20000e1e  1b 78       ldrb     r3, [r3]
    0ba0  20000e20  01 30       adds     r0, #1
    0ba2  20000e22  8b 42       cmp      r3, r1
    0ba4  20000e24  03 d0       beq      #0x20000e2e
    0ba6  20000e26  23 00       movs     r3, r4
    0ba8  20000e28  b5 33       adds     r3, #0xb5
    0baa  20000e2a  19 78       ldrb     r1, [r3]
    0bac  20000e2c  01 31       adds     r1, #1
    0bae  20000e2e  ff f7 93 fb bl       #0x20000558
    0bb2  20000e32  2f 22       movs     r2, #0x2f
    0bb4  20000e34  03 00       movs     r3, r0
    0bb6  20000e36  b4 34       adds     r4, #0xb4
    0bb8  20000e38  1a 70       strb     r2, [r3]
    0bba  20000e3a  21 78       ldrb     r1, [r4]
    0bbc  20000e3c  01 30       adds     r0, #1
    0bbe  20000e3e  ff f7 8b fb bl       #0x20000558
    0bc2  20000e42  00 21       movs     r1, #0
    0bc4  20000e44  01 23       movs     r3, #1
    0bc6  20000e46  01 70       strb     r1, [r0]
    0bc8  20000e48  00 93       str      r3, [sp]
    0bca  20000e4a  30 22       movs     r2, #0x30
    0bcc  20000e4c  0b 00       movs     r3, r1
    0bce  20000e4e  02 a8       add      r0, sp, #8
    0bd0  20000e50  fc 69       ldr      r4, [r7, #0x1c]
    0bd2  20000e52  a0 47       blx      r4
    0bd4  20000e54  3b 6a       ldr      r3, [r7, #0x20]
    0bd6  20000e56  98 47       blx      r3
    0bd8  20000e58  09 b0       add      sp, #0x24
    0bda  20000e5a  f0 bd       pop      {r4, r5, r6, r7, pc}
    0bdc  20000e5c  a4 11       asrs     r4, r4, #6
    0bde  20000e5e  00 20       movs     r0, #0
    0be0  20000e60  57 11       asrs     r7, r2, #5
    0be2  20000e62  00 20       movs     r0, #0
    0be4  20000e64  54 10       asrs     r4, r2, #1
    0be6  20000e66  00 20       movs     r0, #0
    0be8  20000e68  4d 10       asrs     r5, r1, #1
    0bea  20000e6a  00 20       movs     r0, #0
    0bec  20000e6c  50 10       asrs     r0, r2, #1
    0bee  20000e6e  00 20       movs     r0, #0
    0bf0  20000e70  59 10       asrs     r1, r3, #1
    0bf2  20000e72  00 20       movs     r0, #0
    0bf4  20000e74  70 b5       push     {r4, r5, r6, lr}
    0bf6  20000e76  2f 4c       ldr      r4, [pc, #0xbc]
    0bf8  20000e78  26 68       ldr      r6, [r4]
    0bfa  20000e7a  f3 6a       ldr      r3, [r6, #0x2c]
    0bfc  20000e7c  98 47       blx      r3
    0bfe  20000e7e  05 00       movs     r5, r0
    0c00  20000e80  13 28       cmp      r0, #0x13
    0c02  20000e82  04 d0       beq      #0x20000e8e
    0c04  20000e84  23 00       movs     r3, r4
    0c06  20000e86  b7 33       adds     r3, #0xb7
    0c08  20000e88  1a 78       ldrb     r2, [r3]
    0c0a  20000e8a  82 42       cmp      r2, r0
    0c0c  20000e8c  02 d1       bne      #0x20000e94
    0c0e  20000e8e  b7 34       adds     r4, #0xb7
    0c10  20000e90  25 70       strb     r5, [r4]
    0c12  20000e92  70 bd       pop      {r4, r5, r6, pc}
    0c14  20000e94  18 70       strb     r0, [r3]
    0c16  20000e96  33 00       movs     r3, r6
    0c18  20000e98  dc 33       adds     r3, #0xdc
    0c1a  20000e9a  1b 68       ldr      r3, [r3]
    0c1c  20000e9c  98 47       blx      r3
    0c1e  20000e9e  00 28       cmp      r0, #0
    0c20  20000ea0  19 d0       beq      #0x20000ed6
    0c22  20000ea2  23 00       movs     r3, r4
    0c24  20000ea4  b5 33       adds     r3, #0xb5
    0c26  20000ea6  1a 78       ldrb     r2, [r3]
    0c28  20000ea8  10 18       adds     r0, r2, r0
    0c2a  20000eaa  c3 43       mvns     r3, r0
    0c2c  20000eac  db 17       asrs     r3, r3, #0x1f
    0c2e  20000eae  18 40       ands     r0, r3
    0c30  20000eb0  23 00       movs     r3, r4
    0c32  20000eb2  b4 33       adds     r3, #0xb4
    0c34  20000eb4  1b 78       ldrb     r3, [r3]
    0c36  20000eb6  83 42       cmp      r3, r0
    0c38  20000eb8  00 dc       bgt      #0x20000ebc
    0c3a  20000eba  58 1e       subs     r0, r3, #1
    0c3c  20000ebc  c3 43       mvns     r3, r0
    0c3e  20000ebe  db 17       asrs     r3, r3, #0x1f
    0c40  20000ec0  18 40       ands     r0, r3
    0c42  20000ec2  c0 b2       uxtb     r0, r0
    0c44  20000ec4  82 42       cmp      r2, r0
    0c46  20000ec6  e4 d0       beq      #0x20000e92
    0c48  20000ec8  23 00       movs     r3, r4
    0c4a  20000eca  b5 33       adds     r3, #0xb5
    0c4c  20000ecc  18 70       strb     r0, [r3]
    0c4e  20000ece  00 23       movs     r3, #0
    0c50  20000ed0  b6 34       adds     r4, #0xb6
    0c52  20000ed2  23 70       strb     r3, [r4]
    0c54  20000ed4  1d e0       b        #0x20000f12
    0c56  20000ed6  06 2d       cmp      r5, #6
    0c58  20000ed8  1e d0       beq      #0x20000f18
    0c5a  20000eda  0c d8       bhi      #0x20000ef6
    0c5c  20000edc  01 2d       cmp      r5, #1
    0c5e  20000ede  11 d0       beq      #0x20000f04
    0c60  20000ee0  04 2d       cmp      r5, #4
    0c62  20000ee2  d6 d1       bne      #0x20000e92
    0c64  20000ee4  01 23       movs     r3, #1
    0c66  20000ee6  be 34       adds     r4, #0xbe
    0c68  20000ee8  22 78       ldrb     r2, [r4]
    0c6a  20000eea  04 36       adds     r6, #4
    0c6c  20000eec  53 40       eors     r3, r2
    0c6e  20000eee  23 70       strb     r3, [r4]
    0c70  20000ef0  f3 6f       ldr      r3, [r6, #0x7c]
    0c72  20000ef2  98 47       blx      r3
    0c74  20000ef4  0d e0       b        #0x20000f12
    0c76  20000ef6  09 2d       cmp      r5, #9
    0c78  20000ef8  14 d0       beq      #0x20000f24
    0c7a  20000efa  0d 2d       cmp      r5, #0xd
    0c7c  20000efc  c9 d1       bne      #0x20000e92
    0c7e  20000efe  bc 34       adds     r4, #0xbc
    0c80  20000f00  20 70       strb     r0, [r4]
    0c82  20000f02  c6 e7       b        #0x20000e92
    0c84  20000f04  bd 34       adds     r4, #0xbd
    0c86  20000f06  20 78       ldrb     r0, [r4]
    0c88  20000f08  f3 6d       ldr      r3, [r6, #0x5c]
    0c8a  20000f0a  68 40       eors     r0, r5
    0c8c  20000f0c  c0 b2       uxtb     r0, r0
    0c8e  20000f0e  20 70       strb     r0, [r4]
    0c90  20000f10  98 47       blx      r3
    0c92  20000f12  ff f7 1f ff bl       #0x20000d54
    0c96  20000f16  bc e7       b        #0x20000e92
    0c98  20000f18  b6 34       adds     r4, #0xb6
    0c9a  20000f1a  23 78       ldrb     r3, [r4]
    0c9c  20000f1c  00 2b       cmp      r3, #0
    0c9e  20000f1e  b8 d0       beq      #0x20000e92
    0ca0  20000f20  01 3b       subs     r3, #1
    0ca2  20000f22  d6 e7       b        #0x20000ed2
    0ca4  20000f24  b6 34       adds     r4, #0xb6
    0ca6  20000f26  23 78       ldrb     r3, [r4]
    0ca8  20000f28  01 33       adds     r3, #1
    0caa  20000f2a  db b2       uxtb     r3, r3
    0cac  20000f2c  02 2b       cmp      r3, #2
    0cae  20000f2e  b0 d8       bhi      #0x20000e92
    0cb0  20000f30  cf e7       b        #0x20000ed2
    0cb2  20000f32  c0 46       mov      r8, r8
    0cb4  20000f34  a4 11       asrs     r4, r4, #6
    0cb6  20000f36  00 20       movs     r0, #0
    0cb8  20000f38  f0 b5       push     {r4, r5, r6, r7, lr}
    0cba  20000f3a  c3 b0       sub      sp, #0x10c
    0cbc  20000f3c  68 46       mov      r0, sp
    0cbe  20000f3e  ff f7 2b fe bl       #0x20000b98
    0cc2  20000f42  ff f7 e1 fa bl       #0x20000508
    0cc6  20000f46  06 00       movs     r6, r0
    0cc8  20000f48  36 4c       ldr      r4, [pc, #0xd8]
    0cca  20000f4a  23 00       movs     r3, r4
    0ccc  20000f4c  bc 33       adds     r3, #0xbc
    0cce  20000f4e  1b 78       ldrb     r3, [r3]
    0cd0  20000f50  00 2b       cmp      r3, #0
    0cd2  20000f52  3c d0       beq      #0x20000fce
    0cd4  20000f54  ff f7 d8 fa bl       #0x20000508
    0cd8  20000f58  80 1b       subs     r0, r0, r6
    0cda  20000f5a  fb d4       bmi      #0x20000f54
    0cdc  20000f5c  a0 20       movs     r0, #0xa0
    0cde  20000f5e  32 4b       ldr      r3, [pc, #0xc8]
    0ce0  20000f60  c0 03       lsls     r0, r0, #0xf
    0ce2  20000f62  f6 18       adds     r6, r6, r3
    0ce4  20000f64  31 4b       ldr      r3, [pc, #0xc4]
    0ce6  20000f66  19 00       movs     r1, r3
    0ce8  20000f68  8a 68       ldr      r2, [r1, #8]
    0cea  20000f6a  02 43       orrs     r2, r0
    0cec  20000f6c  8a 60       str      r2, [r1, #8]
    0cee  20000f6e  30 4a       ldr      r2, [pc, #0xc0]
    0cf0  20000f70  02 25       movs     r5, #2
    0cf2  20000f72  08 68       ldr      r0, [r1]
    0cf4  20000f74  28 42       tst      r0, r5
    0cf6  20000f76  03 d1       bne      #0x20000f80
    0cf8  20000f78  01 3a       subs     r2, #1
    0cfa  20000f7a  92 b2       uxth     r2, r2
    0cfc  20000f7c  00 2a       cmp      r2, #0
    0cfe  20000f7e  f7 d1       bne      #0x20000f70
    0d00  20000f80  19 6d       ldr      r1, [r3, #0x50]
    0d02  20000f82  2c 4a       ldr      r2, [pc, #0xb0]
    0d04  20000f84  09 05       lsls     r1, r1, #0x14
    0d06  20000f86  09 0d       lsrs     r1, r1, #0x14
    0d08  20000f88  68 46       mov      r0, sp
    0d0a  20000f8a  ff f7 21 fe bl       #0x20000bd0
    0d0e  20000f8e  22 00       movs     r2, r4
    0d10  20000f90  f0 21       movs     r1, #0xf0
    0d12  20000f92  ba 32       adds     r2, #0xba
    0d14  20000f94  13 88       ldrh     r3, [r2]
    0d16  20000f96  89 00       lsls     r1, r1, #2
    0d18  20000f98  01 33       adds     r3, #1
    0d1a  20000f9a  9b b2       uxth     r3, r3
    0d1c  20000f9c  8b 42       cmp      r3, r1
    0d1e  20000f9e  01 d2       bhs      #0x20000fa4
    0d20  20000fa0  13 80       strh     r3, [r2]
    0d22  20000fa2  d1 e7       b        #0x20000f48
    0d24  20000fa4  00 23       movs     r3, #0
    0d26  20000fa6  27 00       movs     r7, r4
    0d28  20000fa8  13 80       strh     r3, [r2]
    0d2a  20000faa  b8 37       adds     r7, #0xb8
    0d2c  20000fac  3b 78       ldrb     r3, [r7]
    0d2e  20000fae  00 2b       cmp      r3, #0
    0d30  20000fb0  06 d0       beq      #0x20000fc0
    0d32  20000fb2  22 00       movs     r2, r4
    0d34  20000fb4  b9 32       adds     r2, #0xb9
    0d36  20000fb6  15 78       ldrb     r5, [r2]
    0d38  20000fb8  00 2d       cmp      r5, #0
    0d3a  20000fba  12 d0       beq      #0x20000fe2
    0d3c  20000fbc  01 3d       subs     r5, #1
    0d3e  20000fbe  15 70       strb     r5, [r2]
    0d40  20000fc0  ff f7 58 ff bl       #0x20000e74
    0d44  20000fc4  23 00       movs     r3, r4
    0d46  20000fc6  bc 33       adds     r3, #0xbc
    0d48  20000fc8  1b 78       ldrb     r3, [r3]
    0d4a  20000fca  00 2b       cmp      r3, #0
    0d4c  20000fcc  1c d1       bne      #0x20001008
    0d4e  20000fce  22 00       movs     r2, r4
    0d50  20000fd0  c0 32       adds     r2, #0xc0
    0d52  20000fd2  12 68       ldr      r2, [r2]
    0d54  20000fd4  c4 34       adds     r4, #0xc4
    0d56  20000fd6  15 4b       ldr      r3, [pc, #0x54]
    0d58  20000fd8  9a 63       str      r2, [r3, #0x38]
    0d5a  20000fda  22 68       ldr      r2, [r4]
    0d5c  20000fdc  5a 61       str      r2, [r3, #0x14]
    0d5e  20000fde  43 b0       add      sp, #0x10c
    0d60  20000fe0  f0 bd       pop      {r4, r5, r6, r7, pc}
    0d62  20000fe2  01 20       movs     r0, #1
    0d64  20000fe4  98 43       bics     r0, r3
    0d66  20000fe6  23 68       ldr      r3, [r4]
    0d68  20000fe8  9b 6b       ldr      r3, [r3, #0x38]
    0d6a  20000fea  98 47       blx      r3
    0d6c  20000fec  03 22       movs     r2, #3
    0d6e  20000fee  3b 78       ldrb     r3, [r7]
    0d70  20000ff0  1a 40       ands     r2, r3
    0d72  20000ff2  01 2a       cmp      r2, #1
    0d74  20000ff4  00 d1       bne      #0x20000ff8
    0d76  20000ff6  04 25       movs     r5, #4
    0d78  20000ff8  22 00       movs     r2, r4
    0d7a  20000ffa  b9 32       adds     r2, #0xb9
    0d7c  20000ffc  15 70       strb     r5, [r2]
    0d7e  20000ffe  22 00       movs     r2, r4
    0d80  20001000  01 3b       subs     r3, #1
    0d82  20001002  b8 32       adds     r2, #0xb8
    0d84  20001004  13 70       strb     r3, [r2]
    0d86  20001006  db e7       b        #0x20000fc0
    0d88  20001008  23 00       movs     r3, r4
    0d8a  2000100a  be 33       adds     r3, #0xbe
    0d8c  2000100c  1b 78       ldrb     r3, [r3]
    0d8e  2000100e  00 2b       cmp      r3, #0
    0d90  20001010  03 d0       beq      #0x2000101a
    0d92  20001012  23 68       ldr      r3, [r4]
    0d94  20001014  04 33       adds     r3, #4
    0d96  20001016  db 6f       ldr      r3, [r3, #0x7c]
    0d98  20001018  98 47       blx      r3
    0d9a  2000101a  23 68       ldr      r3, [r4]
    0d9c  2000101c  08 33       adds     r3, #8
    0d9e  2000101e  db 6f       ldr      r3, [r3, #0x7c]
    0da0  20001020  98 47       blx      r3
    0da2  20001022  91 e7       b        #0x20000f48
    0da4  20001024  a4 11       asrs     r4, r4, #6
    0da6  20001026  00 20       movs     r0, #0
    0da8  20001028  88 13       asrs     r0, r1, #0xe
    0daa  2000102a  00 00       movs     r0, r0
    0dac  2000102c  00 24       movs     r4, #0
    0dae  2000102e  01 40       ands     r1, r0
    0db0  20001030  d1 07       lsls     r1, r2, #0x1f
    0db2  20001032  00 00       movs     r0, r0
    0db4  20001034  15 08       lsrs     r5, r2, #0x20
    0db6  20001036  00 20       movs     r0, #0
    0db8  20001038  ce b4       push     {r1, r2, r3, r6, r7}
    0dba  2000103a  d6 aa       add      r2, sp, #0x358
    0dbc  2000103c  00 2d       cmp      r5, #0
    0dbe  2000103e  2d 2d       cmp      r5, #0x2d
    0dc0  20001040  2d 00       movs     r5, r5
    0dc2  20001042  6b 6d       ldr      r3, [r5, #0x54]
    0dc4  20001044  2f 68       ldr      r7, [r5]
    0dc6  20001046  00 64       str      r0, [r0, #0x40]
    0dc8  20001048  42 00       lsls     r2, r0, #1
    0dca  2000104a  3a 20       movs     r0, #0x3a
    0dcc  2000104c  00 4f       ldr      r7, [pc, #0]
    0dce  2000104e  4e 00       lsls     r6, r1, #1
    0dd0  20001050  4f 46       mov      r7, sb
    0dd2  20001052  46 00       lsls     r6, r0, #1
    0dd4  20001054  34 42       tst      r4, r6
    0dd6  20001056  4c 3a       subs     r2, #0x4c
    0dd8  20001058  00 20       movs     r0, #0
    0dda  2000105a  31 53       strh     r1, [r6, r4]
    0ddc  2000105c  50 4b       ldr      r3, [pc, #0x140]
    0dde  2000105e  3a 00       movs     r2, r7
    0de0  20001060  b1 be       bkpt     #0xb1
    0de2  20001062  ce f1 20 bb b.w      #0x201cf6a6
    0de6  20001066  f5 00       lsls     r5, r6, #3
    0de8  20001068  b1 be       bkpt     #0xb1
    0dea  2000106a  ce f1 20 bf b.w      #0x201cfeae
    0dee  2000106e  cd 00       lsls     r5, r1, #3
    0df0  20001070  b2 b9       cbnz     r2, #0x200010a0
    0df2  20001072  bb fa       .hword   0xbb, 0xfa
    0df4  20001074  20 bb       cbnz     r0, #0x200010c0
    0df6  20001076  f5 00       lsls     r5, r6, #3
    0df8  20001078  b2 b9       cbnz     r2, #0x200010a8
    0dfa  2000107a  bb fa       .hword   0xbb, 0xfa
    0dfc  2000107c  20 bf       wfe      
    0dfe  2000107e  cd 00       lsls     r5, r1, #3

## Important boundaries

- Disassembly is a linear Thumb sweep with invalid halfwords emitted as .hword data. Literal pools and embedded data that happen to decode as valid opcodes can still appear as instructions; verify control-flow before assigning function boundaries.
- Capstone detail mode is disabled in skip-data mode; direct BL targets are parsed from rendered immediates. BLX register calls and indirect function pointers require separate data-flow analysis.
- Code offsets are relative to the byte immediately after the 64-byte FAP1 header; runtime addresses add link_vma.
- This is the published FAP1 application, not a standalone radio firmware and not original C source.
- The DSP at offset 0x950 still requires semantic reconstruction; this report does not claim that the DSP is understood.
