#include "kernel/types.h"
#include "kernel/fs.h"

#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"


int count = 0;


void dummyFunc1(int);
void dummyFunc2(int);

void incCount(int);
void itoa(int, char *);
void usrKillTest(void);
void reverse(char[]);

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
        int child = fork();
        if (child == 0)
        {
            int signum = i;
            if (signum != 9 && signum!=17)
            {
                kill(pid, signum);
                //printf("exited %d\n",i);
            }
            exit(0);
        }
    }
    for (int i = 0; i < numOfSigs; ++i)
    {
        wait((int *)0);
    }
    while (1)
    {
        if (count == numOfSigs - 2)
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


    userHandlerTest();

    // SetMaskTest();
    // stopContTest1();
    // stopContTest2();
    // stopKillTest();
    // signalsmaskstest();






    //   communicationTest();

    //handlerChange();
    
    //   multipleSignalsTest();
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
    printf("in incount\n");
    count++;
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
