; RUN: llc -mtriple=m88k-unknown-netbsd -mcpu=mc88100 -filetype=obj %s -o - \
; RUN:   | llvm-objdump -d - | FileCheck %s
; Inline asm in GNU syntax mixes the expanded %0 (printed as %rN) with bare
; control register names; the integrated assembler must accept both.
; CHECK: ldcr %r{{[0-9]+}}, %cr1
; CHECK: stcr %r{{[0-9]+}}, %cr1

define i32 @rdcr1() {
  %v = call i32 asm sideeffect "ldcr $0, cr1", "=r"()
  ret i32 %v
}

define void @wrcr1(i32 %v) {
  call void asm sideeffect "stcr $0, cr1", "r"(i32 %v)
  ret void
}
