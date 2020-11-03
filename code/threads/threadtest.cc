// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"

// testnum is set in main.cc
int testnum = 1;

//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void
SimpleThread(int which)
{
    int num;
    
    for (num = 0; num < 5; num++) {
	//printf("*** thread %d looped %d time , uid: %d\n", currentThread->getpid(), num, currentThread->getuid());
	    printf("*** thread %d looped %d time, priority: %d\n", currentThread->getpid(), num, currentThread->getPriority());
        //currentThread->Yield();
        //scheduler->Print();
    }
    
}
//----------------------------------------------------------------------
// SimpleThreadTS
// Print information and then yield.
//----------------------------------------------------------------------

void
SimpleThreadTS(int which)
{
    printf("Thread %d yielding...\n", currentThread->getpid());
    currentThread->Print();
    scheduler->Print();
    currentThread->Yield();   
}
//----------------------------------------------------------------------
// SimpleThreadPree
// Fork another thread in the first cycle.
//----------------------------------------------------------------------

void
SimpleThreadPree(int which)
{
    int num;
    for (num = 0; num < 5; num++){
        if (num==0){
            Thread *t2 = new Thread("for test", 1);
            t2->Fork(SimpleThread,0);
        }
        printf("*** thread %d looped %d time, priority: %d\n", currentThread->getpid(), num, currentThread->getPriority());
    }
}
//----------------------------------------------------------------------
// SimpleThreadRR
// Fork another thread in the first cycle.
//----------------------------------------------------------------------

void
SimpleThreadRR(int which)
{
    int num;
    for (num = 0; num < 15; num++){
        printf("*** thread %d looped %d time, priority: %d, time: %d\n", 
        currentThread->getpid(), num, currentThread->getPriority(), stats->totalTicks);
        interrupt->SetLevel(IntOn);
        interrupt->SetLevel(IntOff);
    }
}
//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");

    Thread *t = new Thread("forked thread");

    t->Fork(SimpleThread, 1);
    SimpleThread(0);
}
//----------------------------------------------------------------------
// ThreadTestTS
// 	Set up a ping-pong between four threads, by forking a thread 
//	Then call SimpleThreadTS to print all status and information.
//----------------------------------------------------------------------

void 
ThreadTestTS()
{
    printf("PID \t NAME     \t UID \t STATUS\n");
    Thread *t1 = new Thread("forked thread");
    Thread *t2 = new Thread("forked thread");    
    Thread *t3 = new Thread("forked thread");
    Thread *t4 = new Thread("forked thread");
    t1->Fork(SimpleThreadTS, 0);
    t2->Fork(SimpleThreadTS, 0);
    t3->Fork(SimpleThreadTS, 0);
    t4->Fork(SimpleThreadTS, 0);
    //scheduler->Print();
}


//----------------------------------------------------------------------
// ThreadTestMax
// 	Test the number limit of threads.
//----------------------------------------------------------------------

void 
ThreadTestMax()
{
    for (int i = 0; i <= MaxThreads; i++){
        Thread *t = new Thread("for test");
        printf("*** thread %d created\n", t->getpid());
    }
}

//----------------------------------------------------------------------
// ThreadTestPri
// 	Test priority scheduler.
//----------------------------------------------------------------------

void 
ThreadTestPri()
{
    Thread *t1 = new Thread("for test", 3);
    t1->Fork(SimpleThread,0);
    Thread *t2 = new Thread("for test", 7);
    t2->Fork(SimpleThread,0);
    Thread *t3 = new Thread("for test", 5);
    t3->Fork(SimpleThread,0);
}
//----------------------------------------------------------------------
// ThreadTestPree
// 	Test preemtive algorithm.
//----------------------------------------------------------------------

void 
ThreadTestPree()
{
    Thread *t1 = new Thread("for test", 3);
    t1->Fork(SimpleThreadPree,0);
}
//----------------------------------------------------------------------
// ThreadTestRR
// 	Test Round Robin
//----------------------------------------------------------------------

void
ThreadTestRR()
{
    Thread *t1 = new Thread("test RR");
    Thread *t2 = new Thread("test RR");
    Thread *t3 = new Thread("test RR");
    t1->Fork(SimpleThreadRR,0);
    t2->Fork(SimpleThreadRR,0);
    t3->Fork(SimpleThreadRR,0);
}

//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------

void
ThreadTest()
{
    switch (testnum) {
    case 1:
	    ThreadTest1();
        break;
    case 2:
        ThreadTestTS();
        break;
    case 3:
        ThreadTestMax();
        break;
    case 4:
        ThreadTestPri();
        break;
    case 5:
        ThreadTestPree();
        break;
    case 6:
        ThreadTestRR();
        break;
    default:
	    printf("No test specified.\n");
	    break;
    }
}

