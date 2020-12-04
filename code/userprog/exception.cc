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
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    if ((which == SyscallException) && (type == SC_Halt)) {
        DEBUG('T', "TLB Miss Count: %d, Total Translate: %d, TLB Miss Rate: %.2lf%%\n",TLbMissCount, TranslateCount, (double)(TLbMissCount*100)/(TranslateCount)); 
	    DEBUG('a', "Shutdown, initiated by user program.\n");
   	    interrupt->Halt();
    } 
    else if(which == PageFaultException ){
        //increase miss count
        TLbMissCount += 1;
        if (machine->tlb == NULL){
            ASSERT(FALSE);
        }
        else {
            DEBUG('m',"TLB MISS!\n");
            int BadVAddr = machine->ReadRegister(BadVAddrReg);
            //TLBMissHandlerFIFO(BadVAddr);
            TLBMissHandlerClock(BadVAddr);
        }
        return;
    }
    else {
	printf("Unexpected user mode exception %d %d\n", which, type);
	ASSERT(FALSE);
    }
}

//----------------------------------------------------------------------
// TLBMissHandlerFIFO 
// Use FIFO replacement algorithm for tlb miss handler.
//----------------------------------------------------------------------

void
TLBMissHandlerFIFO(int virtAddr)
{
    int positionFIFO = -1;
    int pos;
    unsigned int vpn;
    vpn = (unsigned) virtAddr / PageSize;
    //Find empty entry
    for (pos = 0; pos < TLBSize; pos++){
        if (machine->tlb[pos].valid == FALSE){
            positionFIFO = pos;
            break;
        }
    }
    //FIFO replacing
    if (positionFIFO == -1){
        positionFIFO = TLBSize - 1;
        for (pos = 0; pos < TLBSize-1; pos++){
            machine->tlb[pos] = machine->tlb[pos+1];
        }
    }
    machine->tlb[positionFIFO] = machine->pageTable[vpn];
}



//----------------------------------------------------------------------
// TLBMissHandlerClock
// Use Clock replacement algorithm for tlb miss handler.
//---------------------------------------------------------------------

int clockpoint = 0;
void
TLBMissHandlerClock(int virtAddr)
{
    unsigned int vpn;
    vpn = (unsigned) virtAddr / PageSize;
    while (true)
    {
        clockpoint %= TLBSize;
        if (machine->tlb[clockpoint].valid == FALSE){
            break;
        }
        else
        {
        //Second Chance 
            if (machine->tlb[clockpoint].use)  // R = 1
            {
                machine->tlb[clockpoint].use = FALSE;
                clockpoint += 1;
            }
            else { // R = 0
                break;
            }
        } 
    }
    machine->tlb[clockpoint] = machine->pageTable[vpn];
    machine->tlb[clockpoint].use = TRUE;  
}