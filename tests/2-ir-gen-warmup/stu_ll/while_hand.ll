define dso_local i32 @main(){
    ; %1%2%3 指向a,i,返回值
    %1 = alloca i32, align 4
    %2 = alloca i32, align 4
    %3 = alloca i32, align 4
    ; 给a,i赋初值
    store i32 10, i32* %1
    store i32 0, i32* %2
    ; 比较 i < 10
    br label %4
4:
    %5 = load i32,i32* %2
    %6 = icmp slt i32 %5, 10
    br i1 %6, label %7, label %13
7:
    ; i = i + 1
    %8 = load i32, i32* %2
    %9 = add i32 %8, 1
    store i32 %9, i32* %2
    ; a = a + i
    %10 = load i32, i32* %1
    %11 = load i32, i32* %2
    %12 = add i32 %10, %11
    store i32 %12, i32* %1
    br label %4
13:
    %14 = load i32, i32* %1
    store i32 %14, i32* %3
    %15 = load i32, i32* %3
    ret i32 %15

}