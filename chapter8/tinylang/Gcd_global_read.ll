; ModuleID = '../Gcd_global_read.mod'
source_filename = "../Gcd_global_read.mod"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@_t3Gcd3Ret = private global i64

define i64 @_t3Gcd3GCD(i64 %a, i64 %b) {
entry:
  %0 = icmp eq i64 %b, 0
  br i1 %0, label %if.body, label %while.cond

if.body:                                          ; preds = %entry
  store i64 %a, ptr @_t3Gcd3Ret, align 8
  ret i64 %a

while.cond:                                       ; preds = %while.body, %entry
  %1 = phi i64 [ %2, %while.body ], [ %a, %entry ]
  %2 = phi i64 [ %4, %while.body ], [ %b, %entry ]
  %3 = icmp ne i64 %2, 0
  br i1 %3, label %while.body, label %after.while

while.body:                                       ; preds = %while.cond
  %4 = srem i64 %1, %2
  br label %while.cond

after.while:                                      ; preds = %while.cond
  store i64 %1, ptr @_t3Gcd3Ret, align 8
  ret i64 %1
}

define i64 @_t3Gcd7Retorna() {
entry:
  %0 = load i64, ptr @_t3Gcd3Ret, align 8
  %1 = mul nsw i64 %0, 2
  %2 = load i64, ptr @_t3Gcd3Ret, align 8
  ret i64 %2
}
