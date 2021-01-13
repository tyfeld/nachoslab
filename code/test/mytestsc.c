#include "syscall.h"
// int fd1, fd2;
// char* buf;
// int
// main()
// {
//     Create("one.txt");
//     Create("two.txt");
//     fd1 = Open("one.txt");
//     Write("MyTestFile",15,fd1);
//     fd2 = Open("two.txt");
//     Read(buf,15,fd1);
//     Write(buf,15,fd2);
//     Close(fd1);
//     Close(fd2);
//     Halt();
// }

int func()
{
    Exit(0);
}

int main()
{
    Fork(func);
    Yield();
    Exit(Join(Exec("../test/sort")));
}