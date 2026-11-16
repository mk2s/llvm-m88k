; The scaled-address (lda.h) selection of a G_PTR_ADD leaves a G_PTRTOINT that
; must become a COPY, otherwise the assembly printer/object writer fails.
;
; RUN: llc < %s -mtriple=m88k-unknown-none-elf -mcpu=mc88100 | FileCheck %s
; RUN: llc < %s -mtriple=m88k-unknown-none-elf -mcpu=mc88100 -filetype=obj -o /dev/null

@tab = external global [0 x i16]

define ptr @lookup(i32 %i) {
; CHECK-LABEL: lookup:
; CHECK:       lda.h
  %p = getelementptr i16, ptr @tab, i32 %i
  ret ptr %p
}
