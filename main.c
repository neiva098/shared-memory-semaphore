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

/*
    to compile && run: gcc -Wall main.c -lrt  -o main -lpthread && ./main
*/

void closeSharedMemory() // fecha a memória compartilhada
{
    if (shm_unlink(SHARED_MEMORY_NAME) == -1)
    {
        printf("Error removing %s\n", SHARED_MEMORY_NAME);
        exit(-1);
    }
}

void closeSemaphore(sem_t *semaphore) // fechamento do semáforo
{
    sem_close(semaphore);
    sem_unlink(SEMAPHORE_NAME);
}

void waitSums() // espera que todos os processos filhos tenham encerrado
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

void childWork(int *sharedMem, sem_t *semaphore) // realiza o processo de soma coordenado pelo semáforo
{
    for (int i = 0; i < NUM_STEPS; i++)
    {
        sem_wait(semaphore);         // solicita acesso a area critica
        *sharedMem = *sharedMem + 1; // critical section
        sem_post(semaphore);         // libera area critica
    }
}

void parentWork(int *sharedMem) // espera os filhos processarem, imprime resultado e fecha a memoria compartilhada
{
    waitSums(); // aguarda encerramento do processamento

    int expected = NUM_PROCESS * NUM_STEPS;

    printf("Sum should be %d and is %d\n", expected, *sharedMem);

    printf("Expected and result are %s equal\n", expected == *sharedMem ? "" : "not");

    closeSharedMemory(); // fecha memoria compartilhada
}

pid_t createProcesses(sem_t *semaphore) // cria novos processos
{
    pid_t pid;

    for (int i = 0; i < NUM_PROCESS; i++)
    {
        pid = fork();

        if (pid == 0) // se for filho saia do loop
            break;
    }

    return pid;
}

int *createSharedMemory() // cria memoria compartilhada que armazenará a soma
{
    const int SIZE = sizeof(int);

    const char *name = SHARED_MEMORY_NAME;

    int shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);

    ftruncate(shm_fd, SIZE);

    int *ptr = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    *ptr = 0; // inicializa soma com 0

    return ptr;
}

int main(int argc, char *argv[], char *envp[])
{
    int *sharedMem = createSharedMemory(); // cria memoria

    //https://www.man7.org/linux/man-pages/man3/shm_open.3.html

    sem_t *semaphore = sem_open(SEMAPHORE_NAME, O_CREAT, 0644, 1); // cria o semaforo em uma memória compartilhada

    pid_t pid = createProcesses(semaphore); // cria processos

    if (pid == 0)
        childWork(sharedMem, semaphore); // soma se for filho

    else
        parentWork(sharedMem); // resultado se for pai
}