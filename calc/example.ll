; ModuleID = 'calc.expr'
source_filename = "calc.expr"

@a.str = private constant [2 x i8] c"a\00"

define i32 @main(i32 %0, ptr %1) {
entry:
  %2 = call i32 @calc_read(ptr @a.str)
  br i1 false, label %7, label %8

exp.loop:                                         ; preds = %exp.loop, %8
  %exp.result = phi i32 [ %2, %8 ], [ %3, %exp.loop ]
  %3 = mul nsw i32 %exp.result, %2
  %4 = load i32, ptr %9, align 4
  %5 = sub i32 %4, 1
  store i32 %5, ptr %9, align 4
  %6 = icmp sgt i32 %5, 1
  br i1 %6, label %exp.loop, label %exp.exit

exp.exit:                                         ; preds = %exp.loop, %7
  %exp.final = phi i32 [ 1, %7 ], [ %3, %exp.loop ]
  call void @calc_write(i32 %exp.final)
  ret i32 0

7:                                                ; preds = %entry
  br label %exp.exit

8:                                                ; preds = %entry
  %9 = alloca i32, align 4
  store i32 3, ptr %9, align 4
  br label %exp.loop
}

declare i32 @calc_read(ptr)

declare void @calc_write(i32)
