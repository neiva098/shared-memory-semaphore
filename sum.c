#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <semaphore.h>

#define SHARED_MEMORY_NAME "/sm_semaphore"
#define NUM_STEPS 100000
#define SEMAPHORE_NAME "/psem"

sem_t *semaphore;

sem_t *openSemaphore()
{
    return sem_open(SEMAPHORE_NAME, O_RDWR);
}

void *openSharedMemory()
{
    const char *name = SHARED_MEMORY_NAME;
    const int SIZE = 4096;

    int shm_fd;

    shm_fd = shm_open(name, O_RDWR, 0666);
    if (shm_fd == -1)
    {
        printf("shared memory failed\n");
        exit(-1);
    }

    return mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
}

void sum(int *sum)
{
    for (int i = 0; i < NUM_STEPS; i++)
    {
        sem_wait(semaphore);
        *sum = *sum + 1; // critical section
        printf("ola");
        sem_post(semaphore);
    }
}

int main()
{
    int *ptr = openSharedMemory();

    semaphore = sem_open(SEMAPHORE_NAME, O_CREAT, 0644, 0);

    printf("%s", semaphore);

    sum(ptr);

    exit(0);
}