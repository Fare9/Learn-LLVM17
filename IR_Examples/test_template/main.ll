; ModuleID = 'main.cpp'
source_filename = "main.cpp"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%class.Test = type { ptr, i32, i32, i64 }
%class.Test.0 = type { ptr, i8, i32, i64 }

$_ZN4TestIiEC2Ei = comdat any

$_ZN4TestIcEC2Ec = comdat any

$_ZNK4TestIiE4showEv = comdat any

$_ZNK4TestIcE4showEv = comdat any

$_ZTV4TestIiE = comdat any

$_ZTS4TestIiE = comdat any

$_ZTI4TestIiE = comdat any

$_ZTV4TestIcE = comdat any

$_ZTS4TestIcE = comdat any

$_ZTI4TestIcE = comdat any

@_ZTV4TestIiE = linkonce_odr dso_local unnamed_addr constant { [3 x ptr] } { [3 x ptr] [ptr null, ptr @_ZTI4TestIiE, ptr @_ZNK4TestIiE4showEv] }, comdat, align 8
@_ZTVN10__cxxabiv117__class_type_infoE = external global ptr
@_ZTS4TestIiE = linkonce_odr dso_local constant [9 x i8] c"4TestIiE\00", comdat, align 1
@_ZTI4TestIiE = linkonce_odr dso_local constant { ptr, ptr } { ptr getelementptr inbounds (ptr, ptr @_ZTVN10__cxxabiv117__class_type_infoE, i64 2), ptr @_ZTS4TestIiE }, comdat, align 8
@_ZTV4TestIcE = linkonce_odr dso_local unnamed_addr constant { [3 x ptr] } { [3 x ptr] [ptr null, ptr @_ZTI4TestIcE, ptr @_ZNK4TestIcE4showEv] }, comdat, align 8
@_ZTS4TestIcE = linkonce_odr dso_local constant [9 x i8] c"4TestIcE\00", comdat, align 1
@_ZTI4TestIcE = linkonce_odr dso_local constant { ptr, ptr } { ptr getelementptr inbounds (ptr, ptr @_ZTVN10__cxxabiv117__class_type_infoE, i64 2), ptr @_ZTS4TestIcE }, comdat, align 8

; Function Attrs: mustprogress noinline norecurse optnone uwtable
define dso_local noundef i32 @main() #0 {
entry:
  %retval = alloca i32, align 4
  %a = alloca %class.Test, align 8
  %b = alloca %class.Test.0, align 8
  store i32 0, ptr %retval, align 4
  call void @_ZN4TestIiEC2Ei(ptr noundef nonnull align 8 dereferenceable(24) %a, i32 noundef 5)
  call void @_ZN4TestIcEC2Ec(ptr noundef nonnull align 8 dereferenceable(24) %b, i8 noundef signext 98)
  %call = call noundef i32 @_ZNK4TestIiE4showEv(ptr noundef nonnull align 8 dereferenceable(24) %a)
  %call1 = call noundef signext i8 @_ZNK4TestIcE4showEv(ptr noundef nonnull align 8 dereferenceable(24) %b)
  ret i32 0
}

; Function Attrs: noinline nounwind optnone uwtable
define linkonce_odr dso_local void @_ZN4TestIiEC2Ei(ptr noundef nonnull align 8 dereferenceable(24) %this, i32 noundef %var) unnamed_addr #1 comdat align 2 {
entry:
  %this.addr = alloca ptr, align 8
  %var.addr = alloca i32, align 4
  store ptr %this, ptr %this.addr, align 8
  store i32 %var, ptr %var.addr, align 4
  %this1 = load ptr, ptr %this.addr, align 8
  store ptr getelementptr inbounds ({ [3 x ptr] }, ptr @_ZTV4TestIiE, i32 0, inrange i32 0, i32 2), ptr %this1, align 8
  %var2 = getelementptr inbounds %class.Test, ptr %this1, i32 0, i32 1
  %0 = load i32, ptr %var.addr, align 4
  store i32 %0, ptr %var2, align 8
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define linkonce_odr dso_local void @_ZN4TestIcEC2Ec(ptr noundef nonnull align 8 dereferenceable(24) %this, i8 noundef signext %var) unnamed_addr #1 comdat align 2 {
entry:
  %this.addr = alloca ptr, align 8
  %var.addr = alloca i8, align 1
  store ptr %this, ptr %this.addr, align 8
  store i8 %var, ptr %var.addr, align 1
  %this1 = load ptr, ptr %this.addr, align 8
  store ptr getelementptr inbounds ({ [3 x ptr] }, ptr @_ZTV4TestIcE, i32 0, inrange i32 0, i32 2), ptr %this1, align 8
  %var2 = getelementptr inbounds %class.Test.0, ptr %this1, i32 0, i32 1
  %0 = load i8, ptr %var.addr, align 1
  store i8 %0, ptr %var2, align 8
  ret void
}

; Function Attrs: mustprogress noinline nounwind optnone uwtable
define linkonce_odr dso_local noundef i32 @_ZNK4TestIiE4showEv(ptr noundef nonnull align 8 dereferenceable(24) %this) unnamed_addr #2 comdat align 2 {
entry:
  %this.addr = alloca ptr, align 8
  store ptr %this, ptr %this.addr, align 8
  %this1 = load ptr, ptr %this.addr, align 8
  %var = getelementptr inbounds %class.Test, ptr %this1, i32 0, i32 1
  %0 = load i32, ptr %var, align 8
  ret i32 %0
}

; Function Attrs: mustprogress noinline nounwind optnone uwtable
define linkonce_odr dso_local noundef signext i8 @_ZNK4TestIcE4showEv(ptr noundef nonnull align 8 dereferenceable(24) %this) unnamed_addr #2 comdat align 2 {
entry:
  %this.addr = alloca ptr, align 8
  store ptr %this, ptr %this.addr, align 8
  %this1 = load ptr, ptr %this.addr, align 8
  %var = getelementptr inbounds %class.Test.0, ptr %this1, i32 0, i32 1
  %0 = load i8, ptr %var, align 8
  ret i8 %0
}

attributes #0 = { mustprogress noinline norecurse optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { mustprogress noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"clang version 16.0.0 (https://github.com/llvm/llvm-project.git 6dd70c9a4b3eae8085a2b4e2336ae8b72d87681f)"}
