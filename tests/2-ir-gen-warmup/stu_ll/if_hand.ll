define dso_local i32 @main(){
    ; %1%2 指向 a与返回值
    %1 = alloca float, align 4
    %2 = alloca i32, align 4
    store float 0x40B1C28FE0000000, float* %1
    ; 用 fcmp 进行比较
    %3 = load float, float* %1
    %4 = fcmp ugt float %3, 1.0
    br i1 %4, label %5, label %6
5:
    store i32 233, i32* %2
    br label %7
6:
    store i32 0, i32* %2
    br label %7
7:
    ; 取%2, 返回
    %8 = load i32, i32* %2
    ret i32 %8
}