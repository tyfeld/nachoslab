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
#include "synch.h"

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
// Producer & Consumer Problem implemented by semaphores.
//----------------------------------------------------------------------

int N = 6;
Semaphore* empty = new Semaphore("empty",N);
Semaphore* full = new Semaphore("full",0);
Semaphore* mutex = new Semaphore("mutex", 1);
int numSema = 0;
void ProducerSema(int val)
{
    for (int i = 0; i < val; i++)
    {
        empty->P();
        mutex->P();
        if(numSema >= N){ 
    	    printf("Buffer is full, waiting for consumers.\n");
        }
        else {
            printf("%s produced 1 item, items in buffer changed from %d to %d\n",currentThread->getName(),numSema,numSema+1);
            ++numSema;
        }
        mutex->V();
        full->V();
    }
}

void ConsumerSema(int val)
{
    for (int i = 0; i < val; i++)
    {
        full->P();
        mutex->P();
        if(numSema <= 0){ 
    	    printf("Buffer is empty, waiting for producers.\n");
        }
        else {
            printf("%s consume 1 item, items in buffer changed from %d to %d\n",currentThread->getName(),numSema,numSema-1);
            --numSema;
        }
        mutex->V();
        empty->V();
    }
}
//----------------------------------------------------------------------
// Producer & Consumer Problem implemented by conditional variables.
//----------------------------------------------------------------------

Lock* pcLock = new Lock("PCLock");
Condition* notempty = new Condition("NotEmpty");
Condition* notfull = new Condition("Notfull");
int numCond = 0;
void ProducerCond(int val)
{
    for (int i = 0; i < val; i++)
    {
        pcLock->Acquire();
        while(numCond == N) {
    	    printf("Buffer is full, waiting for consumers.\n");
            notfull->Wait(pcLock);
        }
        printf("%s produced 1 item, items in buffer changed from %d to %d\n",currentThread->getName(),numCond,numCond+1);
        ++numCond;
        notempty->Signal(pcLock);
        pcLock->Release();
    }
}
void ConsumerCond(int val)
{
    for (int i = 0; i < val; i++)
    {
        pcLock->Acquire();
        while(numCond == 0) {
    	    printf("Buffer is empty, waiting for producers.\n");
            notempty->Wait(pcLock);
        }
        printf("%s consumed 1 item, items in buffer changed from %d to %d\n",currentThread->getName(),numCond,numCond-1);
        --numCond;
        notfull->Signal(pcLock);
        pcLock->Release();
    }
}

// Implementing Barrier.
Condition* barrier = new Condition("barrier");
Lock* barrLock = new Lock("barrLock");
int barrierCnt = 0;
const int barrierLimit = 6;

void Barrier(int val)
{
    barrLock->Acquire();
    barrierCnt++;
    if(barrierCnt < barrierLimit)
    {
        printf("Waiting for more... Present threads: %d, Barrier threads: %d\n",barrierCnt,barrierLimit);
        barrier->Wait(barrLock);
    }
    else //meet barrier
    {
        printf("Meeting Barrier Now! Waking all waiting threads.\n");
        barrier->Broadcast(barrLock);
    }   
    barrLock->Release();
    printf("Thread %d continue to run.\n", val);
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
// ThreadTestPCSema
// Producers & Consumers Problem implemented by semaphores.
//---------------------------------------------------------------------

void 
ThreadTestPCSema()
{
    Thread* t1 = new Thread("Producer");
    Thread* t2 = new Thread("Consumer1");
    Thread* t3 = new Thread("Consumer2");
    t1->Fork(ProducerSema,12);
    t2->Fork(ConsumerSema,6);
    t3->Fork(ConsumerSema,6);
}

//----------------------------------------------------------------------
// ThreadTestPCCond
// Producers & Consumers Problem implemented by Conditional variables.
//---------------------------------------------------------------------

void 
ThreadTestPCCond()
{
    Thread* t1 = new Thread("Producer");
    Thread* t2 = new Thread("Consumer1");
    Thread* t3 = new Thread("Consumer2");
    t1->Fork(ProducerCond,12);
    t2->Fork(ConsumerCond,6);
    t3->Fork(ConsumerCond,6);
}
//----------------------------------------------------------------------
// ThreadTestBarrier
// Implementing barrier by Conditional variables.
//---------------------------------------------------------------------

void 
ThreadTestBarrier()
{
    for (int i = 1; i <= barrierLimit; i++)
    {
        Thread *t = new Thread("BarrierTest");
        t -> Fork(Barrier,i);
    }
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
    case 7:
        ThreadTestPCSema();
        break;
    case 8:
        ThreadTestPCCond();
        break;
    case 9:
        ThreadTestBarrier();
        break;
    default:
	    printf("No test specified.\n");
	    break;
    }
}

