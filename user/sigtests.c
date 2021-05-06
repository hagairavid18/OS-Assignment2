#include "kernel/types.h"
#include "kernel/fs.h"

#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"

#define NUM_OF_CHILDREN 20
#define SIG_FLAG1 10
#define SIG_FLAG2 20
#define SIGKILL 9
#define SIGSTOP 17
#define SIGCONT 19
#define SIG_IGN 1
#define NULL 0
#define SIG_DFL 0






int flag1 = 0;
int flag2 = 0;
int flag3 = 0;
int count = 0;
int count2 = 0;
int count3 = 0;

void fun(int);
void dummyFunc1(int);
void dummyFunc2(int);
void dummyFunc3(int);

void incCount3(int);
void incCount2(int);
void incCount(int);
void itoa(int, char *);
void usrKillTest(void);
void reverse(char[]);

/////////////TAL TESTS /////TODO: delete
void user_handler (int signum){
    printf(" hiii user_handler is working  1 !! \n");
    return;
}

void user_handler2 (int signum){
    printf(" hiii user_handler is working 2 !! \n");
    return;
}

void kill_while_freezed_test(void){
    int pid = fork();
    int status;
    if((pid > 0)) {         // father
        sleep(2);
        printf(" father is sending SIGSTOP to child\n");
        kill(pid, SIGSTOP);
        printf(" father is going to sleep\n");
        sleep(150);
        printf(" father is sending SIGKILL to child\n");
        kill(pid, SIGKILL);
        wait(&status);
        printf(" father finished\n");
    }else{
        printf("child is here\n");
        while(1){

        }

    }
}

void stop_cont_test(char *argv[]){
    int status;
    int pid = fork();
    if (pid > 0){           //father
        sleep(100);
        printf(" father is sending SIGSTOP to child\n");
        kill(pid, SIGSTOP);
        printf(" father is going to sleep\n");
        sleep(200);
        printf(" father is going to sleep\n");
        sleep(100);
        printf(" father is going to sleep\n");
        sleep(100);
        printf(" father is sending SIGCONT to child\n");
        kill(pid, SIGCONT);
        sleep(100);
        printf(" father is sending SIGKILL to child\n");
        kill(pid, SIGKILL);
        wait(&status);
        printf(" father finished\n");

    } else {
        exec("test_function", argv);
    }
}

void ignore_test(void){
    struct sigaction* action = (struct sigaction*) malloc (sizeof sigaction);
    action->sa_handler = (void*)SIG_IGN;
    action->sigmask = 0;
    int status;

    sigaction(15, action, NULL);
    int pid = fork();
    if (pid != 0 ){     // father
        printf("father send signal 15 which should be ignored\n");
        kill(pid, 15);
         printf("father going to sleep \n");
        sleep(20);
        printf("father sending SIGKILL to kill child\n");
        kill(pid, SIGKILL);
        wait(&status);
        printf("ignore succeed, ignore test passed\n");
    } else {        // child
        sleep(10);
        while(1){

        }
        printf("ignore test failed!\n");
    }
    free(action);
}

void user_handler_update_mask_test(void){
    int pid = fork();
    int status;
    struct sigaction* action;
    struct sigaction* action2;
    if (pid) {          // father
        sleep(3);           // make sure child woke first
        printf("father is sending signal 15  \n");
        kill(pid, 15);
        printf("father is going to sleep ... \n");
        sleep(20);
        wait(&status);
        printf("father finish \n");
    } else if (pid==0){            // child
        int pid2 = fork();
        int stat;
        if (pid2== 0){
            sleep(60);
            action2 = (struct sigaction*) malloc (sizeof sigaction);
            action2->sa_handler = user_handler2;
            action2->sigmask = 0;
            sigaction(13, action2, NULL);
            uint oldmask1 = sigprocmask(0x2000);
            printf("child 2 is going to sleep ... \n");
            sleep(60);
            printf("child 2 signal 13 still blocked ... \n");
            oldmask1 = sigprocmask(0);           // unblock signal 0x2000
            sleep(10);
            if (oldmask1 != 0x2000)
                printf(" sigprocmask new failed \n", oldmask1);
        }
        else{
            action = (struct sigaction*) malloc (sizeof sigaction);
            action->sa_handler = user_handler;
            action->sigmask = 0;
            sigaction(15, action, NULL);
          //  uint oldmask = sigprocmask(0x2000);
            uint oldmask = sigprocmask(0x8000);
         //   if (oldmask != 0x2000)                 // cuz oldmask changed
         //       printf("1 sigprocmask failed \n");
            printf("child is going to sleep ... \n");
            sleep(60);
            printf("CHILD is sending signal 13  \n");
            kill(pid2, 13);
            printf("child signal 15 still blocked ... \n");
            sleep(20);
            oldmask = sigprocmask(0);           // unblock signal 15
          //  printf(" before 2 sigprocmask failed %d\n",oldmask);
            sleep(10);
            if (oldmask != 0x8000)
                printf("2 sigprocmask failed \n");
            wait(&stat);
        }
          
       
    }
    //free(action);
    //free(action2);
    return;
}

