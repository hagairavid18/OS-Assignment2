#include "kernel/types.h"
#include "kernel/fs.h"

#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"

#define NUM_OF_CHILDREN 20
#define SIG_FLAG1 10
#define SIG_FLAG2 20

int flag1 = 0;
int flag2 = 0;
int flag3 = 0;
int count = 0;
int count2 = 0;
int count3 = 0;


void dummyFunc1(int);
void dummyFunc2(int);
void incCount3(int);
void incCount2(int);
void incCount(int);
void itoa(int, char *);
void usrKillTest(void);
void reverse(char[]);


// void killTest(){
//   printf("starting kill test\n");
//   int children[NUM_OF_CHILDREN];
//   for(int i=0;i<NUM_OF_CHILDREN;++i){
//     children[i] = fork();
//     if(children[i]==0){while(1);exit(1);}
//   }
//   for(int i=0;i<NUM_OF_CHILDREN;++i){
//     kill(children[i],SIGKILL);
//   }
//   for(int i=0;i<NUM_OF_CHILDREN;++i){wait(1);}
//   printf(2,"All children killed!\n");
// }

void thread_test1()
{

    char *stacks[40];
    for (int i = 0; i < 40; i++)
    {
        stacks[i] = ((char *)malloc(4000 * sizeof(char)));
    }
    //int pid[5];
    int cid;
    int childs1[7];
    int childs2[7];

    if ((cid = fork()) == 0)
    {
        for (int j = 0; j < 7; j++)
        {
            int child1 = kthread_create(&incCount2, stacks[j]);
            childs1[j] = child1;
        }
        for (int j = 0; j < 7; j++)
        {
            kthread_join(childs1[j]);
        }
        kthread_exit(0);
    }

    if ((cid = fork()) == 0)
    {
        for (int j = 0; j < 7; j++)
        {
            int child2 = kthread_create(&incCount2, stacks[7 + j]);
            childs2[j] = child2;
        }
        for (int j = 0; j < 7; j++)
        {
            kthread_join(childs2[j]);
        }
        kthread_exit(0);
    }

    wait((void *)0);
    wait((void *)0);
        printf("passed first thread test!\n");

    exit(0);

}
void thread_test2()
{

    char *stacks[14];
    for (int i = 0; i < 14; i++)
    {
        stacks[i] = ((char *)malloc(4000 * sizeof(char)));
    }
    //int pid[5];
    int childs1[7];
    int childs2[7];

    for (int j = 0; j < 7; j++)
    {
        int child1 = kthread_create(&incCount2, stacks[j]);
        printf("child id is %d\n",child1);
        childs1[j] = child1;
    }
    for (int j = 0; j < 7; j++)
    {
        kthread_join(childs1[j]);
    }
 for (int j = 0; j < 7; j++)
    {
        int child2 = kthread_create(&incCount3, stacks[7+j]);
        printf("child id is %d\n",child2);
        childs2[j] = child2;
    }
    for (int j = 0; j < 7; j++)
    {
        kthread_join(childs2[j]);
    }
    

    printf("count2 is %d\n",count2);
    printf("count3  is %d\n",count3);
    //kthread_exit(0);

    

    printf("passed second thread test!\n");
}

// int child3 =  kthread_create(&incCount,stack3);
// int child4 =  kthread_create(&incCount,stack4);

//printf("in tests, id of child is %d\n",child);
//printf("func adress is %p",&incCount);

void SetMaskTest()
{
    printf("set mask change\n");

    uint firstmask = sigprocmask((1 << 2) | (1 << 3));
    if (firstmask != 0)
    {
        printf("initial mask is'nt 0, failed\n");
        return;
    }
    uint invalidmask = sigprocmask((1 << 9));
    if (invalidmask != ((1 << 2) | (1 << 3)))
    {
        printf("failed to block masking SIGKILL, failed\n");
        return;
    }
    invalidmask = sigprocmask((1 << 17));
    if (invalidmask != ((1 << 2) | (1 << 3)))
    {
        printf("failed to block masking SIGSTOP, failed\n");
        return;
    }

    uint secMask = sigprocmask((1 << 2) | (1 << 3) | (1 << 4));
    if (secMask != ((1 << 2) | (1 << 3)))
    {
        printf("Mask wasn't changed, failed\n");
        return;
    }

    if (fork() == 0)
    {
        secMask = sigprocmask((1 << 2) | (1 << 3));
        if (secMask != ((1 << 2) | (1 << 3) | (1 << 4)))
        {
            printf("Child didn't inherit father's signal mask. Test failed\n");
        }
    }
    wait((int *)0);
    printf("passed set mask test!\n");
    sigprocmask(0);
}

