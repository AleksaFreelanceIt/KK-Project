; ModuleID = 'example_loop_unrolling.ll'
source_filename = "example_loop_unrolling.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@__const.main.a = private unnamed_addr constant [4 x i32] [i32 1, i32 -2, i32 3, i32 -4], align 16

; Function Attrs: noinline nounwind sspstrong uwtable
define dso_local i32 @foo(ptr noundef %0) #0 {
  %2 = alloca ptr, align 8
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  store ptr %0, ptr %2, align 8
  store i32 0, ptr %3, align 4
  store i32 0, ptr %4, align 4
  %5 = load i32, ptr %4, align 4
  %6 = load i32, ptr %4, align 4
  %7 = mul nsw i32 %6, 2
  %8 = load i32, ptr %3, align 4
  %9 = add nsw i32 %8, %7
  store i32 %9, ptr %3, align 4
  %10 = add i32 %5, 1
  %11 = mul nsw i32 %10, 2
  %12 = load i32, ptr %3, align 4
  %13 = add nsw i32 %12, %11
  store i32 %13, ptr %3, align 4
  %14 = add i32 %5, 2
  %15 = mul nsw i32 %14, 2
  %16 = load i32, ptr %3, align 4
  %17 = add nsw i32 %16, %15
  store i32 %17, ptr %3, align 4
  %18 = add i32 %5, 3
  %19 = mul nsw i32 %18, 2
  %20 = load i32, ptr %3, align 4
  %21 = add nsw i32 %20, %19
  store i32 %21, ptr %3, align 4
  br label %22

22:                                               ; preds = %1
  %23 = load i32, ptr %3, align 4
  ret i32 %23
}

; Function Attrs: noinline nounwind sspstrong uwtable
define dso_local i32 @bar(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  store i32 0, ptr %3, align 4
  store i32 0, ptr %4, align 4
  br label %5

5:                                                ; preds = %24, %1
  %6 = load i32, ptr %4, align 4
  %7 = load i32, ptr %2, align 4
  %8 = icmp slt i32 %6, %7
  br i1 %8, label %9, label %27

9:                                                ; preds = %5
  %10 = load i32, ptr %4, align 4
  %11 = mul nsw i32 %10, 3
  %12 = load i32, ptr %3, align 4
  %13 = add nsw i32 %12, %11
  store i32 %13, ptr %3, align 4
  %14 = load i32, ptr %4, align 4
  %15 = add i32 %14, 1
  %16 = mul nsw i32 %15, 3
  %17 = load i32, ptr %3, align 4
  %18 = add nsw i32 %17, %16
  store i32 %18, ptr %3, align 4
  %19 = load i32, ptr %4, align 4
  %20 = add i32 %19, 2
  %21 = mul nsw i32 %20, 3
  %22 = load i32, ptr %3, align 4
  %23 = add nsw i32 %22, %21
  store i32 %23, ptr %3, align 4
  br label %24

24:                                               ; preds = %9
  %25 = load i32, ptr %4, align 4
  %26 = add nsw i32 %25, 3
  store i32 %26, ptr %4, align 4
  br label %5, !llvm.loop !6

27:                                               ; preds = %5
  %28 = load i32, ptr %3, align 4
  ret i32 %28
}

; Function Attrs: noinline nounwind sspstrong uwtable
define dso_local i32 @baz(ptr noundef %0) #0 {
  %2 = alloca ptr, align 8
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  store ptr %0, ptr %2, align 8
  store i32 0, ptr %3, align 4
  store i32 0, ptr %4, align 4
  br label %5

5:                                                ; preds = %1
  %6 = load ptr, ptr %2, align 8
  %7 = load i32, ptr %4, align 4
  %8 = sext i32 %7 to i64
  %9 = getelementptr inbounds i32, ptr %6, i64 %8
  %10 = load i32, ptr %9, align 4
  %11 = icmp sgt i32 %10, 0
  br i1 %11, label %12, label %20

12:                                               ; preds = %5
  %13 = load ptr, ptr %2, align 8
  %14 = load i32, ptr %4, align 4
  %15 = sext i32 %14 to i64
  %16 = getelementptr inbounds i32, ptr %13, i64 %15
  %17 = load i32, ptr %16, align 4
  %18 = load i32, ptr %3, align 4
  %19 = add nsw i32 %18, %17
  store i32 %19, ptr %3, align 4
  br label %28

