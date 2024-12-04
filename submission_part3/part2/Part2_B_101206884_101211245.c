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
#define SEM_NAME "/ta_semaphore"

// shared data
typedef struct {
    int students[NUM_STUDENTS];
    int current_index;
} shared_data_t;

// simulate delay
void random_delay(int min, int max) {
    sleep(min + rand() % (max - min + 1));
}

// initialize the student list in shared memory
void initialize_shared_memory(shared_data_t *data) {
    for (int i = 0; i < NUM_STUDENTS; i++) {
        data->students[i] = (i < NUM_STUDENTS - 1) ? i + 1 : 9999; // 9999 marks the end
    }
    data->current_index = 0;
}

// TA process
void ta_process(int ta_id, sem_t *sem, shared_data_t *shared_data) {
    char ta_file[10];
    snprintf(ta_file, sizeof(ta_file), "TA%d.txt", ta_id);
    FILE *log = fopen(ta_file, "w");
    if (!log) {
        perror("Error creating TA file");
        exit(EXIT_FAILURE);
    }

    srand(time(NULL) + ta_id); // unique seed for each TA
    for (int cycle = 0; cycle < NUM_CYCLES; cycle++) {
        while (1) {
            // access the shared data
            sem_wait(sem);
            int student_id = shared_data->students[shared_data->current_index];
            if (student_id == 9999) {
                shared_data->current_index = 0; // resets index for the next cycle
                sem_post(sem);
                break;
            }
            int local_index = shared_data->current_index++;
            sem_post(sem);

            // simulate marking process
            random_delay(1, 4); // simulate delay accessing shared data
            int mark = rand() % (MAX_MARK + 1);
            fprintf(log, "Student: %04d, Mark: %d\n", student_id, mark);
            fflush(log);
            printf("TA%d: Marked Student %04d with %d\n", ta_id, student_id, mark);

            random_delay(1, 10); // simulate marking delay
        }
    }
    fclose(log);
    printf("TA%d: Finished marking\n", ta_id);
}

int main() {
    // shared memory setup
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

    // semaphore setup
    sem_t *sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0644, 1);
    if (sem == SEM_FAILED) {
        perror("Error creating semaphore");
        exit(EXIT_FAILURE);
    }

    // creates TA processes
    pid_t pids[NUM_TAS];
    for (int i = 0; i < NUM_TAS; i++) {
        pids[i] = fork();
        if (pids[i] == 0) {
            ta_process(i + 1, sem, shared_data);
            exit(EXIT_SUCCESS);
        }
    }

    // waits for all processes to finish
    for (int i = 0; i < NUM_TAS; i++) {
        waitpid(pids[i], NULL, 0);
    }

    // cleanup
    sem_close(sem);
    sem_unlink(SEM_NAME);
    munmap(shared_data, sizeof(shared_data_t));
    shm_unlink(SHM_NAME);

    printf("All TAs finished marking. Program complete.");
    return 0;
}