void sigactionTest()
{
    printf("Starting sigaction test\n");
    struct sigaction sig1;
    struct sigaction sig2;
    sig1.sa_handler = &dummyFunc1;
    sig1.sigmask = 0;
    sig2.sa_handler = 0;
    sig2.sigmask = 0;

    sigaction(2, &sig1, &sig2);
    if (sig2.sa_handler != (void *)0)
    {
        printf("Original wasn't 0. failed\n");
        return;
    }
    sig1.sa_handler = &dummyFunc2;
    sigaction(2, &sig1, &sig2);

    if (sig2.sa_handler != &dummyFunc1)
    {
        printf("Handler wasn't changed to custom handler, test failed\n");
        return;
    }

    sig1.sa_handler = (void *)1;
    sig1.sigmask = 5;
    sigaction(2, &sig1, &sig2);

    if (fork() == 0)
    {
        if (sig2.sa_handler != &dummyFunc2)
        {
            printf("Signal handlers changed after fork. test failed\n");
            return;
        }
    }

    wait((int *)0);
    return;
}

void userHandlerTest()
{
    count = 0;
    int numOfSigs = 32;
    struct sigaction sig1;
    struct sigaction sig2;

    //printf("%p\n", &incCount);
    sig1.sa_handler = &incCount;
    sig1.sigmask = 0;
    for (int i = 0; i < numOfSigs; ++i)
    {
        
        sigaction(i, &sig1, &sig2);
    }
    // sigaction(31, &sig1, &sig2);
    // sigaction(31, &sig1, &sig2);
    // printf("%p\n", sig2.sa_handler);
    int pid = getpid();
    for (int i = 0; i < numOfSigs; ++i)
    {
        //printf("my pid is %d\n",    getpid());
        int ret;
        if ((ret = fork())==0)
        {
            //printf("my pid inside is %d\n",    getpid());
            //printf("my tid inside is %d\n",kthread_id());
            //printf("ret is %d",ret);
            int signum = i;
            if (signum != 9 && signum!=17)
            {
                kill(pid, signum);
            }
            printf("before exit %d my pid is %d\n",i,getpid());

            exit(0);

        }
        //printf("ret is %d",ret);
    }
    int counter = 0;
    for (int i = 0; i < numOfSigs; ++i)
    {
        //printf("waiting for child\n");

        wait((int *)0);
        counter++;
        //printf("waited for %d\n",counter);
    }
    while (1)
    {
        if (count == 0)
        {
            printf("All signals received!\n");
            break;
        }
    }

    
    printf("user handler test passed\n");
    sig1.sa_handler = 0;
    for (int i = 0; i < numOfSigs; ++i)
    {
        sigaction(i, &sig1, 0);
    }
}

void signalsmaskstest()
{
    printf("Starting signals masks test!\n");
    struct sigaction sig1;
    struct sigaction sig2;
    int ret;
    sig1.sa_handler = &dummyFunc1;
    if ((ret = sigaction(9, &sig1, &sig2)) != -1)
    {
        printf("change kill default handler, test failed\n");
        return;
    }
    sig1.sigmask = 1 << 9;
    if ((ret = sigaction(0, &sig1, &sig2)) != -1)
    {
        printf("setted a mask handler which blocks SIGKILL. test failed\n");
        return;
    }

    //we will define two new signals: 5 with sigstop and 20 with sigcont. 19 will be in the mask, therewise wont cont the stop. 20 will make cont.
    sig1.sigmask = ((1 << 19));
    sig1.sa_handler = (void *)17;

    if ((ret = sigaction(5, &sig1, &sig2)) == -1)
    {
        printf("sigaction failed test failed\n");
        return;
    }
    sig1.sa_handler = (void *)19;
    sigaction(20, &sig1, &sig2);
    int child = fork();
    if (child == 0)
    {
        while (1)
        {
            printf("r");
        }
    }
    sleep(1);
    kill(child, 5);
    sleep(1);
    kill(child, 19);
    printf("should not resume running\n");

    sleep(1);
    kill(child, 20); // remove this will cause infinity loop. the father will not continue
    printf("should resume running\n");
    sleep(1);

    kill(child,9);
    wait((int *)0);
    printf("passed signal mask test!\n");
    sig1.sa_handler = (void *)0;
    for (int i = 0; i < 32; i++)
    {
        sigaction(i, &sig1, 0);
    }
}

