#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>

#define NUM_TAS 5
#define NUM_STUDENTS 20
#define MARKING_ROUNDS 3

// Shared memory structure to store the database and TA progress
typedef struct {
    int database[NUM_STUDENTS];
    int ta_progress[NUM_TAS];
} SharedMemory;

sem_t *semaphores[NUM_TAS];
sem_t *print_mutex;

/**
 * Function executed by each TA process.
 * Implements the marking task while ensuring mutual exclusion
 * using semaphores for shared resource access.
 */
void ta_task(int id, SharedMemory *shared_mem) {
    int rounds_completed = 0;

    while (rounds_completed < MARKING_ROUNDS) {
        int next_student;
        int acquired = 0;

        // attempts to acquire necessary semaphores in a retry loop
        while (!acquired) {
            sem_wait(semaphores[id]);
            if (sem_trywait(semaphores[(id + 1) % NUM_TAS]) == 0) {
                acquired = 1;
            } else {
                sem_post(semaphores[id]);
                sleep(1); // Avoid busy waiting
            }
        }

        // accesses the database to fetch the next student
        next_student = shared_mem->database[shared_mem->ta_progress[id]];
        if (next_student == 9999) {
            shared_mem->ta_progress[id] = 0;
            next_student = shared_mem->database[shared_mem->ta_progress[id]];
            rounds_completed++;
        }
        shared_mem->ta_progress[id]++;

        sem_wait(print_mutex);
        printf("TA %d accessing database\n", id + 1);
        sem_post(print_mutex);

        // delay for accessing the database
        sleep(rand() % 4 + 1);

        // eelease semaphores after access
        sem_post(semaphores[id]);
        sem_post(semaphores[(id + 1) % NUM_TAS]);

        sem_wait(print_mutex);
        printf("TA %d is marking student: %04d\n", id + 1, next_student);
        sem_post(print_mutex);

        // delay for marking
        sleep(rand() % 10 + 1);

        // random mark between 0 and 10
        int mark = rand() % 11;
        sem_wait(print_mutex);
        printf("TA %d marked Student %04d: Mark %d\n", id + 1, next_student, mark);
        sem_post(print_mutex);
    }

    sem_wait(print_mutex);
    printf("TA %d has completed marking.\n", id + 1);
    sem_post(print_mutex);

    exit(0);
}

/**
 * Main function to initialize shared memory, semaphores,
 * and processes for TAs, and handle cleanup after completion.
 */
int main() {
    srand(time(NULL));

    // Create shared memory
    SharedMemory *shared_mem = mmap(NULL, sizeof(SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (shared_mem == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // initialize the database with student numbers
    for (int i = 0; i < NUM_STUDENTS - 1; i++) {
        shared_mem->database[i] = 1000 + i;
    }
    shared_mem->database[NUM_STUDENTS - 1] = 9999;

    // initialize TA progress
    for (int i = 0; i < NUM_TAS; i++) {
        shared_mem->ta_progress[i] = 0;
    }

    // unlink existing semaphores
    char sem_name[20];
    for (int i = 0; i < NUM_TAS; i++) {
        snprintf(sem_name, sizeof(sem_name), "/sem_ta_%d", i);
        sem_unlink(sem_name);
    }
    sem_unlink("/sem_print");

    // Create semaphores
    for (int i = 0; i < NUM_TAS; i++) {
        snprintf(sem_name, sizeof(sem_name), "/sem_ta_%d", i);
        semaphores[i] = sem_open(sem_name, O_CREAT | O_EXCL, 0600, 1);
        if (semaphores[i] == SEM_FAILED) {
            perror("sem_open");
            exit(EXIT_FAILURE);
        }
    }

    print_mutex = sem_open("/sem_print", O_CREAT | O_EXCL, 0600, 1);
    if (print_mutex == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    // fork processes for each TA
    for (int i = 0; i < NUM_TAS; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            ta_task(i, shared_mem);
        } else if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
    }

    // wait for all child processes to complete
    for (int i = 0; i < NUM_TAS; i++) {
        wait(NULL);
    }

    // cleanup
    for (int i = 0; i < NUM_TAS; i++) {
        snprintf(sem_name, sizeof(sem_name), "/sem_ta_%d", i);
        sem_close(semaphores[i]);
        sem_unlink(sem_name);
    }

    sem_close(print_mutex);
    sem_unlink("/sem_print");
    munmap(shared_mem, sizeof(SharedMemory));
    printf("All TAs have finished marking.\n");
    return 0;
}
