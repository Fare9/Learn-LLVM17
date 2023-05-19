; ModuleID = 'gcd.c'
source_filename = "gcd.c"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux-gnu"

; Function Attrs: nofree norecurse nosync nounwind memory(none) uwtable
define dso_local i32 @gcd(i32 noundef %a, i32 noundef %b) local_unnamed_addr #0 {
entry:
  %cmp = icmp eq i32 %b, 0
  br i1 %cmp, label %return, label %while.body

while.body:                                       ; preds = %entry, %while.body
  %b.addr.09 = phi i32 [ %rem, %while.body ], [ %b, %entry ]
  %a.addr.08 = phi i32 [ %b.addr.09, %while.body ], [ %a, %entry ]
  %rem = urem i32 %a.addr.08, %b.addr.09
  %cmp1.not = icmp eq i32 %rem, 0
  br i1 %cmp1.not, label %return, label %while.body, !llvm.loop !6

return:                                           ; preds = %while.body, %entry
  %retval.0 = phi i32 [ %a, %entry ], [ %b.addr.09, %while.body ]
  ret i32 %retval.0
}

attributes #0 = { nofree norecurse nosync nounwind memory(none) uwtable "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="generic" "target-features"="+neon,+v8a,-fmv" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 1}
!5 = !{!"clang version 16.0.0 (https://github.com/llvm/llvm-project.git 6dd70c9a4b3eae8085a2b4e2336ae8b72d87681f)"}
!6 = distinct !{!6, !7}
!7 = !{!"llvm.loop.mustprogress"}
