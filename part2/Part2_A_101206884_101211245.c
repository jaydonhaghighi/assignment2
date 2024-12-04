#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define NUM_TAS 5
#define NUM_STUDENTS 20
#define MARKING_ROUNDS 3

pthread_mutex_t mutexes[NUM_TAS];
int database[NUM_STUDENTS];
int ta_progress[NUM_TAS];

pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * Task executed by each TA thread.
 * TAs access the database, mark students, and write marks to a file.
 */
void *ta_task(void *arg) {
    int id = *((int *)arg);
    int rounds_completed = 0;
    FILE *output_file;
    char filename[20];
    snprintf(filename, sizeof(filename), "TA%d.txt", id + 1);
    output_file = fopen(filename, "w");

    while (rounds_completed < MARKING_ROUNDS) {
        int next_student;

        pthread_mutex_lock(&mutexes[id]);
        pthread_mutex_lock(&mutexes[(id + 1) % NUM_TAS]);

        next_student = database[ta_progress[id]];
        if (next_student == 9999) {
            ta_progress[id] = 0;
            next_student = database[ta_progress[id]];
            rounds_completed++;
        }
        ta_progress[id]++;

        pthread_mutex_lock(&print_mutex);
        printf("TA %d accessing database\n", id + 1);
        pthread_mutex_unlock(&print_mutex);

        sleep(rand() % 4 + 1);

        pthread_mutex_unlock(&mutexes[id]);
        pthread_mutex_unlock(&mutexes[(id + 1) % NUM_TAS]);

        pthread_mutex_lock(&print_mutex);
        printf("TA %d is marking student %04d\n", id + 1, next_student);
        pthread_mutex_unlock(&print_mutex);

        sleep(rand() % 10 + 1);

        int mark = rand() % 11;
        fprintf(output_file, "Student %04d: Mark %d\n", next_student, mark);
        fflush(output_file);
    }

    fclose(output_file);

    pthread_mutex_lock(&print_mutex);
    printf("TA %d has completed marking.\n", id + 1);
    pthread_mutex_unlock(&print_mutex);

    return NULL;
}

/**
 * Initializes the database, mutexes, and threads.
 * Waits for all threads to complete and cleans up resources.
 */
int main() {
    srand(time(NULL));

    for (int i = 0; i < NUM_STUDENTS - 1; i++) {
        database[i] = 1000 + i;
    }
    database[NUM_STUDENTS - 1] = 9999;

    for (int i = 0; i < NUM_TAS; i++) {
        pthread_mutex_init(&mutexes[i], NULL);
        ta_progress[i] = 0;
    }

    pthread_t ta_threads[NUM_TAS];
    int ta_ids[NUM_TAS];

    for (int i = 0; i < NUM_TAS; i++) {
        ta_ids[i] = i;
        if (pthread_create(&ta_threads[i], NULL, ta_task, &ta_ids[i]) != 0) {
            perror("Error creating thread");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < NUM_TAS; i++) {
        pthread_join(ta_threads[i], NULL);
    }

    for (int i = 0; i < NUM_TAS; i++) {
        pthread_mutex_destroy(&mutexes[i]);
    }

    printf("All TAs have finished marking.\n");

    return 0;
}