# Shell实现实验报告

> 设计实现一个用户程序shell，通过./nachos -x shell进入用户交互界面中。在该界面中可以查询支持的功能、可以创建删除文件或目录、可以执行另一个用户程序并输出运行结果，类似Linux上跑的bash。





<div STYLE="page-break-after: always;"></div>

## 内容一：任务完成情况
### 任务完成列表
 

| Exercise1 |
|:---------:|
| Y         |

### 具体Exercise完成情况
#### 前期准备
实习要求为创建一个shell用户程序来处理类似Linux bash的相关cli指令。在`test/shell.c`中，可以发现其中的部分铺垫已经由Nachos完成。我们的实现即在这一部分继续完善。`shell.c`的内容如下：
```c
int
main()
{
    SpaceId newProc;
    OpenFileId input = ConsoleInput;
    OpenFileId output = ConsoleOutput;
    char prompt[2], ch, buffer[60];
    int i;

    prompt[0] = '-';
    prompt[1] = '-';

    while( 1 )
    {
	Write(prompt, 2, output);

	i = 0;
	
	do {
	
	    Read(&buffer[i], 1, input); 

	} while( buffer[i++] != '\n' );

	buffer[--i] = '\0';

	if( i > 0 ) {
		newProc = Exec(buffer);
		Join(newProc);
	}
    }
}
```
可以看到，它通过调用lab6实现的`Read`和`Write`系统调用，将用户在命令行的输入解析，储存到`buffer`字符数组中。根据`buffer`数组的内容来实现相关的系统调用。  
为了处理对于Console的读写操作，在lab6中对于`Read,Write`的系统调用部分需要增加对于`fd==ConsoleInput, fd==ConsoleOutput`的处理部分，使得用户能够直接对终端内容进行写入和读出。  
```cpp
// Read
if (fd == ConsoleInput)
{
    for (int i = 0; i < len; i++)
        data[i] = getchar();
    result = len;
}

//Write
if (fd == ConsoleOutput)
{
    for(int i = 0; i < len; i++)
			putchar(data[i]);
}
```

最后，`prompt`数组中存放的是每一行打印的提示标识符。

对于以下每个增加的系统调用，需要在`syscall.h`,`start,s`中声明，并在`exception.cc`中定义，具体方式已在上一个lab中演示。


#### x
运行用户程序。这一部分可以直接在原有的`shell.c`中补充：
```c
if (!strncmp(buffer, "x", 1))
{
    newProc = Exec(buffer + 2);
    Join(newProc);
}
```

#### pwd, ls
打印出当前目录/当前目录中的所有文件。这一部分需要获取当前的目录，在原先实现的文件系统中残缺，参考资料发现可以用`system`命令实现。
```cpp
\\exception.cc
else if (type == SC_Pwd)
{
    system("pwd");
    machine->AdvancePC();
}
else if (type == SC_Ls)
{
    system("ls");
    machine->AdvancePC();
}
\\shell.c
else if (!strncmp(buffer, "ls", 2))
{
    Ls();
}
else if (!strncmp(buffer, "pwd", 3))
{
    Pwd();
}
```

#### cd
进入指定的目录。可以基于c函数`chdir`来实现。新增一个系统调用`SC_Cd`。仿照上次的创建文件`Create`的处理方式，将文件创建语句改为进入指定路径。
```cpp
//exception.cc
else if (type == SC_Cd)
{
    ...
    //fileSystem->Create(name, 0);
    chdir(name);
    ...
}
//shell.c
else if (!strncmp(buffer, "cd", 2))
{
   Cd(buffer+3);
}
```
#### touch, mkdir, rm
文件的创建和删除相关。对于`touch`，创建文件可以直接调用之前实现的系统调用`Create`来创建相关文件：
```c
else if (!strncmp(buffer, "touch", 5))
{
    Create(buffer+6);
}
```
对于`mkdir`，要求创建的是目录，需要添加一个新的系统调用`CreateDir`。实现过程中，我们使用lab5中的文件系统，修改`Create`的参数即可。
```cpp
//exception.cc
else if (type == SC_CreateDir)
{
    ...
    fileSystem->Create(name, 0, FALSE);
    ...
}
//shell.c
else if (!strncmp(buffer, "mkdir", 5))
{
    CreateDir(buffer+6);
}
```
对于`rm`，同样使用之前实现过的，支持递归删除的`fileSystem->Remove`。处理系统调用的过程和触发的过程如下：
```cpp
//exception.cc
else if (type == SC_Remove)
{
    int address = machine->ReadRegister(4);
    int pos = 0;
    char name[10];
    while (1)
    {
        int data;
        machine->ReadMem(address + pos, 1, &data);
        //printf("%s\n",(char*)data);
        name[pos++] = (char)data;
        if (!data)
        {
            break;
        }
    }
    fileSystem->Remove(name);
    machine->AdvancePC();
}
//shell.c
else if (!strncmp(buffer, "rm", 2))
{
    CreateDir(buffer+3);
}
```

#### help
输出帮助信息。
```c
//shell.c
else if (!strncmp(buffer, "help", 4))
{
    printf("Usage: \n");
    printf("    ls: list files and directories in current directroy\n");
    printf("    pwd: print current directory\n");
    printf("    mkdir [-name]: create new directory\n");
    printf("    touch [-name]: create new file\n");
    printf("    rm [-name]: remove a file or directory\n")
    printf("    x [-name]: run a user program")
}
```


#### 测试结果：
在`userprog`和`test`中都编译后，终端输入命令`./nachos -x ../test/shell`，开始测试：
```bash
root@56618acc2196:/home/code/userprog# ./nachos -x ../test/shell
--pwd
/home/code/userprog
--mkdir lab7
--cd lab7
--pwd
/home/code/userprog/lab7
--ls
--touch test_file
--mkdir test_dir
--ls
test_file test_dir
--cd test_dir
--ls
--help
Usage:
    ls: list files and directories in current directroy
    pwd: print current directory
    mkdir [-name]: create new directory
    touch [-name]: create new file
    rm [-name]: remove a file or directory
    x [-name]: run a user program
--cd ..
--x halt
No threads ready or runnable, and no pending interrupts.
Assuming the program completed.
Machine halting!
...
```

可以看到基本完成的需求的功能。


## 内容二：遇到的困难以及收获
主要是对于文件目录跳转部分，由于之前的文件系统缺少对这一块的实现，所以调用了c函数命令来完成。其他部分相当于只需要简单应用上一个lab中的系统调用，因此没有遇到大的困难。  
通过这次lab，再一次巩固了系统调用的实现和使用方法，并且简单模拟了一个bash shell的交互方式。由于时间原因，各方面的实现都相对基础。

## 内容三：对课程或Lab的意见和建议
无

## 内容四：参考文献
1. Nachos中文教程。
2. <https://github.com/Bug-terminator/Nachos/blob/master/labs/Shell/README.md>
3. <https://wenku.baidu.com/view/a3e1f237a31614791711cc7931b765ce04087a54.html?fr=search-1&fixfr=LWjggFmNizxl8xYrBy4q3A%3D%3D>