// void communicationTest(){
//   printf(2,"starting communication test\n");
//   signal(SIG_FLAG1,&setFlag1);
//   signal(SIG_FLAG2,&setFlag2);
//   int father=getpid();
//   int child=fork();
//   if(!child){
//     while(1){
//       if(flag1){
//         flag1=0;
//         printf(2,"received 10 signal. sending 20 signal to father.\n");
//         kill(father,SIG_FLAG2);
//         printf(2,"exiting\n");
//         exit();
//       }
//     }
//   }
//   printf(2,"sending 10 signal to child, waiting for response\n");
//   kill(child,SIG_FLAG1);
//   while(1){
//     if(flag2){
//       printf(2,"received signal from child\n");
//       flag2=0;
//       wait();
//       break;
//     }
//   }
//   printf(2,"communication test passed\n");
// }

// void multipleSignalsTest(){
//   count=0;
//   signal(2,&setFlag1);
//   for(int i=3;i<9;++i){signal(i,&incCount);}
//   int child;
//   if((child=fork())==0){
//     while(1){
//       if(flag1){
//         printf(2,"I'm printing number %d\n",count);
//       }
//       if(count==6){
//         printf(2,"Count is %d\n",count);
//         count=0;
//         flag1=0;
//         exit();
//       }
//     }
//   }
//   printf(2,"Sending signal to child\n");
//   kill(child,2);
//   printf(2,"Sending multiple signals to child\n");
//   for(int i=3;i<9;++i){kill(child,i);}
//   for(int i=0;i<6;++i){wait();}
//   printf(2,"Multiple signals test passed\n");
// }

void stopContTest1()
{

    int child1;
    if ((child1 = fork()) == 0)
    {
        while (1)
        {
            printf("Running!!!\n");
        }
    }
    sleep(1);

    printf("about to stop child:\n");
    kill(child1, 17);
    printf("sleep for 10 seconds and then continue child:\n");
    sleep(10);
    printf("sleeped, now wake up child.\n");
    kill(child1, 19);
    printf("sleep 1 second and then kill child\n");
    sleep(1);
    kill(child1, 9);

    wait((void *)0);
    printf("stop cont test 1 passed\n");
}

void stopContTest2()
{

    //stop child and then continue it's running by assigntwo bits with  the defualt condhandler and stophandler.
    struct sigaction sig1;
    struct sigaction sig2;
    sig1.sa_handler = (void *)19;
    sig2.sa_handler = (void *)17;
    sigaction(5, &sig1, 0);
    sigaction(6, &sig2, 0);
    int child1;
    if ((child1 = fork()) == 0)
    {
        while (1)
        {
            printf("Running!!!\n");
        }
    }
    sleep(1);

    printf("about to stop child:\n");
    kill(child1, 6);
    printf("sleep for 10 seconds and then continue child:\n");
    sleep(10);
    printf("sleeped, now wake up child.\n");
    kill(child1, 5);
    printf("sleep 1 second and then kill child\n");
    sleep(1);
    kill(child1, 9);

    wait((void *)0);
    printf("stop cont test2 passed\n");
    sig1.sa_handler = (void *)0;
    for (int i = 0; i < 32; i++)
    {
        sigaction(i, &sig1, 0);
    }
}

void stopKillTest()
{

    //stop child and then kill him while being stopped

    int child1;
    if ((child1 = fork()) == 0)
    {
        while (1)
        {
            printf("Running!!!\n");
        }
    }
    sleep(1);

    printf("about to stop child:\n");
    kill(child1, 17);
    printf("sleep for 10 seconds and then kill child:\n");
    sleep(10);
    printf("sleeped, now kill child.\n");
    kill(child1, 9);

    wait((void *)0);
    printf("stop kill test passed\n");
}

// void usrKillTest() {
//   int i, j, pid, maxNumOfDigits = 5, numOfSigs = 15;
//   int pids[numOfSigs];
//   char argStrings[numOfSigs*2][maxNumOfDigits];
//   char* argv[numOfSigs*2+2];
//   for(i=0;i<numOfSigs;++i){argv[2*i+1]=argStrings[i];}
//   for (i=0; i<numOfSigs; i++) {
//     if ((pid = fork()) != 0) {  //father
//       pids[i] = pid;
//     }
//     else break; //son
//   }
//   if (!pid) {
//     while(1);
//   }

