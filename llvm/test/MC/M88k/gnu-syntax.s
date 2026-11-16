# RUN: llvm-mc %s -triple=m88k-unknown-openbsd -mattr=+gnu-asm -mcpu=mc88100 -show-encoding | FileCheck %s
# GNU as syntax: bare registers, hi16()/lo16(), condition names, bitfield forms.
# Encodings were cross-checked against m88k--netbsdelf3-as (GNU as 2.16);
# bytes shown as A are filled in by a fixup (the branch/reloc displacement).

# Bare general registers
  add r0, r1, r2
  add.ci r1, r2, r3
  or r31, r30, r29
  or r4, r0, r0
  sub r2, r0, 1
  subu r5, r6, r7
  cmp r3, r2, 5
  mask r3, r4, 255
  and.u r2, r3, 0x8000
  xor r7, r7, r8
  mul r2, r3, r4
  divu r2, r3, r4
# CHECK: add %r0, %r1, %r2 {{.*}}| encoding: [0xf4,0x01,0x70,0x02]
# CHECK: add.ci %r1, %r2, %r3 {{.*}}| encoding: [0xf4,0x22,0x72,0x03]
# CHECK: or %r31, %r30, %r29 {{.*}}| encoding: [0xf7,0xfe,0x58,0x1d]
# CHECK: or %r4, %r0, %r0 {{.*}}| encoding: [0xf4,0x80,0x58,0x00]
# CHECK: sub %r2, %r0, 1 {{.*}}| encoding: [0x74,0x40,0x00,0x01]
# CHECK: subu %r5, %r6, %r7 {{.*}}| encoding: [0xf4,0xa6,0x64,0x07]
# CHECK: cmp %r3, %r2, 5 {{.*}}| encoding: [0x7c,0x62,0x00,0x05]
# CHECK: mask %r3, %r4, 255 {{.*}}| encoding: [0x48,0x64,0x00,0xff]
# CHECK: and.u %r2, %r3, 32768 {{.*}}| encoding: [0x44,0x43,0x80,0x00]
# CHECK: xor %r7, %r7, %r8 {{.*}}| encoding: [0xf4,0xe7,0x50,0x08]
# CHECK: mul %r2, %r3, %r4 {{.*}}| encoding: [0xf4,0x43,0x6c,0x04]
# CHECK: divu %r2, %r3, %r4 {{.*}}| encoding: [0xf4,0x43,0x68,0x04]

# Control registers (cr/fcr, number or alternate name)
  ldcr r2, cr1
  stcr r3, cr18
  xcr r2, r2, cr17
  fldcr r4, fcr62
  fstcr r5, fcr63
  fxcr r6, r6, fcr0
  ldcr r2, PSR
  fldcr r0, FPECR
# CHECK: ldcr %r2, %cr1 {{.*}}| encoding: [0x80,0x40,0x40,0x20]
# CHECK: stcr %r3, %cr18 {{.*}}| encoding: [0x80,0x03,0x82,0x43]
# CHECK: xcr %r2, %r2, %cr17 {{.*}}| encoding: [0x80,0x42,0xc2,0x22]
# CHECK: fldcr %r4, %fcr62 {{.*}}| encoding: [0x80,0x80,0x4f,0xc0]
# CHECK: fstcr %r5, %fcr63 {{.*}}| encoding: [0x80,0x05,0x8f,0xe5]
# CHECK: fxcr %r6, %r6, %fcr0 {{.*}}| encoding: [0x80,0xc6,0xc8,0x06]
# CHECK: ldcr %r2, %cr1 {{.*}}| encoding: [0x80,0x40,0x40,0x20]
# CHECK: fldcr %r0, %fcr0 {{.*}}| encoding: [0x80,0x00,0x48,0x00]

# hi16()/lo16() without a percent sign
  or.u r3, r0, hi16(foo)
  or r3, r3, lo16(foo)
  or.u r3, r0, hi16(foo+4)
  or r3, r3, lo16(foo+4)
  ld r4, r3, lo16(bar)
  st r4, r3, lo16(bar-8)
# CHECK: or.u %r3, %r0, %hi16(foo) {{.*}}| encoding: [0x5c,0x60,A,A]
# CHECK: or %r3, %r3, %lo16(foo) {{.*}}| encoding: [0x58,0x63,A,A]
# CHECK: or.u %r3, %r0, %hi16(foo+4) {{.*}}| encoding: [0x5c,0x60,A,A]
# CHECK: or %r3, %r3, %lo16(foo+4) {{.*}}| encoding: [0x58,0x63,A,A]
# CHECK: ld %r4, %r3, %lo16(bar) {{.*}}| encoding: [0x14,0x83,A,A]
# CHECK: st %r4, %r3, %lo16(bar-8) {{.*}}| encoding: [0x24,0x83,A,A]

