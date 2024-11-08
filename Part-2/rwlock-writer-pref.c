#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
// #include <time.h>

// Global variables
int reader_count = 0;
int writer_count = 0;
sem_t *counter_lock;      // Protects writer_count
sem_t *reader_count_lock; // Protects reader_count
sem_t *write_lock;        // Controls write access
sem_t *read_lock;         // Controls read access
sem_t *output_lock;       // Protects output file
FILE *output_file;
// struct timespec start;
// FILE *shared_file;
FILE *shared_writer_file;

void cleanup_semaphores()
{
    sem_close(counter_lock);
    sem_close(write_lock);
    sem_close(read_lock);
    sem_close(output_lock);
    sem_close(reader_count_lock);
    sem_unlink("/counter_lock_sem");
    sem_unlink("/write_lock_sem");
    sem_unlink("/read_lock_sem");
    sem_unlink("/output_lock_sem");
    sem_unlink("/reader_count_lock_sem");
}

int initialize_semaphores()
{
    // Clean up any existing semaphores
    sem_unlink("/counter_lock_sem");
    sem_unlink("/write_lock_sem");
    sem_unlink("/read_lock_sem");
    sem_unlink("/output_lock_sem");
    sem_unlink("/reader_count_lock_sem");

    // Initialize semaphores

    counter_lock = sem_open("/counter_lock_sem", O_CREAT | O_EXCL, 0644, 1);
    if (counter_lock == SEM_FAILED)
    {
        perror("sem_open failed");
        cleanup_semaphores();
        return 1;
    }

    write_lock = sem_open("/write_lock_sem", O_CREAT | O_EXCL, 0644, 1);
    if (write_lock == SEM_FAILED)
    {
        perror("sem_open failed");
        cleanup_semaphores();
        return 1;
    }

    read_lock = sem_open("/read_lock_sem", O_CREAT | O_EXCL, 0644, 1);
    if (read_lock == SEM_FAILED)
    {
        perror("sem_open failed");
        cleanup_semaphores();
        return 1;
    }

    output_lock = sem_open("/output_lock_sem", O_CREAT | O_EXCL, 0644, 1);
    if (output_lock == SEM_FAILED)
    {
        perror("sem_open failed");
        cleanup_semaphores();
        return 1;
    }

    reader_count_lock = sem_open("/reader_count_lock_sem", O_CREAT | O_EXCL, 0644, 1);
    if (reader_count_lock == SEM_FAILED)
    {
        perror("sem_open failed");
        cleanup_semaphores();
        return 1;
    }

    // Clear output file
    output_file = fopen("output-writer-pref.txt", "w");
    fclose(output_file);

    return 0;
}

void *reader(void *arg)
{
    char buffer[100];

    // Wait for any writers to finish
    sem_wait(read_lock);
    while (writer_count > 0)
    {
        sem_post(read_lock);
        sem_wait(read_lock);
    }
    sem_post(read_lock);

    // Increment reader count
    sem_wait(reader_count_lock);
    int reader_num = ++reader_count;
    if (reader_count == 1)
    {
        sem_wait(write_lock);
    }
    sem_post(reader_count_lock);

    // Output to file
    sem_wait(output_lock);
    output_file = fopen("output-writer-pref.txt", "a");
    fprintf(output_file, "Reading,Number-of-readers-present:[%d]\n", reader_num);
    fclose(output_file);
    sem_post(output_lock);

    // Read from shared file
    // int lines = 0;
    FILE *shared_file = fopen("shared-file.txt", "r");
    while (fgets(buffer, sizeof(buffer), shared_file) != NULL)
    {
        // Reading the file
        // lines++;
    }
    // sleep(1);
    fclose(shared_file);
    // printf("Number of lines in the file: %d\n", lines);

    // struct timespec end;
    // clock_gettime(CLOCK_REALTIME, &end);
    // long elapsed = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000;
    // printf("Time taken Reader:  %ld\n", elapsed);

    // Decrement reader count
    sem_wait(reader_count_lock);
    reader_count--;
    if (reader_count == 0)
    {
        sem_post(write_lock);
    }
    sem_post(reader_count_lock);

    return NULL;
}

void *writer(void *arg)
{
    // your code here

    // Increment writer count
    sem_wait(counter_lock);
    ++writer_count;
    if (writer_count == 1)
    {
        sem_wait(read_lock);
    }
    sem_post(counter_lock);

    sem_wait(write_lock);
    // Output to file
    sem_wait(output_lock);
    output_file = fopen("output-writer-pref.txt", "a");
    fprintf(output_file, "Writing,Number-of-readers-present:[%d]\n", reader_count);
    fclose(output_file);
    sem_post(output_lock);

    // Write to shared file
    shared_writer_file = fopen("shared-file.txt", "a");
    fprintf(shared_writer_file, "Hello world!\n");
    fclose(shared_writer_file);
    // sleep(1);

    // struct timespec end;
    // clock_gettime(CLOCK_REALTIME, &end);
    // long elapsed = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000;
    // printf("Time taken Writer:  %ld\n", elapsed);

    // Decrement writer count
    sem_wait(counter_lock);
    writer_count--;
    if (writer_count == 0)
    {
        sem_post(read_lock);
    }
    sem_post(counter_lock);
    sem_post(write_lock);

    return NULL;
}

int main(int argc, char **argv)
{
    // Initialize semaphores
    int init_status = initialize_semaphores();
    if (init_status != 0)
    {
        return init_status;
    }
    // clock_gettime(CLOCK_REALTIME, &start);

    // Do not change the code below to spawn threads
    if (argc != 3)
        return 1;
    int n = atoi(argv[1]);
    int m = atoi(argv[2]);
    pthread_t readers[n], writers[m];

    // Create reader and writer threads
    for (long i = 0; i < n; i++)
        pthread_create(&readers[i], NULL, reader, NULL);
    for (long i = 0; i < m; i++)
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