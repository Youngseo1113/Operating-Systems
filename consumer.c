#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <pthread.h>

#define TABLE_SIZE 2
#define SHM_NAME "/shm_table"
#define SEM_EMPTY "/empty_sem"
#define SEM_FULL  "/full_sem"
#define SEM_MUTEX "/mutex_sem"

typedef struct {
    int buffer[TABLE_SIZE];
    int in;
    int out;
} shared_data_t;

shared_data_t *shared_data;
sem_t *empty_sem;
sem_t *full_sem;
sem_t *mutex_sem;

void *consumer_thread(void *arg) {
    while (1) {
        // Wait for an available item and acquire mutual exclusion
        sem_wait(full_sem);
        sem_wait(mutex_sem);

        // Retrieve the item from the table
        int item = shared_data->buffer[shared_data->out];
        printf("Consumed item %d from index %d\n", item, shared_data->out);
        shared_data->out = (shared_data->out + 1) % TABLE_SIZE;

        // Release mutex and signal that an empty slot is available
        sem_post(mutex_sem);
        sem_post(empty_sem);

        // Simulate consumption time
        sleep(2);
    }
    return NULL;
}

int main() {
    // Open (or create) shared memory
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    // Set the size of the shared memory (if not already set)
    if (ftruncate(shm_fd, sizeof(shared_data_t)) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    // Map the shared memory object
    shared_data = mmap(NULL, sizeof(shared_data_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // Open semaphores
    empty_sem = sem_open(SEM_EMPTY, O_CREAT, 0666, TABLE_SIZE);
    if (empty_sem == SEM_FAILED) {
        perror("sem_open empty_sem");
        exit(EXIT_FAILURE);
    }

    full_sem = sem_open(SEM_FULL, O_CREAT, 0666, 0);
    if (full_sem == SEM_FAILED) {
        perror("sem_open full_sem");
        exit(EXIT_FAILURE);
    }

    mutex_sem = sem_open(SEM_MUTEX, O_CREAT, 0666, 1);
    if (mutex_sem == SEM_FAILED) {
        perror("sem_open mutex_sem");
        exit(EXIT_FAILURE);
    }

    // Create a thread for consuming items
    pthread_t tid;
    if (pthread_create(&tid, NULL, consumer_thread, NULL) != 0) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    // Wait for the consumer thread (runs indefinitely)
    pthread_join(tid, NULL);

    // Cleanup code (unreachable in this infinite loop)
    munmap(shared_data, sizeof(shared_data_t));
    close(shm_fd);
    sem_close(empty_sem);
    sem_close(full_sem);
    sem_close(mutex_sem);

    return 0;
}