# Loads and stores: immediate offsets and scaled registers
  st r0, r2, 4
  st r0, r2, 0
  ld r4, r5, 88
  ld.b r2, r3, 1
  st.h r2, r3, 2
  ld.d r2, r4, 8
  ld r4, r5[r6]
  ld.bu r2, r3[r4]
  st r1, r2[r3]
  xmem r2, r3, r4
  xmem.bu r2, r3, r4
  ld.usr r2, r3, r4
  st.usr r2, r3, r4
# CHECK: st %r0, %r2, 4 {{.*}}| encoding: [0x24,0x02,0x00,0x04]
# CHECK: st %r0, %r2, 0 {{.*}}| encoding: [0x24,0x02,0x00,0x00]
# CHECK: ld %r4, %r5, 88 {{.*}}| encoding: [0x14,0x85,0x00,0x58]
# CHECK: ld.b %r2, %r3, 1 {{.*}}| encoding: [0x1c,0x43,0x00,0x01]
# CHECK: st.h %r2, %r3, 2 {{.*}}| encoding: [0x28,0x43,0x00,0x02]
# CHECK: ld.d %r2, %r4, 8 {{.*}}| encoding: [0x10,0x44,0x00,0x08]
# CHECK: ld %r4, %r5[%r6] {{.*}}| encoding: [0xf4,0x85,0x16,0x06]
# CHECK: ld.bu %r2, %r3[%r4] {{.*}}| encoding: [0xf4,0x43,0x0e,0x04]
# CHECK: st %r1, %r2[%r3] {{.*}}| encoding: [0xf4,0x22,0x26,0x03]
# CHECK: xmem %r2, %r3, %r4 {{.*}}| encoding: [0xf4,0x43,0x04,0x04]
# CHECK: xmem.bu %r2, %r3, %r4 {{.*}}| encoding: [0xf4,0x43,0x00,0x04]
# CHECK: ld.usr %r2, %r3, %r4 {{.*}}| encoding: [0xf4,0x43,0x15,0x04]
# CHECK: st.usr %r2, %r3, %r4 {{.*}}| encoding: [0xf4,0x43,0x25,0x04]

# Bitfield instructions: width<offset> and the offset-only form
  ext r2, r3, 5<7>
  extu r2, r3, 5<0>
  mak r2, r3, 8<8>
  set r4, r4, 1<5>
  clr r2, r2, 27<5>
  ext r2, r3, <7>
  extu r2, r3, 7
  rot r11, r1, <14>
  rot r11, r1, 0<14>
  ext r2, r3, r4
  mak r2, r3, r4
# CHECK: ext %r2, %r3, 5<7> {{.*}}| encoding: [0xf0,0x43,0x90,0xa7]
# CHECK: extu %r2, %r3, 5<0> {{.*}}| encoding: [0xf0,0x43,0x98,0xa0]
# CHECK: mak %r2, %r3, 8<8> {{.*}}| encoding: [0xf0,0x43,0xa1,0x08]
# CHECK: set %r4, %r4, 1<5> {{.*}}| encoding: [0xf0,0x84,0x88,0x25]
# CHECK: clr %r2, %r2, 27<5> {{.*}}| encoding: [0xf0,0x42,0x83,0x65]
# CHECK: ext %r2, %r3, 0<7> {{.*}}| encoding: [0xf0,0x43,0x90,0x07]
# CHECK: extu %r2, %r3, 0<7> {{.*}}| encoding: [0xf0,0x43,0x98,0x07]
# CHECK: rot %r11, %r1, <14> {{.*}}| encoding: [0xf1,0x61,0xa8,0x0e]
# CHECK: rot %r11, %r1, <14> {{.*}}| encoding: [0xf1,0x61,0xa8,0x0e]
# CHECK: ext %r2, %r3, %r4 {{.*}}| encoding: [0xf4,0x43,0x90,0x04]
# CHECK: mak %r2, %r3, %r4 {{.*}}| encoding: [0xf4,0x43,0xa0,0x04]

