# Operating-Systems Assignment

# Producer-Consumer Problem using POSIX Semaphores and Shared Memory

This repository demonstrates a Producer-Consumer solution implemented in C. It leverages POSIX shared memory, semaphores, and threads to synchronize access between a producer that generates items and a consumer that picks them up. The shared "table" is implemented with a fixed size of two items, ensuring that the producer waits if the table is full and the consumer waits if the table is empty.

## Project Structure

- **producer.c**: Contains the code for the producer. It produces items, places them in shared memory, and uses semaphores to signal item availability.
- **consumer.c**: Contains the code for the consumer. It retrieves items from the shared memory and uses semaphores to indicate available space.
- **README.md**: This file with detailed instructions and examples.

## Prerequisites

- A Unix-like system (Linux, macOS).
- GCC (GNU Compiler Collection) with pthreads, POSIX shared memory, and semaphore support.
- Basic familiarity with command-line operations.

## Compilation

Open your terminal in the repository directory and run the following commands:

```bash
gcc producer.c -pthread -lrt -o producer
gcc consumer.c -pthread -lrt -o consumer
```

## Execution

Run the producer and consumer concurrently. For example, you can start them in the background using:

```bash
./producer & ./consumer &
```

## Termination

Since both programs run in infinite loops, you have two options to stop them:

Manual Termination:
You can simply kill the processes from the shell. For instance:

```bash
killall producer
killall consumer
```

Or, identify the process IDs using ps and terminate them with:

```bash
kill <PID>
```

- **Graceful Shutdown with Signal Handling**:
Enhance the code to catch termination signals (e.g., SIGINT or SIGTERM). When a signal is received, the program can exit the loop, perform cleanup (unmapping shared memory, closing semaphores), and then terminate gracefully.
Below is a sample modification for the producer:

```c
#include <signal.h>
#include <stdbool.h>
// Declare a global flag for termination
volatile sig_atomic_t running = 1;

// Signal handler to update the flag
void handle_sigint(int sig) {
    running = 0;
}

void *producer_thread(void *arg) {
    int item = 0;
    while (running) {
        item++;
        printf("Producing item: %d\n", item);

        sem_wait(empty_sem);
        sem_wait(mutex_sem);

        shared_data->buffer[shared_data->in] = item;
        printf("Placed item %d at index %d\n", item, shared_data->in);
        shared_data->in = (shared_data->in + 1) % TABLE_SIZE;

        sem_post(mutex_sem);
        sem_post(full_sem);

        sleep(1);
    }
    pthread_exit(NULL);
}

int main() {
    // Register the signal handler
    signal(SIGINT, handle_sigint);

    // [Setup shared memory and semaphores as before]

    pthread_t tid;
    if (pthread_create(&tid, NULL, producer_thread, NULL) != 0) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    pthread_join(tid, NULL);

    // Perform cleanup: unmap shared memory, close semaphores, etc.
    munmap(shared_data, sizeof(shared_data_t));
    // Also close semaphores and shared memory file descriptor here

    return 0;
}
```

Make sure to implement similar changes in your consumer code.

## Example Output
While running, you may see output similar to the following:

-**Producer Terminal**:

Producing item: 1
Placed item 1 at index 0
Producing item: 2
Placed item 2 at index 1
Producing item: 3
Placed item 3 at index 0
...

-**Consumer Terminal**:

Consumed item 1 from index 0
Consumed item 2 from index 1
Consumed item 3 from index 0
...

## Code Overview
Shared Memory
Both programs use POSIX shared memory (shm_open) with the name /shm_table to create a shared structure that includes:

- An array buffer for items.

- Two indices (in for insertion and out for removal).

- Semaphores
Three semaphores are used to manage synchronization:

empty_sem: Initialized to the table capacity (2) so that the producer waits when the table is full.

full_sem: Initialized to 0 so that the consumer waits when there are no items.

mutex_sem: A binary semaphore (initialized to 1) that ensures mutual exclusion when accessing the shared table.

- Threads
Each program creates a dedicated thread:

The producer thread continuously produces items and places them into the shared table.

The consumer thread continuously retrieves items from the table.

The main thread in each program uses pthread_join to wait for the spawned thread.
