; A dbg.value of a value whose type is narrower than 32 bits must not crash
; RegBankSelect (picolibc is built with -g by default).
;
; RUN: llc < %s -mtriple=m88k-unknown-none-elf -mcpu=mc88100 | FileCheck %s

define i32 @f(i32 %c, i16 zeroext %d) !dbg !6 {
; CHECK-LABEL: f:
; CHECK:       jmp
entry:
  #dbg_value(i16 %d, !9, !DIExpression(), !11)
  %e = zext i16 %d to i32
  %r = add i32 %e, %c
  ret i32 %r
}

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3}
!0 = distinct !DICompileUnit(language: DW_LANG_C11, file: !1, producer: "t", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug)
!1 = !DIFile(filename: "t.c", directory: "/")
!3 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "f", scope: !1, file: !1, line: 1, type: !7, scopeLine: 1, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0)
!7 = !DISubroutineType(types: !8)
!8 = !{null}
!9 = !DILocalVariable(name: "d", arg: 2, scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "short", size: 16, encoding: DW_ATE_unsigned)
!11 = !DILocation(line: 1, scope: !6)
