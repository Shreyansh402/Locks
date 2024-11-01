#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

// Global variables
int reader_count = 0;
int writer_count = 0;
sem_t *mutex_readers; // Protects reader_count
sem_t *mutex_writers; // Protects writer_count
sem_t *write_lock;    // Controls write access
sem_t *read_lock;     // Controls read access
FILE *output_file;
FILE *shared_file;

void cleanup_semaphores()
{
    sem_close(mutex_readers);
    sem_close(mutex_writers);
    sem_close(write_lock);
    sem_close(read_lock);
    sem_unlink("/mutex_readers_sem");
    sem_unlink("/mutex_writers_sem");
    sem_unlink("/write_lock_sem");
    sem_unlink("/read_lock_sem");
}

void *reader(void *arg)
{
    char buffer[100];

    // Wait for any writers to finish
    sem_wait(read_lock);

    // Entry section
    sem_wait(mutex_readers);
    reader_count++;
    if (reader_count == 1)
    {
        sem_wait(write_lock);
    }

    // Output to file
    output_file = fopen("output-writer-pref.txt", "a");
    fprintf(output_file, "Reading,Number-of-readers-present:[%d]\n", reader_count);
    fclose(output_file);

    sem_post(mutex_readers);
    sem_post(read_lock);

    // Critical section
    shared_file = fopen("shared-file.txt", "r");
    while (fgets(buffer, sizeof(buffer), shared_file) != NULL)
    {
        // Reading the file
        sleep(1); // Simulate reading time
    }
    fclose(shared_file);

    // Exit section
    sem_wait(mutex_readers);
    reader_count--;
    if (reader_count == 0)
    {
        sem_post(write_lock);
    }
    sem_post(mutex_readers);
    return NULL;
}

void *writer(void *arg)
{
    // your code here
    sem_wait(mutex_writers);
    writer_count++;
    if (writer_count == 1)
    {
        sem_wait(read_lock);
    }
    sem_post(mutex_writers);

    sem_wait(write_lock);

    // Output to file
    output_file = fopen("output-writer-pref.txt", "a");
    fprintf(output_file, "Writing,Number-of-readers-present:[0]\n");
    fclose(output_file);

    // Critical section
    shared_file = fopen("shared-file.txt", "a");
    fprintf(shared_file, "Hello world!\n");
    fclose(shared_file);

    sleep(1); // Simulate writing time

    // Exit section
    sem_post(write_lock);

    sem_wait(mutex_writers);
    writer_count--;
    if (writer_count == 0)
    {
        sem_post(read_lock);
    }
    sem_post(mutex_writers);
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
    sem_unlink("/mutex_readers_sem");
    sem_unlink("/mutex_writers_sem");
    sem_unlink("/write_lock_sem");
    sem_unlink("/read_lock_sem");

    // Initialize semaphores
    mutex_readers = sem_open("/mutex_readers_sem", O_CREAT | O_EXCL, 0644, 1);
    if (mutex_readers == SEM_FAILED)
    {
        perror("sem_open mutex_readers failed");
        return 1;
    }

    mutex_writers = sem_open("/mutex_writers_sem", O_CREAT | O_EXCL, 0644, 1);
    if (mutex_writers == SEM_FAILED)
    {
        perror("sem_open mutex_writers failed");
        cleanup_semaphores();
        return 1;
    }

    write_lock = sem_open("/write_lock_sem", O_CREAT | O_EXCL, 0644, 1);
    if (write_lock == SEM_FAILED)
    {
        perror("sem_open write_lock failed");
        cleanup_semaphores();
        return 1;
    }

    read_lock = sem_open("/read_lock_sem", O_CREAT | O_EXCL, 0644, 1);
    if (read_lock == SEM_FAILED)
    {
        perror("sem_open read_lock failed");
        cleanup_semaphores();
        return 1;
    }

    // Clear output file
    output_file = fopen("output-writer-pref.txt", "w");
    fclose(output_file);

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

    cleanup_semaphores();

    return 0;
}