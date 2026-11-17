; RUN: llc -mtriple=m88k-unknown-openbsd -mcpu=mc88100 -O2 < %s | FileCheck %s
; RUN: llc -mtriple=m88k-unknown-openbsd -mcpu=mc88100 -O2 -filetype=obj < %s \
; RUN:   | llvm-dwarfdump --debug-frame - | FileCheck --check-prefix=FRAME %s
; RUN: opt -strip-debug -S < %s | llc -mtriple=m88k-unknown-openbsd -mcpu=mc88100 -O2 \
; RUN:   | FileCheck --check-prefix=NODEBUG %s

; With debug info, the prologue is described by CFI in .debug_frame (the target
; uses SjLj exceptions, so no .eh_frame is emitted). The CFA is the incoming
; stack pointer. The return address r1 is stored next to the previous frame
; pointer r30, and register pairs are stored with the even register at the
; lower address.

; CHECK-LABEL: many:
; CHECK:       .cfi_sections .debug_frame
; CHECK:       .cfi_startproc
; CHECK:       subu %r31, %r31, 48
; CHECK:       .cfi_def_cfa_offset 48
; CHECK:       st %r1, %r31, 36
; CHECK:       st %r30, %r31, 32
; CHECK:       st.d %r24, %r31, 24
; CHECK:       st.d %r22, %r31, 16
; CHECK:       st.d %r20, %r31, 8
; CHECK:       .cfi_offset %r1, -12
; CHECK:       .cfi_offset %r30, -16
; CHECK:       .cfi_offset %r24, -24
; CHECK:       .cfi_offset %r25, -20
; CHECK:       .cfi_offset %r22, -32
; CHECK:       .cfi_offset %r23, -28
; CHECK:       .cfi_offset %r20, -40
; CHECK:       .cfi_offset %r21, -36
; CHECK:       addu %r30, %r31, 32
; CHECK:       .cfi_def_cfa %r30, 16
; CHECK:       .cfi_endproc

; CHECK-LABEL: leaf:
; CHECK:       .cfi_startproc
; CHECK:       subu %r31, %r31, 16
; CHECK:       .cfi_def_cfa_offset 16
; CHECK:       st %r30, %r31, 0
; CHECK:       .cfi_offset %r30, -16
; CHECK:       addu %r30, %r31, 0
; CHECK:       .cfi_def_cfa %r30, 16
; CHECK:       .cfi_endproc

; FRAME:      .debug_frame contents:
; FRAME:      CIE
; FRAME:        Return address column: 1
; FRAME:        DW_CFA_def_cfa: R31 +0
; FRAME:      FDE
; FRAME:        CFA=R30+16: R1=[CFA-12], R20=[CFA-40], R21=[CFA-36], R22=[CFA-32], R23=[CFA-28], R24=[CFA-24], R25=[CFA-20], R30=[CFA-16]
; FRAME:      FDE
; FRAME:        CFA=R30+16: R30=[CFA-16]
; FRAME:      .eh_frame contents:
; FRAME-NOT:  FDE

; NODEBUG-NOT: .cfi_

; ModuleID = 'cfi.c'
source_filename = "cfi.c"
target datalayout = "E-m:e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-a:8:16-n32"
target triple = "m88k-unknown-openbsd"

@g = external global i32, align 4

; Function Attrs: nounwind sspstrong uwtable
define dso_local i32 @many(i32 noundef %a) local_unnamed_addr #0 !dbg !11 {
entry:
    #dbg_value(i32 %a, !16, !DIExpression(), !20)
    #dbg_value(i32 0, !17, !DIExpression(), !20)
    #dbg_value(i32 0, !18, !DIExpression(), !21)
  %cmp10 = icmp sgt i32 %a, 0, !dbg !22
  br i1 %cmp10, label %for.body, label %for.cond.cleanup, !dbg !24

for.cond.cleanup:                                 ; preds = %for.body, %entry
  %s.0.lcssa = phi i32 [ 0, %entry ], [ %conv4, %for.body ], !dbg !20
  ret i32 %s.0.lcssa, !dbg !25

