#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <pthread.h>
#include <time.h>

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

void *producer_thread(void *arg) {
    int item = 0;
    while (1) {
        // Produce an item (for demonstration, we simply increment a counter)
        item++;
        printf("Producing item: %d\n", item);

        // Wait for an empty slot and acquire mutual exclusion
        sem_wait(empty_sem);
        sem_wait(mutex_sem);

        // Place the item into the table
        shared_data->buffer[shared_data->in] = item;
        printf("Placed item %d at index %d\n", item, shared_data->in);
        shared_data->in = (shared_data->in + 1) % TABLE_SIZE;

        // Release mutex and signal that a new item is available
        sem_post(mutex_sem);
        sem_post(full_sem);

        // Simulate production time
        sleep(1);
    }
    return NULL;
}

int main() {
    // Create or open shared memory
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    // Set the size of the shared memory
    if (ftruncate(shm_fd, sizeof(shared_data_t)) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    // Map the shared memory object into our address space
    shared_data = mmap(NULL, sizeof(shared_data_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // Initialize the shared table indices (assuming producer initializes first)
    shared_data->in = 0;
    shared_data->out = 0;

    // Open (or create) semaphores
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

    // Create a thread for producing items
    pthread_t tid;
    if (pthread_create(&tid, NULL, producer_thread, NULL) != 0) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    // Wait for the producer thread (runs indefinitely)
    pthread_join(tid, NULL);

    // Cleanup code (unreachable in this infinite loop)
    munmap(shared_data, sizeof(shared_data_t));
    close(shm_fd);
    sem_close(empty_sem);
    sem_close(full_sem);
    sem_close(mutex_sem);

    return 0;
}