20:                                               ; preds = %5
  %21 = load ptr, ptr %2, align 8
  %22 = load i32, ptr %4, align 4
  %23 = sext i32 %22 to i64
  %24 = getelementptr inbounds i32, ptr %21, i64 %23
  %25 = load i32, ptr %24, align 4
  %26 = load i32, ptr %3, align 4
  %27 = sub nsw i32 %26, %25
  store i32 %27, ptr %3, align 4
  br label %28

28:                                               ; preds = %20, %12
  br label %29

29:                                               ; preds = %28
  %30 = load ptr, ptr %2, align 8
  %31 = load i32, ptr %4, align 4
  %32 = add i32 %31, 1
  %33 = sext i32 %32 to i64
  %34 = getelementptr inbounds i32, ptr %30, i64 %33
  %35 = load i32, ptr %34, align 4
  %36 = icmp sgt i32 %35, 0
  br i1 %36, label %46, label %37

37:                                               ; preds = %29
  %38 = load ptr, ptr %2, align 8
  %39 = load i32, ptr %4, align 4
  %40 = add i32 %39, 1
  %41 = sext i32 %40 to i64
  %42 = getelementptr inbounds i32, ptr %38, i64 %41
  %43 = load i32, ptr %42, align 4
  %44 = load i32, ptr %3, align 4
  %45 = sub nsw i32 %44, %43
  store i32 %45, ptr %3, align 4
  br label %55

46:                                               ; preds = %29
  %47 = load ptr, ptr %2, align 8
  %48 = load i32, ptr %4, align 4
  %49 = add i32 %48, 1
  %50 = sext i32 %49 to i64
  %51 = getelementptr inbounds i32, ptr %47, i64 %50
  %52 = load i32, ptr %51, align 4
  %53 = load i32, ptr %3, align 4
  %54 = add nsw i32 %53, %52
  store i32 %54, ptr %3, align 4
  br label %55

55:                                               ; preds = %46, %37
  br label %56

56:                                               ; preds = %55
  %57 = load ptr, ptr %2, align 8
  %58 = load i32, ptr %4, align 4
  %59 = add i32 %58, 2
  %60 = sext i32 %59 to i64
  %61 = getelementptr inbounds i32, ptr %57, i64 %60
  %62 = load i32, ptr %61, align 4
  %63 = icmp sgt i32 %62, 0
  br i1 %63, label %73, label %64

64:                                               ; preds = %56
  %65 = load ptr, ptr %2, align 8
  %66 = load i32, ptr %4, align 4
  %67 = add i32 %66, 2
  %68 = sext i32 %67 to i64
  %69 = getelementptr inbounds i32, ptr %65, i64 %68
  %70 = load i32, ptr %69, align 4
  %71 = load i32, ptr %3, align 4
  %72 = sub nsw i32 %71, %70
  store i32 %72, ptr %3, align 4
  br label %82

73:                                               ; preds = %56
  %74 = load ptr, ptr %2, align 8
  %75 = load i32, ptr %4, align 4
  %76 = add i32 %75, 2
  %77 = sext i32 %76 to i64
  %78 = getelementptr inbounds i32, ptr %74, i64 %77
  %79 = load i32, ptr %78, align 4
  %80 = load i32, ptr %3, align 4
  %81 = add nsw i32 %80, %79
  store i32 %81, ptr %3, align 4
  br label %82

82:                                               ; preds = %73, %64
  br label %83

83:                                               ; preds = %82
  %84 = load i32, ptr %3, align 4
  ret i32 %84
}

; Function Attrs: noinline nounwind sspstrong uwtable
define dso_local i32 @qux(ptr noundef %0, i32 noundef %1) #0 {
  %3 = alloca ptr, align 8
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  store ptr %0, ptr %3, align 8
  store i32 %1, ptr %4, align 4
  store i32 0, ptr %5, align 4
  store i32 0, ptr %6, align 4
  br label %7

7:                                                ; preds = %90, %2
  %8 = load i32, ptr %6, align 4
  %9 = add i32 %8, 2
  %10 = load i32, ptr %4, align 4
  %11 = icmp slt i32 %9, %10
  br i1 %11, label %12, label %93

12:                                               ; preds = %7
  %13 = load ptr, ptr %3, align 8
  %14 = load i32, ptr %6, align 4
  %15 = sext i32 %14 to i64
  %16 = getelementptr inbounds i32, ptr %13, i64 %15
  %17 = load i32, ptr %16, align 4
  %18 = icmp sgt i32 %17, 0
  br i1 %18, label %19, label %27

