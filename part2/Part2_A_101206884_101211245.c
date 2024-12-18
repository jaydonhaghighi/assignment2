#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <time.h>

#define NUM_TAS 5
#define NUM_STUDENTS 20
#define NUM_CYCLES 3
#define DATABASE_FILE "database.txt"

// generates a random delay within the given range
void random_delay(int min, int max) {
    sleep(min + rand() % (max - min + 1));
}

// initializes the student database with ids
void initialize_database() {
    FILE *db = fopen(DATABASE_FILE, "w");
    if (!db) {
        perror("error creating database file");
        exit(EXIT_FAILURE);
    }
    for (int i = 1; i <= NUM_STUDENTS; i++) {
        fprintf(db, "%04d\n", i < NUM_STUDENTS ? i : 9999);
    }
    fclose(db);
}

// handles the ta process for marking students
void ta_process(int ta_id, sem_t *sem, int *current_index) {
    char ta_file[10];
    snprintf(ta_file, sizeof(ta_file), "TA%d.txt", ta_id);
    FILE *log = fopen(ta_file, "w");
    if (!log) {
        perror("error creating ta file");
        exit(EXIT_FAILURE);
    }

    srand(time(NULL) + ta_id);
    int local_index = 0;
    for (int cycle = 0; cycle < NUM_CYCLES; cycle++) {
        while (1) {
            sem_wait(sem);
            FILE *db = fopen(DATABASE_FILE, "r");
            if (!db) {
                perror("error opening database file");
                exit(EXIT_FAILURE);
            }
            
            fseek(db, *current_index * 5, SEEK_SET);
            fscanf(db, "%d", &local_index);
            if (local_index == 9999) {
                *current_index = 0;
                fclose(db);
                sem_post(sem);
                break;
            }
            *current_index += 1;
            fclose(db);
            sem_post(sem);

            random_delay(1, 4);
            int mark = rand() % 11;
            fprintf(log, "student: %04d, mark: %d\n", local_index, mark);
            fflush(log);
            printf("ta%d: marked student %04d with %d\n", ta_id, local_index, mark);
            
            random_delay(1, 10);
        }
    }
    fclose(log);
    printf("ta%d: finished marking\n", ta_id);
}

// initialize database, create ta processes, and handle cleanup
int main() {
    initialize_database();

    int *current_index = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE,
                              MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *current_index = 0;
    sem_t *sem = sem_open("/ta_semaphore", O_CREAT | O_EXCL, 0644, 1);
    if (sem == SEM_FAILED) {
        perror("error creating semaphore");
        exit(EXIT_FAILURE);
    }

    pid_t pids[NUM_TAS];
    for (int i = 0; i < NUM_TAS; i++) {
        pids[i] = fork();
        if (pids[i] == 0) {
            ta_process(i + 1, sem, current_index);
            exit(EXIT_SUCCESS);
        }
    }

    for (int i = 0; i < NUM_TAS; i++) {
        waitpid(pids[i], NULL, 0);
    }

    sem_close(sem);
    sem_unlink("/ta_semaphore");
    munmap(current_index, sizeof(int));

    printf("all tas finished marking. program complete.\n");
    return 0;
}