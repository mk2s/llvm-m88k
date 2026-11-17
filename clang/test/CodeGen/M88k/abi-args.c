// REQUIRES: m88k-registered-target
// Argument and return value classification of the m88k Clang ABI.
// This pins the current behavior: scalars are passed directly (small integers
// sign or zero extended to 32 bits), 64-bit values and doubles as a whole,
// aggregates and complex values indirectly (byval argument, sret result).
// long double is the same as double. It does not claim to match GCC's m88k ABI.
// RUN: %clang_cc1 -triple m88k-unknown-none-elf -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple m88k-unknown-openbsd -target-cpu mc88110 -emit-llvm -o - %s | FileCheck %s

struct S1 { char c; };
struct S2 { int a, b; };
struct Big { int a[10]; };
union U { int i; float f; };
typedef struct { double d; } SD;
enum E { A, B };

// CHECK: define dso_local signext i8 @sc(i8 noundef signext
signed char sc(signed char x) { return x; }
// CHECK: define dso_local zeroext i8 @uc(i8 noundef zeroext
unsigned char uc(unsigned char x) { return x; }
// CHECK: define dso_local signext i16 @ss(i16 noundef signext
short ss(short x) { return x; }
// CHECK: define dso_local zeroext i16 @us(i16 noundef zeroext
unsigned short us(unsigned short x) { return x; }
// CHECK: define dso_local zeroext i1 @bl(i1 noundef zeroext
_Bool bl(_Bool x) { return x; }
// CHECK: define dso_local i32 @in(i32 noundef
int in(int x) { return x; }
// CHECK: define dso_local i64 @ll(i64 noundef
long long ll(long long x) { return x; }
// CHECK: define dso_local float @fl(float noundef
float fl(float x) { return x; }
// CHECK: define dso_local double @db(double noundef
double db(double x) { return x; }
// CHECK: define dso_local double @ld(double noundef
long double ld(long double x) { return x; }
// CHECK: define dso_local i32 @en(i32 noundef
enum E en(enum E x) { return x; }
// CHECK: define dso_local ptr @pt(ptr noundef
void *pt(void *x) { return x; }

// Aggregates: result through a hidden sret pointer, argument as byval copy.
// CHECK: define dso_local void @rs1(ptr {{.*}}sret(%struct.S1) align 1 %agg.result, ptr noundef byval(%struct.S1) align 1
struct S1 rs1(struct S1 x) { return x; }
// CHECK: define dso_local void @rs2(ptr {{.*}}sret(%struct.S2) align 4 %agg.result, ptr noundef byval(%struct.S2) align 4
struct S2 rs2(struct S2 x) { return x; }
// CHECK: define dso_local void @rbig(ptr {{.*}}sret(%struct.Big) align 4 %agg.result, ptr noundef byval(%struct.Big) align 4
struct Big rbig(struct Big x) { return x; }
// CHECK: define dso_local void @ru(ptr {{.*}}sret(%union.U) align 4 %agg.result, ptr noundef byval(%union.U) align 4
union U ru(union U x) { return x; }
// CHECK: define dso_local void @rsd(ptr {{.*}}sret(%struct.SD) align 8 %agg.result, ptr noundef byval(%struct.SD) align 8
SD rsd(SD x) { return x; }
// CHECK: define dso_local void @cf(ptr {{.*}}sret({ float, float }) align 4 %agg.result, ptr noundef byval({ float, float }) align 4
_Complex float cf(_Complex float x) { return x; }
// CHECK: define dso_local void @cd(ptr {{.*}}sret({ double, double }) align 8 %agg.result, ptr noundef byval({ double, double }) align 8
_Complex double cd(_Complex double x) { return x; }

// Many arguments: every argument stays a plain IR parameter, the backend
// assigns registers and stack slots.
// CHECK: define dso_local i32 @many(i32 noundef %a, i64 noundef %b, i32 noundef %c, double noundef %d, i32 noundef %e, i32 noundef %f, i32 noundef %g, i32 noundef %h, i32 noundef %i, i32 noundef %j)
int many(int a, long long b, int c, double d, int e, int f, int g, int h, int i, int j) { return j; }

// A call passes a copy of the aggregate (byval) and a temporary for the result.
// CHECK-LABEL: define dso_local void @call(
// CHECK: call void @rs2(ptr {{.*}}sret(%struct.S2) align 4 %{{.*}}, ptr noundef byval(%struct.S2) align 4 %{{.*}})
void call(struct S2 s) { rs2(s); }

// A va_list is a 12-byte struct, passed by value like any aggregate.
// CHECK: define dso_local i32 @vl(ptr noundef byval(%struct.__va_list_tag) align 4
int vl(__builtin_va_list ap) { return __builtin_va_arg(ap, int); }