19:                                               ; preds = %12
  %20 = load ptr, ptr %3, align 8
  %21 = load i32, ptr %6, align 4
  %22 = sext i32 %21 to i64
  %23 = getelementptr inbounds i32, ptr %20, i64 %22
  %24 = load i32, ptr %23, align 4
  %25 = load i32, ptr %5, align 4
  %26 = add nsw i32 %25, %24
  store i32 %26, ptr %5, align 4
  br label %35

27:                                               ; preds = %12
  %28 = load ptr, ptr %3, align 8
  %29 = load i32, ptr %6, align 4
  %30 = sext i32 %29 to i64
  %31 = getelementptr inbounds i32, ptr %28, i64 %30
  %32 = load i32, ptr %31, align 4
  %33 = load i32, ptr %5, align 4
  %34 = sub nsw i32 %33, %32
  store i32 %34, ptr %5, align 4
  br label %35

35:                                               ; preds = %27, %19
  br label %36

36:                                               ; preds = %35
  %37 = load ptr, ptr %3, align 8
  %38 = load i32, ptr %6, align 4
  %39 = add i32 %38, 1
  %40 = sext i32 %39 to i64
  %41 = getelementptr inbounds i32, ptr %37, i64 %40
  %42 = load i32, ptr %41, align 4
  %43 = icmp sgt i32 %42, 0
  br i1 %43, label %53, label %44

44:                                               ; preds = %36
  %45 = load ptr, ptr %3, align 8
  %46 = load i32, ptr %6, align 4
  %47 = add i32 %46, 1
  %48 = sext i32 %47 to i64
  %49 = getelementptr inbounds i32, ptr %45, i64 %48
  %50 = load i32, ptr %49, align 4
  %51 = load i32, ptr %5, align 4
  %52 = sub nsw i32 %51, %50
  store i32 %52, ptr %5, align 4
  br label %62

53:                                               ; preds = %36
  %54 = load ptr, ptr %3, align 8
  %55 = load i32, ptr %6, align 4
  %56 = add i32 %55, 1
  %57 = sext i32 %56 to i64
  %58 = getelementptr inbounds i32, ptr %54, i64 %57
  %59 = load i32, ptr %58, align 4
  %60 = load i32, ptr %5, align 4
  %61 = add nsw i32 %60, %59
  store i32 %61, ptr %5, align 4
  br label %62

62:                                               ; preds = %53, %44
  br label %63

63:                                               ; preds = %62
  %64 = load ptr, ptr %3, align 8
  %65 = load i32, ptr %6, align 4
  %66 = add i32 %65, 2
  %67 = sext i32 %66 to i64
  %68 = getelementptr inbounds i32, ptr %64, i64 %67
  %69 = load i32, ptr %68, align 4
  %70 = icmp sgt i32 %69, 0
  br i1 %70, label %80, label %71

71:                                               ; preds = %63
  %72 = load ptr, ptr %3, align 8
  %73 = load i32, ptr %6, align 4
  %74 = add i32 %73, 2
  %75 = sext i32 %74 to i64
  %76 = getelementptr inbounds i32, ptr %72, i64 %75
  %77 = load i32, ptr %76, align 4
  %78 = load i32, ptr %5, align 4
  %79 = sub nsw i32 %78, %77
  store i32 %79, ptr %5, align 4
  br label %89

80:                                               ; preds = %63
  %81 = load ptr, ptr %3, align 8
  %82 = load i32, ptr %6, align 4
  %83 = add i32 %82, 2
  %84 = sext i32 %83 to i64
  %85 = getelementptr inbounds i32, ptr %81, i64 %84
  %86 = load i32, ptr %85, align 4
  %87 = load i32, ptr %5, align 4
  %88 = add nsw i32 %87, %86
  store i32 %88, ptr %5, align 4
  br label %89

89:                                               ; preds = %80, %71
  br label %90

90:                                               ; preds = %89
  %91 = load i32, ptr %6, align 4
  %92 = add nsw i32 %91, 3
  store i32 %92, ptr %6, align 4
  br label %7, !llvm.loop !8

93:                                               ; preds = %7, %121
  %94 = load i32, ptr %6, align 4
  %95 = load i32, ptr %4, align 4
  %96 = icmp slt i32 %94, %95
  br i1 %96, label %97, label %124

