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
#define SHARED_MEMORY_NAME "/smsemaphore"
#define SEMAPHORE_NAME "mysemaphore"
#define NUM_STEPS 100000

void closeSharedMemory()
{
    if (shm_unlink(SHARED_MEMORY_NAME) == -1)
    {
        printf("Error removing %s\n", SHARED_MEMORY_NAME);
        exit(-1);
    }
}

void closeSemaphore(sem_t *semaphore)
{
    sem_close(semaphore);
    sem_unlink(SEMAPHORE_NAME);
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

void childWork(int *sharedMem, sem_t *semaphore)
{
    for (int i = 0; i < NUM_STEPS; i++)
    {
        sem_wait(semaphore);
        *sharedMem = *sharedMem + 1; // critical section
        sem_post(semaphore);
    }
}

void parentWork(int *sharedMem)
{
    waitSums();

    int expected = NUM_PROCESS * NUM_STEPS;

    printf("Sum should be %d and is %d\n", expected, *sharedMem);

    printf("Expected and result are %s equal\n", expected == *sharedMem ? "" : "not");

    closeSharedMemory();
}

pid_t createProcesses(sem_t *semaphore)
{
    pid_t pid;

    for (int i = 0; i < NUM_PROCESS; i++)
    {
        pid = fork();

        if (pid == 0)
            break;
    }

    return pid;
}

int *createSharedMemory()
{
    const int SIZE = sizeof(int);

    const char *name = SHARED_MEMORY_NAME;

    int shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);

    ftruncate(shm_fd, SIZE);

    int *ptr = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    *ptr = 0;

    return ptr;
}

int main(int argc, char *argv[], char *envp[])
{
    int *sharedMem = createSharedMemory();

    sem_t *semaphore = sem_open(SEMAPHORE_NAME, O_CREAT, 0644, 1);

    pid_t pid = createProcesses(semaphore);

    if (pid == 0)
        childWork(sharedMem, semaphore);

    else
        parentWork(sharedMem);
}