# Condition names as bit-field offsets
  set r4, r4, 1<le>
  set r6, r0, 1<nc>
  set r6, r6, 1<cp>
  set r4, r4, 1<eq>
  set r4, r4, 1<ne>
  set r4, r4, 1<gt>
  set r4, r4, 1<lt>
  set r4, r4, 1<ge>
  set r4, r4, 1<hi>
  set r4, r4, 1<ls>
  set r4, r4, 1<lo>
  set r4, r4, 1<hs>
  set r4, r4, 1<be>
  set r4, r4, 1<nb>
  set r4, r4, 1<he>
  set r4, r4, 1<nh>
  set r4, r4, 1<ib>
  set r4, r4, 1<in>
  set r4, r4, 1<ob>
  set r4, r4, 1<ou>
# CHECK: set %r4, %r4, 1<5> {{.*}}| encoding: [0xf0,0x84,0x88,0x25]
# CHECK: set %r6, %r0, 1<0> {{.*}}| encoding: [0xf0,0xc0,0x88,0x20]
# CHECK: set %r6, %r6, 1<1> {{.*}}| encoding: [0xf0,0xc6,0x88,0x21]
# CHECK: set %r4, %r4, 1<2> {{.*}}| encoding: [0xf0,0x84,0x88,0x22]
# CHECK: set %r4, %r4, 1<3> {{.*}}| encoding: [0xf0,0x84,0x88,0x23]
# CHECK: set %r4, %r4, 1<4> {{.*}}| encoding: [0xf0,0x84,0x88,0x24]
# CHECK: set %r4, %r4, 1<6> {{.*}}| encoding: [0xf0,0x84,0x88,0x26]
# CHECK: set %r4, %r4, 1<7> {{.*}}| encoding: [0xf0,0x84,0x88,0x27]
# CHECK: set %r4, %r4, 1<8> {{.*}}| encoding: [0xf0,0x84,0x88,0x28]
# CHECK: set %r4, %r4, 1<9> {{.*}}| encoding: [0xf0,0x84,0x88,0x29]
# CHECK: set %r4, %r4, 1<10> {{.*}}| encoding: [0xf0,0x84,0x88,0x2a]
# CHECK: set %r4, %r4, 1<11> {{.*}}| encoding: [0xf0,0x84,0x88,0x2b]
# CHECK: set %r4, %r4, 1<12> {{.*}}| encoding: [0xf0,0x84,0x88,0x2c]
# CHECK: set %r4, %r4, 1<13> {{.*}}| encoding: [0xf0,0x84,0x88,0x2d]
# CHECK: set %r4, %r4, 1<14> {{.*}}| encoding: [0xf0,0x84,0x88,0x2e]
# CHECK: set %r4, %r4, 1<15> {{.*}}| encoding: [0xf0,0x84,0x88,0x2f]
# CHECK: set %r4, %r4, 1<9> {{.*}}| encoding: [0xf0,0x84,0x88,0x29]
# CHECK: set %r4, %r4, 1<10> {{.*}}| encoding: [0xf0,0x84,0x88,0x2a]
# CHECK: set %r4, %r4, 1<11> {{.*}}| encoding: [0xf0,0x84,0x88,0x2b]
# CHECK: set %r4, %r4, 1<8> {{.*}}| encoding: [0xf0,0x84,0x88,0x28]

# bb0/bb1/bcnd with condition names, numbers, delay slots
  bb1 lt, r9, foo
  bb0 ge, r9, foo
  bb1.n eq, r2, foo
  bb0.n ne, r2, foo
  bb1 0, r2, foo
  bb0 31, r2, foo
  bb1 nc, r2, foo
  bb1 cp, r2, foo
  bb1 ib, r2, foo
  bb1 ou, r2, foo
  bcnd ne0, r8, foo
  bcnd.n eq0, r8, foo
  bcnd lt0, r8, foo
  bcnd 13, r8, foo
  br foo
  br.n foo
  bsr foo
  bsr.n foo
