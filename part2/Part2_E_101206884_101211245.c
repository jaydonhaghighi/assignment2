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

// shared memory structure to store student database and TA progress
typedef struct {
    int database[NUM_STUDENTS];
    int ta_progress[NUM_TAS];
} SharedMemory;

sem_t *semaphores[NUM_TAS];
sem_t *print_mutex;

/**
 * function executed by each TA process.
 * accessing a shared database, marking students, and handling concurrency.
 */
void ta_task(int id, SharedMemory *shared_mem) {
    int rounds_completed = 0;

    while (rounds_completed < MARKING_ROUNDS) {
        int next_student;
        int first_sem = id;
        int second_sem = (id + 1) % NUM_TAS;

        if (first_sem > second_sem) {
            int temp = first_sem;
            first_sem = second_sem;
            second_sem = temp;
        }

        sem_wait(semaphores[first_sem]);
        sem_wait(semaphores[second_sem]);

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

        sleep(rand() % 4 + 1);

        sem_post(semaphores[first_sem]);
        sem_post(semaphores[second_sem]);

        sem_wait(print_mutex);
        printf("TA %d is marking student: %04d\n", id + 1, next_student);
        sem_post(print_mutex);

        sleep(rand() % 10 + 1);

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
 * initializes shared memory, semaphores, and forks processes for each TA.
 * cleans up resources after processes completed execution.
 */
int main() {
    srand(time(NULL));

    SharedMemory *shared_mem = mmap(NULL, sizeof(SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (shared_mem == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < NUM_STUDENTS - 1; i++) {
        shared_mem->database[i] = 1000 + i;
    }
    shared_mem->database[NUM_STUDENTS - 1] = 9999;

    for (int i = 0; i < NUM_TAS; i++) {
        shared_mem->ta_progress[i] = 0;
    }

    char sem_name[20];

    for (int i = 0; i < NUM_TAS; i++) {
        snprintf(sem_name, sizeof(sem_name), "/sem_ta_%d", i);
        sem_unlink(sem_name);
    }
    sem_unlink("/sem_print");

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

    for (int i = 0; i < NUM_TAS; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            ta_task(i, shared_mem);
        } else if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < NUM_TAS; i++) {
        wait(NULL);
    }

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