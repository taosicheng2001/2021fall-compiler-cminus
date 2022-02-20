define dso_local i32 @main() #0 {
    ; %1%2 指向 数组和返回值
    %1 = alloca [10 x i32], align 64
    %2 = alloca i32, align 4
    ; %3 指向a[0] %4 指向a[1]
    %3 = getelementptr [10 x i32], [10 x i32]* %1, i32 0, i32 0
    store i32 10, i32* %3
    %4 = getelementptr [10 x i32], [10 x i32]* %1, i32 0, i32 1
    %5 = load i32, i32* %3
    %6 = mul i32 2, %5
    store i32 %6, i32* %4
    ; 保存返回值
    %7 = load i32, i32* %4
    store i32 %7, i32* %2
    ; 取返回值，返回
    %8 = load i32, i32* %2
    ret i32 %8
}