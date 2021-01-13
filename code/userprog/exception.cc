// exception.cc
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "translate.h"
#include "machine.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The  of the system call, if any, must be put back into r2.
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions
//	are in machine.h.
//----------------------------------------------------------------------
void PagefaultHandler(int vpn);
void TLBMissHandler(int virtAddr);
void ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    if (which == SyscallException)
    {
        if (type == SC_Halt)
        {
            DEBUG('T', "TLB Miss Count: %d, Total Translate: %d, TLB Miss Rate: %.2lf%%\n", TLbMissCount, TranslateCount, (double)(TLbMissCount * 100) / (TranslateCount));
            DEBUG('a', "Shutdown, initiated by user program.\n");
            interrupt->Halt();
        }
        else if (type == SC_Exit)
        {
            printf("Thread exit.\n");
            printf("Inverted Page Table Info:\n");
            printf("Physical Page\tvalid\tvpn\tpid\n");
            for (int i = 0; i < NumPhysPages; i++)
            {
                printf("%d\t\t", machine->pageTable[i].physicalPage);
                if (machine->pageTable[i].valid)
                {
                    printf("FALSE\t");
                    printf("%d\t%d\n", machine->pageTable[i].virtualPage, machine->pageTable[i].pid);
                }
                else
                {
                    printf("TRUE\t\t\t\n");
                }
            }
            if (currentThread->space)
            {
                int nextPc = machine->ReadRegister(NextPCReg);
                machine->WriteRegister(PCReg, nextPc);
                machine->ReclaimMem();
                delete currentThread->space;
                currentThread->space = NULL;
                currentThread->Finish();
                printf("Exit!\n");
            }
        }
        else if (type == SC_Create)
        {
            printf("Syscall: CREATE\n");
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
            fileSystem->Create(name, 128);
            machine->AdvancePC();
        }
        else if (type == SC_Open)
        {
            printf("Syscall: Open\n");
            int address = machine->ReadRegister(4);
            int pos = 0;
            char name[10];

            while (1)
            {
                int data;
                machine->ReadMem(address + pos, 1, &data);
                name[pos++] = (char)data;
                if (!data)
                {
                    break;
                }
            }
            OpenFile *openfile = fileSystem->Open(name);
            machine->WriteRegister(2, int(openfile));
            machine->AdvancePC();
        }
        else if (type == SC_Close)
        {
            printf("Syscall: Close\n");
            int id = machine->ReadRegister(4);
            OpenFile *openFile = (OpenFile *)id;
            delete openFile;
            machine->AdvancePC();
        }
        else if (type == SC_Read)
        {
            printf("Syscall: Read\n");
            int address = machine->ReadRegister(4);
            int len = machine->ReadRegister(5);
            int fd = machine->ReadRegister(6);
            char *data = new char[len];
            int res;
            OpenFile *openfile = (OpenFile *)fd;
            res = openfile->Read(data, len);
            for (int i = 0; i < res; i++)
            {
                machine->WriteMem(address + i, 1, int(data[i]));
            }
            machine->WriteRegister(2, res);
            machine->AdvancePC();
        }
        else if (type == SC_Write)
        {
            printf("Syscall: Write\n");
            int buff = machine->ReadRegister(4);
            int size = machine->ReadRegister(5);
            int fd = machine->ReadRegister(6);
            char *data = new char[size];
            int value;
            for (int i = 0; i < size; i++)
            {
                machine->ReadMem(buff + i, 1, &value);
                data[i] = char(value);
            }
            OpenFile *openfile = (OpenFile *)fd;
            openfile->Write(data, size);
            machine->AdvancePC();
        }
        else if (type == SC_Exit)
        {
            printf("Syscall: Exit\n");
            machine->AdvancePC();
            currentThread->Finish();
        }
        else if (type == SC_Exec)
        {
            printf("Syscall: Exec\n");
            int address = machine->ReadRegister(4);
            Thread *newThread = new Thread("new thread");
            newThread->Fork(exec, address);
            machine->WriteRegister(2, newThread->getThreadId());
            machine->AdvancePC();
        }
        else if (type == SC_Fork)
        {
            printf("Syscall: Fork");
            int func = machine->ReadRegister(4);
            OpenFile *executable = fileSystem->Open(currentThread->filename);
            AddrSpace *space = new AddrSpace(executable);
            space->AddrSpaceCpy(currentThread->space);
            Thread *t = new Thread("new thread");
            t->space = space;
            t->Fork(fork, func);
            machine->AdvancePC();
        }
        else if (type == SC_Yield)
        {
            printf("Syscall: Yield");
            machine->AdvancePC();
            currentThread->Yield();
        }
        else if (type == SC_Join)
        {
            printf("SC_Join called\n");
            int threadId = machine->ReadRegister(4);
            while (pids[threadId] == true)
                currentThread->Yield();
            machine->AdvancePC();
        }
    }
    else if (which == PageFaultException)
    {
        //increase miss count
        TLbMissCount += 1;
        if (machine->tlb == NULL)
        {
            ASSERT(FALSE);
        }
        else
        {
            DEBUG('m', "TLB MISS!\n");
            int BadVAddr = machine->ReadRegister(BadVAddrReg);

            //TLBMissHandlerFIFO(BadVAddr);
            TLBMissHandlerFIFO(BadVAddr);
        }
        return;
    }
    else
    {
        printf("Unexpected user mode exception %d %d\n", which, type);
        ASSERT(FALSE);
    }
}

