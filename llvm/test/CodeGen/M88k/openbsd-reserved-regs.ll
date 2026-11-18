; Regression (found running Clang-built programs on OpenBSD 7.1 luna88k under run88k):
; OpenBSD's ld.so sets r27 to the thread information block pointer and libc reads it, so a
; compiler must not allocate r27 there. OpenBSD's GCC does not allocate r26 either. Other OSes
; (NetBSD) keep both allocatable. The function has enough live values to need r26/r27.
;
; RUN: llc < %s -mtriple=m88k-unknown-openbsd -mcpu=mc88100 | FileCheck %s --check-prefix=OPENBSD
; RUN: llc < %s -mtriple=m88k-unknown-openbsd -mcpu=mc88110 | FileCheck %s --check-prefix=OPENBSD
; RUN: llc < %s -mtriple=m88k-unknown-netbsd -mcpu=mc88100 | FileCheck %s --check-prefix=NETBSD
; RUN: llc < %s -mtriple=m88k-unknown-openbsd -mcpu=mc88100 -O0 | FileCheck %s --check-prefix=OPENBSD
; RUN: llc < %s -mtriple=m88k-unknown-netbsd -mcpu=mc88100 -O0 | FileCheck %s --check-prefix=NETBSD-O0
;
; The register pair r26:r27 must be reserved too (the -O0 allocator picked it for doubles).
;
; OPENBSD-NOT: %r26
; OPENBSD-NOT: %r27
; NETBSD: %r27
; NETBSD-O0: %r26
define i32 @f(ptr %p) {
  %q0 = getelementptr i32, ptr %p, i32 0
  %v0 = load volatile i32, ptr %q0
  %q1 = getelementptr i32, ptr %p, i32 1
  %v1 = load volatile i32, ptr %q1
  %q2 = getelementptr i32, ptr %p, i32 2
  %v2 = load volatile i32, ptr %q2
  %q3 = getelementptr i32, ptr %p, i32 3
  %v3 = load volatile i32, ptr %q3
  %q4 = getelementptr i32, ptr %p, i32 4
  %v4 = load volatile i32, ptr %q4
  %q5 = getelementptr i32, ptr %p, i32 5
  %v5 = load volatile i32, ptr %q5
  %q6 = getelementptr i32, ptr %p, i32 6
  %v6 = load volatile i32, ptr %q6
  %q7 = getelementptr i32, ptr %p, i32 7
  %v7 = load volatile i32, ptr %q7
  %q8 = getelementptr i32, ptr %p, i32 8
  %v8 = load volatile i32, ptr %q8
  %q9 = getelementptr i32, ptr %p, i32 9
  %v9 = load volatile i32, ptr %q9
  %q10 = getelementptr i32, ptr %p, i32 10
  %v10 = load volatile i32, ptr %q10
  %q11 = getelementptr i32, ptr %p, i32 11
  %v11 = load volatile i32, ptr %q11
  %q12 = getelementptr i32, ptr %p, i32 12
  %v12 = load volatile i32, ptr %q12
  %q13 = getelementptr i32, ptr %p, i32 13
  %v13 = load volatile i32, ptr %q13
  %q14 = getelementptr i32, ptr %p, i32 14
  %v14 = load volatile i32, ptr %q14
  %q15 = getelementptr i32, ptr %p, i32 15
  %v15 = load volatile i32, ptr %q15
  %q16 = getelementptr i32, ptr %p, i32 16
  %v16 = load volatile i32, ptr %q16
  %q17 = getelementptr i32, ptr %p, i32 17
  %v17 = load volatile i32, ptr %q17
  %q18 = getelementptr i32, ptr %p, i32 18
  %v18 = load volatile i32, ptr %q18
  %q19 = getelementptr i32, ptr %p, i32 19
  %v19 = load volatile i32, ptr %q19
  %q20 = getelementptr i32, ptr %p, i32 20
  %v20 = load volatile i32, ptr %q20
  %q21 = getelementptr i32, ptr %p, i32 21
  %v21 = load volatile i32, ptr %q21
  %q22 = getelementptr i32, ptr %p, i32 22
  %v22 = load volatile i32, ptr %q22
  %q23 = getelementptr i32, ptr %p, i32 23
  %v23 = load volatile i32, ptr %q23
  %q24 = getelementptr i32, ptr %p, i32 24
  %v24 = load volatile i32, ptr %q24
  %q25 = getelementptr i32, ptr %p, i32 25
  %v25 = load volatile i32, ptr %q25
  %q26 = getelementptr i32, ptr %p, i32 26
  %v26 = load volatile i32, ptr %q26
  %q27 = getelementptr i32, ptr %p, i32 27
  %v27 = load volatile i32, ptr %q27
  %q28 = getelementptr i32, ptr %p, i32 28
  %v28 = load volatile i32, ptr %q28
  %q29 = getelementptr i32, ptr %p, i32 29
  %v29 = load volatile i32, ptr %q29
  %m1 = mul i32 %v1, %v29
  %m2 = mul i32 %v2, %v28
  %m3 = mul i32 %v3, %v27
  %m4 = mul i32 %v4, %v26
  %m5 = mul i32 %v5, %v25
  %m6 = mul i32 %v6, %v24
  %m7 = mul i32 %v7, %v23
  %m8 = mul i32 %v8, %v22
  %m9 = mul i32 %v9, %v21
  %m10 = mul i32 %v10, %v20
  %m11 = mul i32 %v11, %v19
  %m12 = mul i32 %v12, %v18
  %m13 = mul i32 %v13, %v17
  %m14 = mul i32 %v14, %v16
  %m15 = mul i32 %v15, %v15
  %m16 = mul i32 %v16, %v14
  %m17 = mul i32 %v17, %v13
  %m18 = mul i32 %v18, %v12
  %m19 = mul i32 %v19, %v11
  %m20 = mul i32 %v20, %v10
  %m21 = mul i32 %v21, %v9
  %m22 = mul i32 %v22, %v8
  %m23 = mul i32 %v23, %v7
  %m24 = mul i32 %v24, %v6
  %m25 = mul i32 %v25, %v5
  %m26 = mul i32 %v26, %v4
  %m27 = mul i32 %v27, %v3
  %m28 = mul i32 %v28, %v2
  %m29 = mul i32 %v29, %v1
  %a1 = add i32 %v0, %m1
  %a2 = add i32 %a1, %m2
  %a3 = add i32 %a2, %m3
  %a4 = add i32 %a3, %m4
  %a5 = add i32 %a4, %m5
  %a6 = add i32 %a5, %m6
  %a7 = add i32 %a6, %m7
  %a8 = add i32 %a7, %m8
  %a9 = add i32 %a8, %m9
  %a10 = add i32 %a9, %m10
  %a11 = add i32 %a10, %m11
  %a12 = add i32 %a11, %m12
  %a13 = add i32 %a12, %m13
  %a14 = add i32 %a13, %m14
  %a15 = add i32 %a14, %m15
  %a16 = add i32 %a15, %m16
  %a17 = add i32 %a16, %m17
  %a18 = add i32 %a17, %m18
  %a19 = add i32 %a18, %m19
  %a20 = add i32 %a19, %m20
  %a21 = add i32 %a20, %m21
  %a22 = add i32 %a21, %m22
  %a23 = add i32 %a22, %m23
  %a24 = add i32 %a23, %m24
  %a25 = add i32 %a24, %m25
  %a26 = add i32 %a25, %m26
  %a27 = add i32 %a26, %m27
  %a28 = add i32 %a27, %m28
  %a29 = add i32 %a28, %m29
  %r = add i32 %a29, %v0
  ret i32 %r
}

declare i32 @printf(ptr, ...)
@s = private constant [15 x i8] c"%f %f %f %f %f\00"
define void @g(ptr %p) {
  %a = load volatile double, ptr %p
  %b = load volatile double, ptr %p
  %c = load volatile double, ptr %p
  %d = load volatile double, ptr %p
  %e = load volatile double, ptr %p
  %f = load volatile double, ptr %p
  call i32 (ptr, ...) @printf(ptr @s, double %a, double %b, double %c, double %d, double %e, double %f)
  ret void
}