97:                                               ; preds = %93
  %98 = load ptr, ptr %3, align 8
  %99 = load i32, ptr %6, align 4
  %100 = sext i32 %99 to i64
  %101 = getelementptr inbounds i32, ptr %98, i64 %100
  %102 = load i32, ptr %101, align 4
  %103 = icmp sgt i32 %102, 0
  br i1 %103, label %112, label %104

104:                                              ; preds = %97
  %105 = load ptr, ptr %3, align 8
  %106 = load i32, ptr %6, align 4
  %107 = sext i32 %106 to i64
  %108 = getelementptr inbounds i32, ptr %105, i64 %107
  %109 = load i32, ptr %108, align 4
  %110 = load i32, ptr %5, align 4
  %111 = sub nsw i32 %110, %109
  store i32 %111, ptr %5, align 4
  br label %120

112:                                              ; preds = %97
  %113 = load ptr, ptr %3, align 8
  %114 = load i32, ptr %6, align 4
  %115 = sext i32 %114 to i64
  %116 = getelementptr inbounds i32, ptr %113, i64 %115
  %117 = load i32, ptr %116, align 4
  %118 = load i32, ptr %5, align 4
  %119 = add nsw i32 %118, %117
  store i32 %119, ptr %5, align 4
  br label %120

120:                                              ; preds = %112, %104
  br label %121

121:                                              ; preds = %120
  %122 = load i32, ptr %6, align 4
  %123 = add nsw i32 %122, 1
  store i32 %123, ptr %6, align 4
  br label %93, !llvm.loop !8

124:                                              ; preds = %93
  %125 = load i32, ptr %5, align 4
  ret i32 %125
}

; Function Attrs: noinline nounwind sspstrong uwtable
define dso_local i32 @quux(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  store i32 0, ptr %3, align 4
  store i32 0, ptr %4, align 4
  br label %5

5:                                                ; preds = %15, %1
  %6 = load i32, ptr %4, align 4
  %7 = load i32, ptr %2, align 4
  %8 = icmp slt i32 %6, %7
  br i1 %8, label %9, label %18

9:                                                ; preds = %5
  %10 = load i32, ptr %4, align 4
  %11 = load i32, ptr %3, align 4
  %12 = add nsw i32 %11, %10
  store i32 %12, ptr %3, align 4
  %13 = load i32, ptr %4, align 4
  %14 = add nsw i32 %13, 1
  store i32 %14, ptr %4, align 4
  br label %15

15:                                               ; preds = %9
  %16 = load i32, ptr %4, align 4
  %17 = add nsw i32 %16, 1
  store i32 %17, ptr %4, align 4
  br label %5, !llvm.loop !9

18:                                               ; preds = %5
  %19 = load i32, ptr %3, align 4
  ret i32 %19
}

; Function Attrs: noinline nounwind sspstrong uwtable
define dso_local i32 @main() #0 {
  %1 = alloca i32, align 4
  %2 = alloca [4 x i32], align 16
  store i32 0, ptr %1, align 4
  call void @llvm.memcpy.p0.p0.i64(ptr align 16 %2, ptr align 16 @__const.main.a, i64 16, i1 false)
  %3 = getelementptr inbounds [4 x i32], ptr %2, i64 0, i64 0
  %4 = call i32 @foo(ptr noundef %3)
  %5 = call i32 @bar(i32 noundef 10)
  %6 = add nsw i32 %4, %5
  %7 = getelementptr inbounds [4 x i32], ptr %2, i64 0, i64 0
  %8 = call i32 @baz(ptr noundef %7)
  %9 = add nsw i32 %6, %8
  %10 = getelementptr inbounds [4 x i32], ptr %2, i64 0, i64 0
  %11 = call i32 @qux(ptr noundef %10, i32 noundef 4)
  %12 = add nsw i32 %9, %11
  %13 = call i32 @quux(i32 noundef 10)
  %14 = add nsw i32 %12, %13
  ret i32 %14
}

; Function Attrs: nocallback nofree nounwind willreturn memory(argmem: readwrite)
declare void @llvm.memcpy.p0.p0.i64(ptr noalias writeonly captures(none), ptr noalias readonly captures(none), i64, i1 immarg) #1

attributes #0 = { noinline nounwind sspstrong uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nocallback nofree nounwind willreturn memory(argmem: readwrite) }

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
!8 = distinct !{!8, !7}
!9 = distinct !{!9, !7}
