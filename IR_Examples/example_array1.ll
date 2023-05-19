; ModuleID = 'example_array1.c'
source_filename = "example_array1.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@array = dso_local global [8 x i8] c"\03\01\08\04\05\02\06\07", align 1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local zeroext i8 @ret_index_1() #0 {
entry:
  ; getelementptr takes as first parameter the
  ; base of type of the array, second the pointer
  ; to the array, third it takes index 0 for
  ; deferencing the array, and then the accessed index.
  %0 = load i8, ptr getelementptr inbounds ([8 x i8], ptr @array, i64 0, i64 1), align 1
  ret i8 %0
}

attributes #0 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"clang version 16.0.0 (https://github.com/llvm/llvm-project.git 6dd70c9a4b3eae8085a2b4e2336ae8b72d87681f)"}
