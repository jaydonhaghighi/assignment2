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
#define SEM_BASE_NAME "/ta_semaphore_"

// Struct to store shared data
typedef struct {
    int students[NUM_STUDENTS];
    int current_index;
} shared_data_t;

// Function to simulate delay
void random_delay(int min, int max) {
    sleep(min + rand() % (max - min + 1));
}

// Function to initialize the student list in shared memory
void initialize_shared_memory(shared_data_t *data) {
    for (int i = 0; i < NUM_STUDENTS; i++) {
        data->students[i] = (i < NUM_STUDENTS - 1) ? i + 1 : 9999; // 9999 marks the end
    }
    data->current_index = 0;
}

// Function to create semaphore names
void get_semaphore_name(int index, char *buffer, size_t size) {
    snprintf(buffer, size, "%s%d", SEM_BASE_NAME, index);
}

// TA process
void ta_process(int ta_id, sem_t **semaphores, shared_data_t *shared_data) {
    char ta_file[10];
    snprintf(ta_file, sizeof(ta_file), "TA%d.txt", ta_id);
    FILE *log = fopen(ta_file, "w");
    if (!log) {
        perror("Error creating TA file");
        exit(EXIT_FAILURE);
    }

    srand(time(NULL) + ta_id); // Unique seed for each TA
    for (int cycle = 0; cycle < NUM_CYCLES; cycle++) {
        while (1) {
            int acquired = 0; // Track if semaphores are acquired
            int student_id = -1;

            // Attempt to acquire semaphore j and (j+1) % NUM_TAS
            sem_wait(semaphores[ta_id - 1]); // Acquire semaphore j
            acquired++;
            if (sem_trywait(semaphores[ta_id % NUM_TAS]) == 0) { // Try to acquire (j+1) % NUM_TAS
                acquired++;
            } else {
                // If unable to acquire (j+1), release j
                sem_post(semaphores[ta_id - 1]);
                acquired--;
                continue;
            }

            // Access shared memory
            student_id = shared_data->students[shared_data->current_index];
            if (student_id == 9999) {
                shared_data->current_index = 0; // Reset index for the next cycle
                sem_post(semaphores[ta_id - 1]);
                sem_post(semaphores[ta_id % NUM_TAS]);
                break;
            }
            int local_index = shared_data->current_index++;
            sem_post(semaphores[ta_id - 1]); // Release semaphore j
            sem_post(semaphores[ta_id % NUM_TAS]); // Release (j+1)

            // Simulate marking process
            random_delay(1, 4); // Simulate delay accessing shared data
            int mark = rand() % (MAX_MARK + 1);
            fprintf(log, "Student: %04d, Mark: %d\n", student_id, mark);
            fflush(log);
            printf("TA%d: Marked Student %04d with %d\n", ta_id, student_id, mark);

            random_delay(1, 10); // Simulate marking delay
        }
    }
    fclose(log);
    printf("TA%d: Finished marking\n", ta_id);
}

int main() {
    // Shared memory setup
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

    // Semaphore setup
    sem_t *semaphores[NUM_TAS];
    for (int i = 0; i < NUM_TAS; i++) {
        char sem_name[32];
        get_semaphore_name(i, sem_name, sizeof(sem_name));
        semaphores[i] = sem_open(sem_name, O_CREAT | O_EXCL, 0644, 1);
        if (semaphores[i] == SEM_FAILED) {
            perror("Error creating semaphore");
            exit(EXIT_FAILURE);
        }
    }

    // Create TA processes
    pid_t pids[NUM_TAS];
    for (int i = 0; i < NUM_TAS; i++) {
        pids[i] = fork();
        if (pids[i] == 0) {
            ta_process(i + 1, semaphores, shared_data);
            exit(EXIT_SUCCESS);
        }
    }

    // Wait for all processes to finish
    for (int i = 0; i < NUM_TAS; i++) {
        waitpid(pids[i], NULL, 0);
    }

    // Cleanup
    for (int i = 0; i < NUM_TAS; i++) {
        sem_close(semaphores[i]);
        char sem_name[32];
        get_semaphore_name(i, sem_name, sizeof(sem_name));
        sem_unlink(sem_name);
    }
    munmap(shared_data, sizeof(shared_data_t));
    shm_unlink(SHM_NAME);

    printf("All TAs finished marking. Program complete.\n");
    return 0;
}
