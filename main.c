#include <semaphore.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <time.h>
#include <stdio.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#define NUM_PROCESS 100
#define SHARED_MEMORY_NAME "/sm_semaphore"
#define SEMAPHORE_NAME "/pSem"
#define NUM_STEPS 100000

void callOneSum(char *argv[])
{
    pid_t pid = fork();

    if (pid == 0)
    {
        execve("./sum", argv, NULL);

        exit(0);
    }
}

void doSums(char *argv[])
{
    for (int i = 0; i < NUM_PROCESS; i++)
        callOneSum(argv);
}

int *createSharedMemory()
{
    const int SIZE = 4096;
    const char *name = SHARED_MEMORY_NAME;

    int shm_fd;

    shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);

    ftruncate(shm_fd, SIZE);

    int *ptr = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    *ptr = 0;

    return ptr;
}

void closeSharedMemory()
{
    if (shm_unlink(SHARED_MEMORY_NAME) == -1)
    {
        printf("Error removing %s\n", SHARED_MEMORY_NAME);
        exit(-1);
    }
}

void waitSums()
{
    pid_t pid;
    while ((pid = waitpid(-1, NULL, 0)))
    {
        if (errno == ECHILD)
        {
            break;
        }
    }
}

int main(int argc, char *argv[], char *envp[])
{
    int *sum = createSharedMemory();

    doSums(argv);

    waitSums();

    printf("Sum should be %d and is %d\n", NUM_PROCESS * NUM_STEPS, *sum);

    closeSharedMemory();

    sem_unlink(SEMAPHORE_NAME);
}