//   for (i=0; i<numOfSigs; i++) {itoa(pids[i], argv[2*i+1]);}
//   argv[0] = "kill";
//   j=0;
//   for (i=0, j=0; i<numOfSigs; i++, j=j+2) {
//     argv[j+2] = "9";
//   }
//   argv[j+1] = 0;
//   j=0;
//   if(!fork()){
//     if(exec(argv[0],argv)<0){
//         printf(2,"couldn't exec kill, test failed\n");
//         exit();
//     }
//   }
//   wait();
//   for(int i=0;i<numOfSigs;i++){
//     printf(2,"another one bites the dust\n");
//     wait();
//   }
//   printf(2,"User Kill Test Passed\n");
// }

// void sendSignalUsingKillProgram(){
//   printf(2,"starting signal test using kill prog\n");
//   signal(SIG_FLAG1,&setFlag1);
//   signal(SIG_FLAG2,&setFlag2);
//   int pid=getpid();

//   char pidStr[20];
//   char signumStr[3];
//   char* argv[] = {"kill",pidStr,signumStr,0};

//   itoa(pid,pidStr);
//   int child = fork();
//   if(child==0){
//     while(1){
//       if(flag1){
//         flag1=0;
//         itoa(SIG_FLAG2,signumStr);
//         if(fork()==0){
//           if(exec(argv[0],argv)<0){
//             printf(2,"couldn't exec. test failed\n");
//             exit();
//           }
//         }
//         exit();
//       }
//     }
//   }
//   itoa(child,pidStr);
//   itoa(SIG_FLAG1,signumStr);
//   if(fork()==0){
//     if(exec(argv[0],argv)<0){
//       printf(2,"couldn't exec. test failed\n");
//       exit();
//     }
//   }
//   while(1){
//     if(flag2){
//       flag2=0;
//       wait();
//       break;
//     }
//   }
//   printf(2,"signal test using kill prog passed\n");
// }

int main(int argc, char **argv)
{
    if (argc > 1)
    {
        if (strcmp(argv[1], "sigmask") == 0)
        {
            uint secMask = sigprocmask(0);
            if (secMask != 28)
            {
                printf("Mask was changed after exec, test failed.\n");
                exit(1);
            }
            printf("Mask change test passed\n");
            exit(1);
        }
    }
    //     if(strcmp(argv[1],"sighandlers")==0){
    //       if(signal(3,(void*)SIG_IGN)!=(void*)SIG_IGN){
    //         printf(2,"SIG_IGN wasn't kept after exec. test failed\n");
    //         exit();
    //       }
    //       if(signal(4,&setFlag1)==&setFlag1){
    //         printf(2,"Custom signal handler wasn't removed after exec. test failed\n");
    //         exit();
    //       }
    //       printf(2,"Handler change test passed\n");
    //       exit();
    //     }
    //     printf(2,"Unknown argument\n");
    //     exit();
    //   }
    //   usrKillTest();
    //   sendSignalUsingKillProgram();
    //   killTest();

    //thread_test1();
    //thread_test2();
    userHandlerTest();

    // SetMaskTest();
    // stopContTest1();
    // stopContTest2();
    // stopKillTest();
    // signalsmaskstest();






    //   communicationTest();

    //handlerChange();
    
    //   multipleSignalsTest();
    printf("hereeeeeeeeeeeeeeeeeee");
    exit(0);
}

void dummyFunc1(int signum)
{

    return;
}
void dummyFunc2(int signum)
{

    return;
}



void incCount(int signum)
{
    //printf("in incount\n");
    count++;
    printf("%d\n",count);
}

void incCount2(int signum)
{
    //printf("in incount1\n");
    count2++;
    kthread_exit(0);
}

void incCount3(int signum)
{
    //printf("in incount1\n");
    count3++;
    kthread_exit(0);
}

// void itoa(int num,char* str){
//
// }

void itoa(int n, char s[])
{
    int i, sign;

    if ((sign = n) < 0) /* record sign */
        n = -n;         /* make n positive */
    i = 0;
    do
    {                          /* generate digits in reverse order */
        s[i++] = n % 10 + '0'; /* get next digit */
    } while ((n /= 10) > 0);   /* delete it */
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    reverse(s);
}

void reverse(char s[])
{
    int i, j;
    char c;
    char *curr = s;
    int stringLength = 0;
    while (*(curr++) != 0)
        stringLength++;

    for (i = 0, j = stringLength - 1; i < j; i++, j--)
    {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}