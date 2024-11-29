#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>
#include <string.h>

#define NUM_TAS 5
#define NUM_STUDENTS 80

typedef struct {
    int student_numbers[NUM_STUDENTS]; // Shared list of student numbers
    int current_index;                 // Index of the next student to mark
} SharedData;

sem_t *sems[NUM_TAS];       // Array of semaphores
SharedData *shared_data = NULL; // Pointer to shared data

// Function to clean up resources
void cleanup_resources() {
    char sem_name[10];
    for (int i = 0; i < NUM_TAS; i++) {
        snprintf(sem_name, sizeof(sem_name), "/sem%d", i + 1);
        sem_unlink(sem_name); // Delete semaphore
    }
    shm_unlink("/shared_mem"); // Delete shared memory
    printf("Resources cleaned up successfully.\n");
}

// Signal handler for cleanup
void handle_signal(int signum) {
    printf("\nSignal %d received. Cleaning up resources...\n", signum);
    cleanup_resources();
    exit(0);
}

// Function to initialize semaphores and shared memory
void initialize_resources() {
    char sem_name[10];
    for (int i = 0; i < NUM_TAS; i++) {
        snprintf(sem_name, sizeof(sem_name), "/sem%d", i + 1);
        sem_unlink(sem_name); // Ensure semaphore doesn't already exist
        sems[i] = sem_open(sem_name, O_CREAT | O_EXCL, 0644, 1);
        if (sems[i] == SEM_FAILED) {
            perror("sem_open failed");
            cleanup_resources();
            exit(1);
        }
    }

    // Create and map shared memory
    shm_unlink("/shared_mem"); // Ensure shared memory doesn't already exist
    int shm_fd = shm_open("/shared_mem", O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        cleanup_resources();
        exit(1);
    }
    ftruncate(shm_fd, sizeof(SharedData));
    shared_data = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        perror("mmap failed");
        cleanup_resources();
        exit(1);
    }
    shared_data->current_index = 0;
}

// Function to create the student list in shared memory
void create_student_list() {
    srand(time(NULL));
    int student_count = 0;
    while (student_count < NUM_STUDENTS - 1) {
        int student_num = rand() % 9999 + 1; // Random 4-digit student number
        shared_data->student_numbers[student_count] = student_num;
        student_count++;
    }
    // Last number is 9999
    shared_data->student_numbers[NUM_STUDENTS - 1] = 9999;
}

// TA process function
void ta_process(int ta_id) {
    int times_through_list = 0;
    int total_students = NUM_STUDENTS;
    int own_sem = ta_id - 1;
    int next_sem = ta_id % NUM_TAS;

    srand(time(NULL) + getpid()); // Seed random number generator

    while (times_through_list < 3) {
        // Lock semaphores
        sem_wait(sems[own_sem]);
        sem_wait(sems[next_sem]);

        printf("TA%d is accessing the database.\n", ta_id);

        // Access shared index
        int index = shared_data->current_index;
        if (index >= total_students || shared_data->student_numbers[index] == 9999) {
            shared_data->current_index = 0;
            times_through_list++;
            printf("TA%d has completed %d time(s) through the list.\n", ta_id, times_through_list);
            // Release semaphores
            sem_post(sems[own_sem]);
            sem_post(sems[next_sem]);
            continue;
        }
        int student_num = shared_data->student_numbers[index];
        shared_data->current_index++;

        // Simulate delay while accessing database
        sleep(rand() % 4 + 1);

        // Release semaphores
        sem_post(sems[own_sem]);
        sem_post(sems[next_sem]);

        // Simulate marking
        printf("TA%d is marking student number %04d.\n", ta_id, student_num);
        int mark = rand() % 11; // Random mark between 0 and 10
        printf("TA%d assigned mark %d to student %04d.\n", ta_id, mark, student_num);

        // Simulate marking delay
        sleep(rand() % 10 + 1);
    }
    printf("TA%d has finished marking.\n", ta_id);
    exit(0);
}

// Main function
int main() {
    // Handle signals for cleanup
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // Initialize resources
    initialize_resources();

    // Create the student list in shared memory
    create_student_list();

    // Create TA processes
    for (int i = 1; i <= NUM_TAS; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            ta_process(i);
        } else if (pid < 0) {
            perror("fork failed");
            cleanup_resources();
            exit(1);
        }
    }

    // Wait for all TA processes to finish
    for (int i = 0; i < NUM_TAS; i++) {
        wait(NULL);
    }

    // Clean up resources
    cleanup_resources();

    printf("All TAs have finished marking.\n");
    return 0;
}
