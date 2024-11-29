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

#define NUM_TAS 5
#define NUM_STUDENTS 20 // One student per row

typedef struct {
    int current_index;              // Shared index of the student being processed
    int student_numbers[NUM_STUDENTS]; // Shared list of student numbers
} SharedData;

sem_t *sems[NUM_TAS]; // Global semaphore array for cleanup
SharedData *shared_data = NULL; // Global shared data pointer

// Function to clean up semaphores and shared memory
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

// Function to create the student list in shared memory and save to a file
void create_student_list(const char *filename) {
    FILE *db = fopen(filename, "w");
    if (db == NULL) {
        perror("Failed to create database file");
        exit(1);
    }

    srand(time(NULL));
    for (int i = 0; i < NUM_STUDENTS - 1; i++) {
        int student_num = rand() % 9999 + 1; // Generate random 4-digit student number
        shared_data->student_numbers[i] = student_num;
        fprintf(db, "%04d\n", student_num); // Write to file, one per row
    }
    // Add last student as 9999 to indicate end of the list
    shared_data->student_numbers[NUM_STUDENTS - 1] = 9999;
    fprintf(db, "9999\n");
    fclose(db);
    printf("Database file '%s' created successfully with %d students.\n", filename, NUM_STUDENTS);
}

void ta_process(int ta_id) {
    int graded_count[NUM_STUDENTS] = {0}; // Tracks how many times each student is graded
    int total_students = NUM_STUDENTS;
    int own_sem = ta_id - 1;
    int next_sem = ta_id % NUM_TAS;

    srand(time(NULL) + getpid()); // Seed random number generator

    while (1) {
        // Lock semaphores
        sem_wait(sems[own_sem]);
        sem_wait(sems[next_sem]);

        // Access shared index
        printf("TA%d is accessing the database.\n", ta_id);
        int index = shared_data->current_index;

        // Exit condition: all students graded 3 times
        int all_graded = 1;
        for (int i = 0; i < total_students; i++) {
            if (graded_count[i] < 3) {
                all_graded = 0;
                break;
            }
        }
        if (all_graded) {
            // Release semaphores and exit
            sem_post(sems[own_sem]);
            sem_post(sems[next_sem]);
            break;
        }

        if (index < total_students && graded_count[index] < 3) {
            // Mark the student
            int student_num = shared_data->student_numbers[index];
            graded_count[index]++;
            shared_data->current_index++;

            printf("TA%d is marking student number %04d.\n", ta_id, student_num);
            int mark = rand() % 11; // Random mark between 0 and 10
            printf("TA%d assigned mark %d to student %04d.\n", ta_id, mark, student_num);

            // Write to TA's file
            char filename[10];
            snprintf(filename, sizeof(filename), "TA%d.txt", ta_id);
            FILE *fp = fopen(filename, "a");
            if (fp == NULL) {
                perror("Failed to open TA file");
                exit(1);
            }
            fprintf(fp, "Student %04d: Mark %d\n", student_num, mark);
            fclose(fp);
        }

        // Reset current_index if all students are visited
        if (shared_data->current_index >= total_students) {
            shared_data->current_index = 0;
        }

        // Release semaphores
        sem_post(sems[own_sem]);
        sem_post(sems[next_sem]);

        // Simulate marking delay
        sleep(rand() % 10 + 1);
    }

    printf("TA%d has finished marking.\n", ta_id);
    exit(0);
}

// Main function
int main() {
    const char *database_filename = "database.txt";

    // Handle signals for cleanup
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // Initialize resources
    initialize_resources();

    // Create the student list in shared memory and save to file
    create_student_list(database_filename);

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
