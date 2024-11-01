#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

// Global variables
int reader_count = 0;
sem_t *counter_lock; // Protects reader_count
sem_t *output_lock;  // Protects output file
sem_t *write_lock;   // Controls write access
sem_t *read_lock;    // Controls read access
FILE *output_file;
FILE *shared_file;

void cleanup_semaphores()
{
    sem_close(counter_lock);
    sem_close(write_lock);
    sem_close(output_lock);
    sem_close(read_lock);
    sem_unlink("/counter_lock_sem");
    sem_unlink("/write_lock_sem");
    sem_unlink("/read_lock_sem");
    sem_unlink("/output_lock_sem");
}

void *reader(void *arg)
{
    // your code here
    char buffer[100];

    // Entry section
    sem_wait(counter_lock);
    reader_count++;
    int reader_num = reader_count;
    if (reader_count == 1)
    {
        // First reader blocks writers
        sem_wait(write_lock);
    }
    sem_post(counter_lock);

    sem_wait(output_lock);

    // Output to file
    output_file = fopen("output-reader-pref.txt", "a");
    fprintf(output_file, "Reading,Number-of-readers-present:[%d]\n", reader_num);
    fclose(output_file);

    sem_post(output_lock);

    // Critical section
    sem_wait(read_lock);
    shared_file = fopen("shared-file.txt", "r");
    if (shared_file == NULL)
    {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    while (fgets(buffer, sizeof(buffer), shared_file) != NULL)
    {
        // Reading the file
    }
    // sleep(1); // Simulate reading time
    fclose(shared_file);
    sem_post(read_lock);

    // Exit section
    sem_wait(counter_lock);
    reader_count--;
    if (reader_count == 0)
    {
        // Last reader unblocks writers
        sem_post(write_lock);
    }
    sem_post(counter_lock);

    return NULL;
}

void *writer(void *arg)
{
    // your code here
    // Entry section
    sem_wait(write_lock);

    // Output to file
    output_file = fopen("output-reader-pref.txt", "a");
    fprintf(output_file, "Writing,Number-of-readers-present:[%d]\n", reader_count);
    fclose(output_file);

    // Critical section
    shared_file = fopen("shared-file.txt", "a");
    fprintf(shared_file, "Hello world!\n");
    fclose(shared_file);

    // sleep(1); // Simulate writing time

    // Exit section
    sem_post(write_lock);

    return NULL;
}

int main(int argc, char **argv)
{

    // Do not change the code below to spawn threads
    if (argc != 3)
        return 1;
    int n = atoi(argv[1]);
    int m = atoi(argv[2]);
    pthread_t readers[n], writers[m];

    // Clean up any existing semaphores
    sem_unlink("/counter_lock_sem");
    sem_unlink("/write_lock_sem");
    sem_unlink("/read_lock_sem");
    sem_unlink("/output_lock_sem");

    // Initialize semaphores
    counter_lock = sem_open("/counter_lock_sem", O_CREAT | O_EXCL, 0644, 1);
    if (counter_lock == SEM_FAILED)
    {
        perror("sem_open counter_lock failed");
        return 1;
    }

    write_lock = sem_open("/write_lock_sem", O_CREAT | O_EXCL, 0644, 1);
    if (write_lock == SEM_FAILED)
    {
        perror("sem_open write_lock failed");
        sem_unlink("/counter_lock_sem");
        return 1;
    }

    read_lock = sem_open("/read_lock_sem", O_CREAT | O_EXCL, 0644, 1);
    if (read_lock == SEM_FAILED)
    {
        perror("sem_open read_lock failed");
        sem_unlink("/counter_lock_sem");
        sem_unlink("/write_lock_sem");
        return 1;
    }

    output_lock = sem_open("/output_lock_sem", O_CREAT | O_EXCL, 0644, 1);
    if (output_lock == SEM_FAILED)
    {
        perror("sem_open output_lock failed");
        sem_unlink("/counter_lock_sem");
        sem_unlink("/write_lock_sem");
        sem_unlink("/read_lock_sem");
        return 1;
    }

    // Clear output file
    output_file = fopen("output-reader-pref.txt", "w");
    fclose(output_file);

    // Create reader and writer threads
    for (int i = 0; i < n; i++)
        pthread_create(&readers[i], NULL, reader, NULL);
    for (int i = 0; i < m; i++)
        pthread_create(&writers[i], NULL, writer, NULL);

    // Wait for all threads to complete
    for (int i = 0; i < n; i++)
        pthread_join(readers[i], NULL);
    for (int i = 0; i < m; i++)
        pthread_join(writers[i], NULL);

    // Clean up
    cleanup_semaphores();

    return 0;
}