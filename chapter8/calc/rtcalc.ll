; ModuleID = 'rtcalc.c'
source_filename = "rtcalc.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [19 x i8] c"The result is: %d\0A\00", align 1
@.str.1 = private unnamed_addr constant [23 x i8] c"Enter a value for %s: \00", align 1
@stdin = external global ptr, align 8
@.str.2 = private unnamed_addr constant [3 x i8] c"%d\00", align 1
@.str.3 = private unnamed_addr constant [21 x i8] c"Value %s is invalid\0A\00", align 1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @calc_write(i32 noundef %v) #0 {
entry:
  %v.addr = alloca i32, align 4
  store i32 %v, ptr %v.addr, align 4
  %0 = load i32, ptr %v.addr, align 4
  %call = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %0)
  ret void
}

declare i32 @printf(ptr noundef, ...) #1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @calc_read(ptr noundef %s) #0 {
entry:
  %s.addr = alloca ptr, align 8
  %buf = alloca [64 x i8], align 16
  %val = alloca i32, align 4
  store ptr %s, ptr %s.addr, align 8
  %0 = load ptr, ptr %s.addr, align 8
  %call = call i32 (ptr, ...) @printf(ptr noundef @.str.1, ptr noundef %0)
  %arraydecay = getelementptr inbounds [64 x i8], ptr %buf, i64 0, i64 0
  %1 = load ptr, ptr @stdin, align 8
  %call1 = call ptr @fgets(ptr noundef %arraydecay, i32 noundef 64, ptr noundef %1)
  %arraydecay2 = getelementptr inbounds [64 x i8], ptr %buf, i64 0, i64 0
  %call3 = call i32 (ptr, ptr, ...) @__isoc99_sscanf(ptr noundef %arraydecay2, ptr noundef @.str.2, ptr noundef %val) #4
  %cmp = icmp eq i32 -1, %call3
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %arraydecay4 = getelementptr inbounds [64 x i8], ptr %buf, i64 0, i64 0
  %call5 = call i32 (ptr, ...) @printf(ptr noundef @.str.3, ptr noundef %arraydecay4)
  call void @exit(i32 noundef 1) #5
  unreachable

if.end:                                           ; preds = %entry
  %2 = load i32, ptr %val, align 4
  ret i32 %2
}

declare ptr @fgets(ptr noundef, i32 noundef, ptr noundef) #1

; Function Attrs: nounwind
declare i32 @__isoc99_sscanf(ptr noundef, ptr noundef, ...) #2

; Function Attrs: noreturn nounwind
declare void @exit(i32 noundef) #3

attributes #0 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { nounwind "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #3 = { noreturn nounwind "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #4 = { nounwind }
attributes #5 = { noreturn nounwind }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"clang version 16.0.0 (https://github.com/llvm/llvm-project.git 6dd70c9a4b3eae8085a2b4e2336ae8b72d87681f)"}