void kernel_handler_update_mask_test(void){
    int pid = fork();
    int status;
    if (pid) {          // father
        sleep(3);           // make sure child woke first
         printf("father send signal 15 ... \n");
        kill(pid, 15);
        sleep(10);
        kill(pid, SIGCONT);
        printf("father is going to wait ... \n");
        wait(&status);
         printf("father finished ... \n");
    } else {            // child
        struct sigaction* action = (struct sigaction*) malloc (sizeof sigaction);
        action->sa_handler = (void*)SIGSTOP;
        action->sigmask = 0;
        sigaction(15, action, NULL);
        uint oldmask = sigprocmask(0x8000);
        if (oldmask != 0)                 // cuz oldmask changed
            printf("sigprocmask failed \n");
        printf("child is going to sleep ... \n");
        sleep(10);
        printf("child signal 15 still blocked ... \n");
        printf("child unblocked signal ... \n");
        oldmask = sigprocmask(0);           // unblock signal 15
        if (oldmask != 0x8000)
            printf("sigprocmask failed \n");
        free(action);
        exit(0);
    }
}


void
test_stop_cont(){
    int pid = fork();

    if(pid == 0){           // child 
        printf( "child %d going to loop \n", getpid());

        while(1){
          //  running
        }
    }
    else{       //  father
        sleep(40);
        printf( " father sending  SIGSTOP ... \n");
        kill(pid, SIGSTOP);                         // child should yeild
        sleep(40);
        printf( "father sending  SIGCONT\n");           // child continue
        kill(pid, SIGCONT);
        sleep(40);
        printf( "father sending  SIG_DFL\n");            // child going to die
        kill(pid, SIG_DFL);
        wait(&pid);
        printf("father finish waiting for child end exit \n");
        exit(0);
    }
}

void user_handler_update_mask_test2(void){
    int pid = fork();
    int status;
    if (pid) {          // father
        sleep(3);           // make sure child woke first
        kill(pid, 15);
        wait(&status);
    } else if (pid==0){            // child
        struct sigaction* action = (struct sigaction*) malloc (sizeof sigaction);
        action->sa_handler = user_handler;
        action->sigmask = 0;
        sigaction(15, action, NULL);
        uint oldmask = sigprocmask(0x2000);
        oldmask = sigprocmask(0x8000);
        if (oldmask != 0x2000)                 // cuz oldmask changed
            printf("1 sigprocmask failed \n");
        printf("child is going to sleep ... \n");
        sleep(20);
        printf("child signal 15 still blocked ... \n");
        oldmask = sigprocmask(0);           // unblock signal 15
        printf(" before 2 sigprocmask failed %d\n",oldmask);
        sleep(10);
        if (oldmask != 0x8000)
            printf("2 sigprocmask failed \n");
        free(action);
       return;
    }
}




/////TAL TESTS ENDDDDDDDDD  ///TODO: delete




void test_morehtreadsthantable(){
    char *stacks[20];
    int cid;
    for (int i = 0; i < sizeof(stacks)/sizeof(char *); i++)
    {
        stacks[i] = ((char *)malloc(4000 * sizeof(char)));
    }
    //int pid[5];

    if ((cid = fork()) == 0)
    {
        int childs1[sizeof(stacks)/sizeof(char *)];
        // int childs2[7];

        for (int j = 0; j < 20; j++)
        {
            int child1 = kthread_create(&incCount2, stacks[j]);
            printf("child id is %d\n",child1);
            childs1[j] = child1;
        }
        for (int j = 0; j < sizeof(stacks)/sizeof(char *); j++)
        {
            kthread_join(childs1[j],(int *)0);
        }
        

        printf("count1 is %d\n",count);
        // printf("count2  is %d\n",count2);
        kthread_exit(0);
        
    }
    

    wait(0);
    printf("test_morehtreadsthantable PASS\n");
}


