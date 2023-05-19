; ModuleID = 'example_throw.cpp'
source_filename = "example_throw.cpp"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%"class.std::ios_base::Init" = type { i8 }

@_ZStL8__ioinit = internal global %"class.std::ios_base::Init" zeroinitializer, align 1
@__dso_handle = external hidden global i8
@_ZTIi = external constant ptr
@_ZTId = external constant ptr
@llvm.global_ctors = appending global [1 x { i32, ptr, ptr }] [{ i32, ptr, ptr } { i32 65535, ptr @_GLOBAL__sub_I_example_throw.cpp, ptr null }]

; Function Attrs: noinline uwtable
define internal void @__cxx_global_var_init() #0 section ".text.startup" {
entry:
  call void @_ZNSt8ios_base4InitC1Ev(ptr noundef nonnull align 1 dereferenceable(1) @_ZStL8__ioinit)
  %0 = call i32 @__cxa_atexit(ptr @_ZNSt8ios_base4InitD1Ev, ptr @_ZStL8__ioinit, ptr @__dso_handle) #3
  ret void
}

declare void @_ZNSt8ios_base4InitC1Ev(ptr noundef nonnull align 1 dereferenceable(1)) unnamed_addr #1

; Function Attrs: nounwind
declare void @_ZNSt8ios_base4InitD1Ev(ptr noundef nonnull align 1 dereferenceable(1)) unnamed_addr #2

; Function Attrs: nounwind
declare i32 @__cxa_atexit(ptr, ptr, ptr) #3

; Function Attrs: mustprogress noinline optnone uwtable
define dso_local noundef i32 @_Z3bari(i32 noundef %x) #4 {
entry:
  %x.addr = alloca i32, align 4
  store i32 %x, ptr %x.addr, align 4
  %0 = load i32, ptr %x.addr, align 4
  %cmp = icmp eq i32 %0, 1
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %exception = call ptr @__cxa_allocate_exception(i64 4) #3
  store i32 1, ptr %exception, align 16
  call void @__cxa_throw(ptr %exception, ptr @_ZTIi, ptr null) #6 ; throw needs a pointer to the exception
  								  ; information of the type (_ZTIi = integer)
								  ; pointer to a destructor (if it exists)
  unreachable

if.end:                                           ; preds = %entry
  %1 = load i32, ptr %x.addr, align 4
  %cmp1 = icmp eq i32 %1, 2
  br i1 %cmp1, label %if.then2, label %if.end4

if.then2:                                         ; preds = %if.end
  %exception3 = call ptr @__cxa_allocate_exception(i64 8) #3
  store double 4.200000e+01, ptr %exception3, align 16
  call void @__cxa_throw(ptr %exception3, ptr @_ZTId, ptr null) #6 ; in this case @_ZTId = double
  unreachable

if.end4:                                          ; preds = %if.end
  %2 = load i32, ptr %x.addr, align 4
  ret i32 %2
}

declare ptr @__cxa_allocate_exception(i64)

declare void @__cxa_throw(ptr, ptr, ptr)

; Function Attrs: mustprogress noinline optnone uwtable
define dso_local noundef i32 @_Z3fooi(i32 noundef %x) #4 personality ptr @__gxx_personality_v0 {
entry:
  %x.addr = alloca i32, align 4
  %y = alloca i32, align 4
  %exn.slot = alloca ptr, align 8
  %ehselector.slot = alloca i32, align 4
  %e = alloca i32, align 4
  store i32 %x, ptr %x.addr, align 4
  store i32 0, ptr %y, align 4
  %0 = load i32, ptr %x.addr, align 4
  %call = invoke noundef i32 @_Z3bari(i32 noundef %0)
          to label %invoke.cont unwind label %lpad

invoke.cont:                                      ; preds = %entry
  store i32 %call, ptr %y, align 4
  br label %try.cont

lpad:                                             ; preds = %entry
  %1 = landingpad { ptr, i32 }				; %1 = { ptr, i32 }
          catch ptr @_ZTIi
  %2 = extractvalue { ptr, i32 } %1, 0			; obtain the pointer to the exception
  store ptr %2, ptr %exn.slot, align 8			; %exn.slot = pointer to the exception
  %3 = extractvalue { ptr, i32 } %1, 1			; obtain the typeid from the landingpad structure
  store i32 %3, ptr %ehselector.slot, align 4		; %ehselector.slot = typeid
  br label %catch.dispatch

catch.dispatch:                                   ; preds = %lpad
  %sel = load i32, ptr %ehselector.slot, align 4	; load the type of exception
  %4 = call i32 @llvm.eh.typeid.for(ptr @_ZTIi) #3	; check if the exception type is an integer
  %matches = icmp eq i32 %sel, %4			
  br i1 %matches, label %catch, label %eh.resume	; if exception is an integer 

catch:                                            ; preds = %catch.dispatch
  %exn = load ptr, ptr %exn.slot, align 8		; get pointer to the exception
  %5 = call ptr @__cxa_begin_catch(ptr %exn) #3		; call catch and give as argument the exception pointer
  %6 = load i32, ptr %5, align 4
  store i32 %6, ptr %e, align 4
  %7 = load i32, ptr %e, align 4
  store i32 %7, ptr %y, align 4
  call void @__cxa_end_catch() #3
  br label %try.cont

try.cont:                                         ; preds = %catch, %invoke.cont
  %8 = load i32, ptr %y, align 4
  ret i32 %8

eh.resume:                                        ; preds = %catch.dispatch
  %exn1 = load ptr, ptr %exn.slot, align 8
  %sel2 = load i32, ptr %ehselector.slot, align 4
  %lpad.val = insertvalue { ptr, i32 } poison, ptr %exn1, 0
  %lpad.val3 = insertvalue { ptr, i32 } %lpad.val, i32 %sel2, 1
  resume { ptr, i32 } %lpad.val3
}

declare i32 @__gxx_personality_v0(...)

; Function Attrs: nounwind memory(none)
declare i32 @llvm.eh.typeid.for(ptr) #5

declare ptr @__cxa_begin_catch(ptr)

declare void @__cxa_end_catch()

; Function Attrs: noinline uwtable
define internal void @_GLOBAL__sub_I_example_throw.cpp() #0 section ".text.startup" {
entry:
  call void @__cxx_global_var_init()
  ret void
}

attributes #0 = { noinline uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { nounwind "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #3 = { nounwind }
attributes #4 = { mustprogress noinline optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #5 = { nounwind memory(none) }
attributes #6 = { noreturn }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"clang version 16.0.0 (https://github.com/llvm/llvm-project.git 6dd70c9a4b3eae8085a2b4e2336ae8b72d87681f)"}