void exec(int which)
{
    currentThread->space->InitRegisters();
    currentThread->space->RestoreState();
    machine->Run();
    ASSERT(false);
}

void fork(void(*func))
{
    currentThread->space->InitRegisters();
    currentThread->space->RestoreState();
    machine->WriteRegister(PCReg, func);
    machine->WriteRegister(NextPCReg, func + 4);
    machine->Run();
    ASSERT(false);
}
//----------------------------------------------------------------------
// TLBMissHandlerFIFO
// Use FIFO replacement algorithm for tlb miss handler.
//----------------------------------------------------------------------

void TLBMissHandlerFIFO(int virtAddr)
{
    int positionFIFO = -1;
    int pos;
    unsigned int vpn;
    vpn = (unsigned)virtAddr / PageSize;
    //Find empty entry
    for (pos = 0; pos < TLBSize; pos++)
    {
        if (machine->tlb[pos].valid == FALSE)
        {
            positionFIFO = pos;
            break;
        }
    }
    //FIFO replacing
    if (positionFIFO == -1)
    {
        positionFIFO = TLBSize - 1;
        for (pos = 0; pos < TLBSize - 1; pos++)
        {
            machine->tlb[pos] = machine->tlb[pos + 1];
        }
    }
    machine->tlb[positionFIFO] = machine->pageTable[vpn];
}

//----------------------------------------------------------------------
// TLBMissHandlerClock
// Use Clock replacement algorithm for tlb miss handler.
//---------------------------------------------------------------------

int clockpoint = 0;
void TLBMissHandlerClock(int virtAddr)
{
    unsigned int vpn;
    vpn = (unsigned)virtAddr / PageSize;
    while (true)
    {
        clockpoint %= TLBSize;
        if (machine->tlb[clockpoint].valid == FALSE)
        {
            break;
        }
        else
        {
            //Second Chance
            if (machine->tlb[clockpoint].use) // R = 1
            {
                machine->tlb[clockpoint].use = FALSE;
                clockpoint += 1;
            }
            else
            { // R = 0
                break;
            }
        }
    }
    machine->tlb[clockpoint] = machine->pageTable[vpn];
    machine->tlb[clockpoint].use = TRUE;
}

//----------------------------------------------------------------------
// TLBMissHandler
// Handle tlb miss and page fault.
//----------------------------------------------------------------------

void TLBMissHandler(int virtAddr)
{
    unsigned int vpn;
    vpn = (unsigned)virtAddr / PageSize;
    if (machine->pageTable[vpn].valid)
    {
        TLBMissHandlerClock(virtAddr);
    }
    else
    {
        DEBUG('M', "Page fault with vpn %d!\n", vpn);
        PagefaultHandler(vpn);
    }
}

void PagefaultHandler(int vpn)
{
    int pageFrame = machine->AllocateMem();
    if (pageFrame == -1)
    {
        for (int targetvpn1 = 0; targetvpn1 < machine->pageTableSize, targetvpn1 != vpn; targetvpn1++)
        {
            if (machine->pageTable[targetvpn1].valid && !machine->pageTable[targetvpn1].dirty)
            {
                pageFrame = machine->pageTable[targetvpn1].physicalPage;
                break;
            }
        }
        if (pageFrame == -1)
        {
            for (int targetvpn2 = 0; targetvpn2 < machine->pageTableSize, targetvpn2 != vpn; targetvpn2++)
            {
                if (machine->pageTable[targetvpn2].valid)
                {
                    machine->pageTable[targetvpn2].valid = FALSE;
                    pageFrame = machine->pageTable[targetvpn2].physicalPage;
                    OpenFile *vm = fileSystem->Open("VM");
                    vm->WriteAt(&(machine->mainMemory[pageFrame * PageSize]), PageSize, targetvpn2 * PageSize);
                    delete vm;
                    break;
                }
            }
        }
    }
    machine->pageTable[vpn].physicalPage = pageFrame;
    OpenFile *vm = fileSystem->Open("VM");
    vm->ReadAt(&(machine->mainMemory[pageFrame * PageSize]), PageSize, vpn * PageSize);
    delete vm;
    machine->pageTable[vpn].valid = TRUE;
    machine->pageTable[vpn].use = FALSE;
    machine->pageTable[vpn].dirty = FALSE;
    machine->pageTable[vpn].readOnly = FALSE;
}

// int PageReplacementAlgo(int vpn)
// {
//     int pageFrame = -1;
//     for (int targetvpn1 = 0; targetvpn1 < machine->pageTableSize, targetvpn1 != vpn; targetvpn1++) {
//         if (machine->pageTable[targetvpn1].valid) {
//             if (!machine->pageTable[targetvpn1].dirty) {
//                 pageFrame = machine->pageTable[targetvpn1].physicalPage;
//                 break;
//             }
//         }
//     }
//     if (pageFrame == -1) {
//         for (int targetvpn2 = 0; targetvpn2 < machine->pageTableSize, targetvpn2 != vpn; targetvpn2++) {
//             if (machine->pageTable[targetvpn2].valid) {
//                 machine->pageTable[targetvpn2].valid = FALSE;
//                 pageFrame = machine->pageTable[targetvpn2].physicalPage;
//                 OpenFile *vm = fileSystem->Open("VirtualMemory");
//                 vm->WriteAt(&(machine->mainMemory[pageFrame*PageSize]), PageSize, targetvpn2*PageSize);
//                 delete vm;
//                 break;
//             }
//         }
//     }
//     return pageFrame;
// }