for.body:                                         ; preds = %entry, %for.body
  %i.012 = phi i32 [ %inc, %for.body ], [ 0, %entry ]
  %s.011 = phi i32 [ %conv4, %for.body ], [ 0, %entry ]
    #dbg_value(i32 %i.012, !18, !DIExpression(), !21)
    #dbg_value(i32 %s.011, !17, !DIExpression(), !20)
  %call = tail call i32 @add(i32 noundef %i.012, i32 noundef %a) #3, !dbg !26
  %conv1 = zext nneg i32 %i.012 to i64, !dbg !26
  %call2 = tail call i64 @big(i64 noundef %conv1, ptr noundef nonnull @g) #3, !dbg !26
  %0 = trunc i64 %call2 to i32, !dbg !26
  %1 = mul i32 %call, %0, !dbg !26
  %conv4 = add i32 %1, %s.011, !dbg !26
    #dbg_value(i32 %conv4, !17, !DIExpression(), !20)
  %inc = add nuw nsw i32 %i.012, 1, !dbg !22
    #dbg_value(i32 %inc, !18, !DIExpression(), !21)
  %exitcond.not = icmp eq i32 %inc, %a, !dbg !22
  br i1 %exitcond.not, label %for.cond.cleanup, label %for.body, !dbg !24, !llvm.loop !27
}

declare i32 @add(i32 noundef, i32 noundef) local_unnamed_addr #1

declare i64 @big(i64 noundef, ptr noundef) local_unnamed_addr #1

; Function Attrs: mustprogress nofree norecurse nosync nounwind sspstrong willreturn memory(none) uwtable
define dso_local i32 @leaf(i32 noundef %a, i32 noundef %b) local_unnamed_addr #2 !dbg !30 {
entry:
    #dbg_value(i32 %a, !34, !DIExpression(), !36)
    #dbg_value(i32 %b, !35, !DIExpression(), !36)
  %add = add nsw i32 %b, %a, !dbg !37
  ret i32 %add, !dbg !37
}

attributes #0 = { nounwind sspstrong uwtable "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="mc88100" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="mc88100" }
attributes #2 = { mustprogress nofree norecurse nosync nounwind sspstrong willreturn memory(none) uwtable "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="mc88100" }
attributes #3 = { nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2, !3, !4, !5, !6, !7, !8, !9}

!0 = distinct !DICompileUnit(language: DW_LANG_C11, file: !1, producer: "clang version 21.0.0git", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "cfi.c", directory: ".")
!2 = !{i32 7, !"Dwarf Version", i32 2}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !{i32 1, !"wchar_size", i32 4}
!5 = !{i32 8, !"PIC Level", i32 2}
!6 = !{i32 7, !"PIE Level", i32 2}
!7 = !{i32 7, !"uwtable", i32 2}
!8 = !{i32 7, !"frame-pointer", i32 2}
!9 = !{i32 7, !"debug-info-assignment-tracking", i1 true}
!11 = distinct !DISubprogram(name: "many", scope: !1, file: !1, line: 4, type: !12, scopeLine: 4, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !15)
!12 = !DISubroutineType(types: !13)
!13 = !{!14, !14}
!14 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!15 = !{!16, !17, !18}
!16 = !DILocalVariable(name: "a", arg: 1, scope: !11, file: !1, line: 4, type: !14)
!17 = !DILocalVariable(name: "s", scope: !11, file: !1, line: 5, type: !14)
!18 = !DILocalVariable(name: "i", scope: !19, file: !1, line: 6, type: !14)
!19 = distinct !DILexicalBlock(scope: !11, file: !1, line: 6)
!20 = !DILocation(line: 0, scope: !11)
!21 = !DILocation(line: 0, scope: !19)
!22 = !DILocation(line: 6, scope: !23)
!23 = distinct !DILexicalBlock(scope: !19, file: !1, line: 6)
!24 = !DILocation(line: 6, scope: !19)
!25 = !DILocation(line: 8, scope: !11)
!26 = !DILocation(line: 7, scope: !23)
!27 = distinct !{!27, !24, !28, !29}
!28 = !DILocation(line: 7, scope: !19)
!29 = !{!"llvm.loop.mustprogress"}
!30 = distinct !DISubprogram(name: "leaf", scope: !1, file: !1, line: 10, type: !31, scopeLine: 10, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !33)
!31 = !DISubroutineType(types: !32)
!32 = !{!14, !14, !14}
!33 = !{!34, !35}
!34 = !DILocalVariable(name: "a", arg: 1, scope: !30, file: !1, line: 10, type: !14)
!35 = !DILocalVariable(name: "b", arg: 2, scope: !30, file: !1, line: 10, type: !14)
!36 = !DILocation(line: 0, scope: !30)
!37 = !DILocation(line: 10, scope: !30)