void test_alteringcreatejointhreads(){
    char *stacks[20];
    int cid;
    for (int i = 0; i < 20; i++)
    {
        stacks[i] = ((char *)malloc(4000 * sizeof(char)));
    }
    //int pid[5];

    if ((cid = fork()) == 0)
    {
        int childs1[sizeof(stacks)/sizeof(char *)];
        // int childs2[7];

        for (int j = 0; j < 7; j++)
        {
            int child1 = kthread_create(&incCount2, stacks[j]);
            printf("child id is %d\n",child1);
            childs1[j] = child1;
        }
        printf("finished loop\n");

        printf("----- Altering -----\n");

        printf("Altering thread 1: ");
        kthread_join(childs1[1],(int *)0);
        stacks[1] = ((char *)malloc(4000 * sizeof(char)));
        int child1 = kthread_create(&incCount2, stacks[1]);        
        printf("child id is %d\n",child1);

        printf("Altering thread 5: ");
        kthread_join(childs1[5],(int *)0);
        stacks[5] = ((char *)malloc(4000 * sizeof(char)));
        child1 = kthread_create(&incCount2, stacks[5]);
        printf("child id is %d\n",child1);

        printf("Altering thread 7: ");
        kthread_join(childs1[7],(int *)0);
        stacks[7] = ((char *)malloc(4000 * sizeof(char)));
        child1 = kthread_create(&incCount2, stacks[7]);
        printf("child id is %d\n",child1);

        printf("----- Done -----\nJoin on all table\n");
        for (int j = 0; j < sizeof(stacks)/sizeof(char *); j++)
        {
            kthread_join(childs1[j],(int *)0);
        }
        

        printf("count2 is %d\n",count2);
        // printf("count2  is %d\n",count2);
        kthread_exit(0);
        
    }
    

    wait(0);
    printf("test_alteringcreatejointhreads PASS\n");
}



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
    //printf("add is %p\n",&incCount2);
    if ((cid = fork()) == 0)
    {
        //printf("forked1\n");
        for (int j = 0; j < 7; j++)
        {
            int child1 = kthread_create(&incCount2, stacks[j]);
            // if (child1 < 0)
            //     printf("error, child couldnt be created");
            childs1[j] = child1;
        }
        for (int j = 0; j < 7; j++)
        {
            //printf("1 join to %d\n",childs1[j]);
            kthread_join(childs1[j], (int *)0);
        }
        //printf("1 exit\n");
        kthread_exit(0);
    }

    if ((cid = fork()) == 0)
    {
                //printf("forked2\n");

        for (int j = 0; j < 7; j++)
        {
            int child2 = kthread_create(&incCount2, stacks[7 + j]);
            // if (child2 < 0)
            //     printf("error, child couldnt be created");
            childs2[j] = child2;
        }
        for (int j = 0; j < 7; j++)
        {
            //printf("2 join to %d\n",childs2[j]);
            kthread_join(childs2[j], (int *)0);
        }
                //printf("2 exit\n");

        kthread_exit(0);
    }

    wait((void *)0);
    printf("waited once\n");
    wait((void *)0);
    printf("passed first thread test!\n");
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
        //printf("child id is %d\n", child1);
        childs1[j] = child1;
    }
    for (int j = 0; j < 7; j++)
    {
        kthread_join(childs1[j], (int *)0);
    }
    for (int j = 0; j < 7; j++)
    {
        int child2 = kthread_create(&incCount3, stacks[7 + j]);
        //printf("child id is %d\n", child2);
        childs2[j] = child2;
    }
    for (int j = 0; j < 7; j++)
    {
        //printf("join for t: %d\n",childs2[j]);
        kthread_join(childs2[j], (int *)0);
    }

    printf("count2 is %d\n", count2);
    printf("count3  is %d\n", count3);
    //kthread_exit(0);

    printf("passed second thread test!\n");
}

void thread_test3()
{
    printf("start third thread test. should just finish normally\n");

    char *stacks[14];
    for (int i = 0; i < 7; i++)
    {
        stacks[i] = ((char *)malloc(4000 * sizeof(char)));
    }
    //int pid[5];
    //int childs[7];

    for (int j = 0; j < 7; j++)
    {
        kthread_create(&incCount2, stacks[j]);
        //printf("child id is %d\n", child);
        //childs[j] = child;
    }
    exit(0);

    printf("passed second thread test!\n");
}