# CHECK: bb1 6, %r9, foo {{.*}}| encoding: [0xd8,0xc9,A,A]
# CHECK: bb0 7, %r9, foo {{.*}}| encoding: [0xd0,0xe9,A,A]
# CHECK: bb1.n 2, %r2, foo {{.*}}| encoding: [0xdc,0x42,A,A]
# CHECK: bb0.n 3, %r2, foo {{.*}}| encoding: [0xd4,0x62,A,A]
# CHECK: bb1 0, %r2, foo {{.*}}| encoding: [0xd8,0x02,A,A]
# CHECK: bb0 31, %r2, foo {{.*}}| encoding: [0xd3,0xe2,A,A]
# CHECK: bb1 0, %r2, foo {{.*}}| encoding: [0xd8,0x02,A,A]
# CHECK: bb1 1, %r2, foo {{.*}}| encoding: [0xd8,0x22,A,A]
# CHECK: bb1 9, %r2, foo {{.*}}| encoding: [0xd9,0x22,A,A]
# CHECK: bb1 8, %r2, foo {{.*}}| encoding: [0xd9,0x02,A,A]
# CHECK: bcnd ne0, %r8, foo {{.*}}| encoding: [0xe9,0xa8,A,A]
# CHECK: bcnd.n eq0, %r8, foo {{.*}}| encoding: [0xec,0x48,A,A]
# CHECK: bcnd lt0, %r8, foo {{.*}}| encoding: [0xe9,0x88,A,A]
# CHECK: bcnd ne0, %r8, foo {{.*}}| encoding: [0xe9,0xa8,A,A]
# CHECK: br foo {{.*}}| encoding: [0b110000AA,A,A,A]
# CHECK: br.n foo {{.*}}| encoding: [0b110001AA,A,A,A]
# CHECK: bsr foo {{.*}}| encoding: [0b110010AA,A,A,A]
# CHECK: bsr.n foo {{.*}}| encoding: [0b110011AA,A,A,A]

# Traps, jumps and misc
  tb0 0, r0, 450
  tb1 1, r2, 3
  tcnd ne0, r2, 4
  jmp r1
  jmp.n r1
  jsr r2
  jsr.n r13
  ff1 r2, r3
  ff0 r2, r3
  rte
  tbnd r2, 7
# CHECK: tb0 0, %r0, 450 {{.*}}| encoding: [0xf0,0x00,0xd1,0xc2]
# CHECK: tb1 1, %r2, 3 {{.*}}| encoding: [0xf0,0x22,0xd8,0x03]
# CHECK: tcnd ne0, %r2, 4 {{.*}}| encoding: [0xf1,0xa2,0xe8,0x04]
# CHECK: jmp %r1 {{.*}}| encoding: [0xf4,0x00,0xc0,0x01]
# CHECK: jmp.n %r1 {{.*}}| encoding: [0xf4,0x00,0xc4,0x01]
# CHECK: jsr %r2 {{.*}}| encoding: [0xf4,0x00,0xc8,0x02]
# CHECK: jsr.n %r13 {{.*}}| encoding: [0xf4,0x00,0xcc,0x0d]
# CHECK: ff1 %r2, %r3 {{.*}}| encoding: [0xf4,0x40,0xe8,0x03]
# CHECK: ff0 %r2, %r3 {{.*}}| encoding: [0xf4,0x40,0xec,0x03]
# CHECK: rte {{.*}}| encoding: [0xf4,0x00,0xfc,0x00]
# CHECK: tbnd %r2, 7 {{.*}}| encoding: [0xf8,0x02,0x00,0x07]

# Mixed %-prefixed and bare operands (clang inline asm expands %0 to %rN)
  ldcr %r3, cr1
  stcr %r4, cr1
  fldcr %r5, fcr62
  or.u %r3, %r0, hi16(foo)
  or %r3, %r3, %lo16(foo)
  ld r4, %r5[r6]
  tb0 0, r0, 450
  st %r0, r2, 4
# CHECK: ldcr %r3, %cr1 {{.*}}| encoding: [0x80,0x60,0x40,0x20]
# CHECK: stcr %r4, %cr1 {{.*}}| encoding: [0x80,0x04,0x80,0x24]
# CHECK: fldcr %r5, %fcr62 {{.*}}| encoding: [0x80,0xa0,0x4f,0xc0]
# CHECK: or.u %r3, %r0, %hi16(foo) {{.*}}| encoding: [0x5c,0x60,A,A]
# CHECK: or %r3, %r3, %lo16(foo) {{.*}}| encoding: [0x58,0x63,A,A]
# CHECK: ld %r4, %r5[%r6] {{.*}}| encoding: [0xf4,0x85,0x16,0x06]
# CHECK: tb0 0, %r0, 450 {{.*}}| encoding: [0xf0,0x00,0xd1,0xc2]
# CHECK: st %r0, %r2, 4 {{.*}}| encoding: [0x24,0x02,0x00,0x04]

# Expression operands
  or r2, r0, (1<<4)|3
  addu r1, r1, (0xffff & ( foo - . - 4))
  ld r2, r3, 4*4
# CHECK: or %r2, %r0, 16 {{.*}}| encoding: [0x58,0x40,0x00,0x10]
# CHECK: addu %r1, %r1, 65535&((foo-.Ltmp0)-4) {{.*}}| encoding: [0x60,0x21,A,A]
# CHECK: ld %r2, %r3, 16 {{.*}}| encoding: [0x14,0x43,0x00,0x10]

foo:
bar:
