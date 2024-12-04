#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <time.h>
#include <string.h>

#define NUM_TAS 5
#define NUM_STUDENTS 20
#define NUM_CYCLES 3
#define MAX_MARK 10
#define SHM_NAME "/student_list_shm"
#define SEM_MUTEX_NAME "/shared_mutex"

typedef struct {
    int students[NUM_STUDENTS];
    int current_index;
} shared_data_t;

/*
 * Introduces a random delay between a minimum and maximum duration.
 */
void random_delay(int min, int max) {
    sleep(min + rand() % (max - min + 1));
}

/*
 * Initializes the shared memory with a list of student IDs.
 * The last student ID is set to 9999 to indicate the end of the list.
 */
void initialize_shared_memory(shared_data_t *data) {
    for (int i = 0; i < NUM_STUDENTS; i++) {
        data->students[i] = (i < NUM_STUDENTS - 1) ? i + 1 : 9999;
    }
    data->current_index = 0;
}

/*
 * Simulates the process of a TA marking student assignments.
 * Each TA accesses the shared memory in a synchronized manner using a mutex.
 * Parameters:
 *   ta_id - ID of the TA process
 *   mutex - mutex for synchronizing access to shared data
 *   shared_data - pointer to the shared memory data structure
 */
void ta_process(int ta_id, sem_t *mutex, shared_data_t *shared_data) {
    char ta_file[10];
    snprintf(ta_file, sizeof(ta_file), "TA%d.txt", ta_id);
    FILE *log = fopen(ta_file, "w");
    if (!log) {
        perror("Error creating TA file");
        exit(EXIT_FAILURE);
    }

    srand(time(NULL) + ta_id);
    for (int cycle = 0; cycle < NUM_CYCLES; cycle++) {
        while (1) {
            int student_id = -1;

            sem_wait(mutex);
            if (shared_data->current_index < NUM_STUDENTS) {
                student_id = shared_data->students[shared_data->current_index++];
            }
            sem_post(mutex);

            if (student_id == -1 || student_id == 9999) {
                break;
            }

            random_delay(1, 4);
            int mark = rand() % (MAX_MARK + 1);
            fprintf(log, "Student: %04d, Mark: %d\n", student_id, mark);
            fflush(log);
            printf("TA%d: Marked Student %04d with %d\n", ta_id, student_id, mark);

            random_delay(1, 10);
        }
    }
    fclose(log);
    printf("TA%d: Finished marking\n", ta_id);
}

/*
 * Initializes shared memory and mutex, creates TA processes,
 * and waits for all processes to complete before cleaning up resources.
 * Returns:
 *   0 on successful completion.
 */
int main() {
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Error creating shared memory");
        exit(EXIT_FAILURE);
    }
    ftruncate(shm_fd, sizeof(shared_data_t));
    shared_data_t *shared_data = mmap(NULL, sizeof(shared_data_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        perror("Error mapping shared memory");
        exit(EXIT_FAILURE);
    }
    initialize_shared_memory(shared_data);

    sem_t *mutex = sem_open(SEM_MUTEX_NAME, O_CREAT | O_EXCL, 0644, 1);
    if (mutex == SEM_FAILED) {
        perror("Error creating mutex");
        exit(EXIT_FAILURE);
    }

    pid_t pids[NUM_TAS];
    for (int i = 0; i < NUM_TAS; i++) {
        pids[i] = fork();
        if (pids[i] == 0) {
            ta_process(i + 1, mutex, shared_data);
            exit(EXIT_SUCCESS);
        }
    }

    for (int i = 0; i < NUM_TAS; i++) {
        waitpid(pids[i], NULL, 0);
    }

    sem_close(mutex);
    sem_unlink(SEM_MUTEX_NAME);
    munmap(shared_data, sizeof(shared_data_t));
    shm_unlink(SHM_NAME);

    printf("All TAs finished marking. Program complete.\n");
    return 0;
}