void SetMaskTest()
{
    printf("starting setmask Test\n");

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

    printf("passed sigaction test!\n");
    sig1.sa_handler = 0;
    sig1.sigmask = 0;
    for (int i = 0; i < 32; ++i)
    {
        sigaction(i, &sig1, 0);
    }
    return;
}

void userHandlerTest()
{
    printf("starting userHandlerTest test!\n");

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
        if ((ret = fork()) == 0)
        {
            //printf("my pid inside is %d\n",    getpid());
            //printf("my tid inside is %d\n",kthread_id());
            //printf("ret is %d",ret);
            int signum = i;
            if (signum != 9 && signum != 17)
            {
                kill(pid, signum);
            }
            printf("before exit %d my pid is %d\n", i, getpid());

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

    kill(child, 9);
    wait((int *)0);
    printf("passed signal mask test!\n");
    sig1.sa_handler = (void *)0;
    for (int i = 0; i < 32; i++)
    {
        sigaction(i, &sig1, 0);
    }
}

void stopContTest1()
{
    printf("statring stop cont 1 test!\n");

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
    printf("statring stop cont 2 test!\n");
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
    printf("starting stopkill test!\n");
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
    printf("stop kill test passed!\n");
}

void KillFromKeyboard()
{
    printf("starting signal test using kill prog\n");
    struct sigaction sig1;
    sig1.sa_handler = &incCount;
    printf("incount adress is %p\n", &incCount);
    if (sigaction(10, &sig1, 0) < 0)
        printf("failed sigaction");

    sig1.sa_handler = &dummyFunc2;
    printf("dummyfunc3 adress is %p\n", &dummyFunc2);

    if (sigaction(11, &sig1, 0) < 0)
        printf("failed sigaction");
    int pid = getpid();

    char pidStr[20];
    char signumStr1[3];
    char signumStr2[3];
    char *argv[] = {"kill", pidStr, signumStr1, pidStr, signumStr2, 0};

    itoa(pid, pidStr);
    itoa(10, signumStr1);
    itoa(11, signumStr2);
    if (fork() == 0)
    {
        if (exec(argv[0], argv) < 0)
        {
            printf("exec failed\n");
            exit(0);
        }
    }
    while (1)
    {
        sleep(1);
        if (count && flag2)
        {
            count = 0;
            flag3 = 0;
            wait(0);
            break;
        }
    }
    printf("signal test using kill prog passed!!\n");
    sig1.sa_handler = 0;
    for (int i = 0; i < 32; ++i)
    {
        sigaction(i, &sig1, 0);
    }
}

int main(int argc, char **argv)
{

    test_alteringcreatejointhreads();
    ///TAL TESTS ///TODO delte
    //user_handler_update_mask_test2();
    //ser_handler_update_mask_test();
    
    //kernel_handler_update_mask_test();
    //test_stop_cont();
    //ignore_test();
    //char * arg[]={"test_function","test working",'\0'};
    //stop_cont_test(arg); //DIDNT pass, we dont have test function
    //kill_while_freezed_test();
    

    ///END TAL TESTS???TODO DELETE

    // thread_test1();
    // thread_test2();
    // thread_test3();
    //userHandlerTest(); // dosnt work with multiplate cpu.

    // SetMaskTest();
    // stopContTest1();
    // stopContTest2();
    // stopKillTest();
    // signalsmaskstest();
    // KillFromKeyboard();

    //   communicationTest();

    //   multipleSignalsTest();
    //printf("hereeeeeeeeeeeeeeeeeee");
    exit(0);
}
void fun(int signum)
{

    return;
}

void dummyFunc1(int signum)
{
    printf("in dummy fuc 1\n");
    flag1 = 1;
    return;
}
void dummyFunc2(int signum)
{
    printf("in dummy fuc 2\n");
    flag2 = 1;
    return;
}

void dummyFunc3(int signum)
{
    //printf("in dummy fuc 3\n");
    flag3 = 1;
    return;
}

void incCount(int signum)
{
    // printf("in incount()\n");
    count++;
    //printf("%d\n", count);
}

void incCount2(int signum)
{
   // printf("count2 is %d\n", count2);

    count2++;

    kthread_exit(0);
}

void incCount3(int signum)
{
   // printf("in incount3\n");
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