define dso_local i32 @callee(i32 %0){
    ; %2,%3 指向 a与返回值
    %2 = alloca i32, align 4
    %3 = alloca i32, align 4
    store i32 %0, i32* %2
    ; 取数计算，存入%3
    %4 = load i32, i32* %2
    %5 = mul i32 2, %4
    store i32 %5, i32* %3
    ; 取出%3,返回
    %6 = load i32, i32* %3
    ret  i32 %6
}

define dso_local i32 @main(){
    ; %1 指向 返回值
    %1 = alloca i32, align 4
    ; 函数调用,将函数返回值存入%3
    %2 = call i32 @callee(i32 110)
    store i32 %2, i32* %1
    ; 取出%3,返回
    %3 = load i32, i32* %1
    ret i32 %3
}