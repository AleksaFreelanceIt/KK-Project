; ModuleID = 'example_licm_loop_unrolling.ll'
source_filename = "example_licm_loop_unrolling.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@global_const = dso_local global i32 7, align 4

; Function Attrs: noinline nounwind sspstrong uwtable
define dso_local i32 @foo(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  store i32 0, ptr %3, align 4
  store i32 0, ptr %4, align 4
  %7 = load i32, ptr %4, align 4
  store i32 6, ptr %5, align 4
  %8 = load i32, ptr %5, align 4
  %9 = add nsw i32 %8, 1
  store i32 %9, ptr %6, align 4
  %10 = load i32, ptr %6, align 4
  %11 = load i32, ptr %4, align 4
  %12 = add nsw i32 %10, %11
  %13 = load i32, ptr %3, align 4
  %14 = add nsw i32 %13, %12
  store i32 %14, ptr %3, align 4
  store i32 6, ptr %5, align 4
  %15 = load i32, ptr %5, align 4
  %16 = add nsw i32 %15, 1
  store i32 %16, ptr %6, align 4
  %17 = load i32, ptr %6, align 4
  %18 = add i32 %7, 1
  %19 = add nsw i32 %17, %18
  %20 = load i32, ptr %3, align 4
  %21 = add nsw i32 %20, %19
  store i32 %21, ptr %3, align 4
  store i32 6, ptr %5, align 4
  %22 = load i32, ptr %5, align 4
  %23 = add nsw i32 %22, 1
  store i32 %23, ptr %6, align 4
  %24 = load i32, ptr %6, align 4
  %25 = add i32 %7, 2
  %26 = add nsw i32 %24, %25
  %27 = load i32, ptr %3, align 4
  %28 = add nsw i32 %27, %26
  store i32 %28, ptr %3, align 4
  store i32 6, ptr %5, align 4
  %29 = load i32, ptr %5, align 4
  %30 = add nsw i32 %29, 1
  store i32 %30, ptr %6, align 4
  %31 = load i32, ptr %6, align 4
  %32 = add i32 %7, 3
  %33 = add nsw i32 %31, %32
  %34 = load i32, ptr %3, align 4
  %35 = add nsw i32 %34, %33
  store i32 %35, ptr %3, align 4
  br label %36

36:                                               ; preds = %1
  %37 = load i32, ptr %3, align 4
  ret i32 %37
}

; Function Attrs: noinline nounwind sspstrong uwtable
define dso_local i32 @bar(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  store i32 0, ptr %3, align 4
  store i32 0, ptr %4, align 4
  %6 = load i32, ptr %2, align 4
  br label %7

7:                                                ; preds = %28, %1
  %8 = load i32, ptr %4, align 4
  %9 = icmp slt i32 %8, %6
  br i1 %9, label %10, label %31

10:                                               ; preds = %7
  store i32 15, ptr %5, align 4
  %11 = load i32, ptr %5, align 4
  %12 = load i32, ptr %4, align 4
  %13 = add nsw i32 %11, %12
  %14 = load i32, ptr %3, align 4
  %15 = add nsw i32 %14, %13
  store i32 %15, ptr %3, align 4
  store i32 15, ptr %5, align 4
  %16 = load i32, ptr %5, align 4
  %17 = load i32, ptr %4, align 4
  %18 = add i32 %17, 1
  %19 = add nsw i32 %16, %18
  %20 = load i32, ptr %3, align 4
  %21 = add nsw i32 %20, %19
  store i32 %21, ptr %3, align 4
  store i32 15, ptr %5, align 4
  %22 = load i32, ptr %5, align 4
  %23 = load i32, ptr %4, align 4
  %24 = add i32 %23, 2
  %25 = add nsw i32 %22, %24
  %26 = load i32, ptr %3, align 4
  %27 = add nsw i32 %26, %25
  store i32 %27, ptr %3, align 4
  br label %28

28:                                               ; preds = %10
  %29 = load i32, ptr %4, align 4
  %30 = add nsw i32 %29, 3
  store i32 %30, ptr %4, align 4
  br label %7, !llvm.loop !6

31:                                               ; preds = %7
  %32 = load i32, ptr %3, align 4
  ret i32 %32
}

; Function Attrs: noinline nounwind sspstrong uwtable
define dso_local i32 @main() #0 {
  %1 = alloca i32, align 4
  store i32 0, ptr %1, align 4
  %2 = call i32 @foo(i32 noundef 4)
  %3 = call i32 @bar(i32 noundef 10)
  %4 = add nsw i32 %2, %3
  ret i32 %4
}

attributes #0 = { noinline nounwind sspstrong uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"clang version 22.1.5"}
!6 = distinct !{!6, !7}
!7 = !{!"llvm.loop.mustprogress"}
