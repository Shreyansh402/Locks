#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define BUFFER_SIZE 100

// Circular buffer
typedef struct
{
    unsigned int buffer[BUFFER_SIZE];
    int head;
    int tail;
    int count;                // Number of elements in buffer at any given time, important variable to check if buffer is full or empty
    pthread_mutex_t mutex;    // Mutex to protect buffer
    pthread_cond_t not_full;  // Condition variable to signal when buffer is not full
    pthread_cond_t not_empty; // Condition variable to signal when buffer is not empty
} CircularBuffer;

CircularBuffer cb;
FILE *input_file;
FILE *output_file;
int production_complete = 0;

// Initialize the circular buffer
void init_buffer()
{
    cb.head = 0;
    cb.tail = 0;
    cb.count = 0;
    pthread_mutex_init(&cb.mutex, NULL);
    pthread_cond_init(&cb.not_full, NULL);
    pthread_cond_init(&cb.not_empty, NULL);
}

// Add element to buffer
void produce(unsigned int item)
{
    pthread_mutex_lock(&cb.mutex); // Lock the mutex as we are going to modify the buffer

    while (cb.count == BUFFER_SIZE)
    {
        pthread_cond_wait(&cb.not_full, &cb.mutex); // Wait for buffer to become not full
    }

    cb.buffer[cb.tail] = item;
    cb.tail = (cb.tail + 1) % BUFFER_SIZE;
    cb.count++;

    pthread_cond_signal(&cb.not_empty); // Signal that buffer is not empty
    pthread_mutex_unlock(&cb.mutex);    // Unlock the mutex
}

// Remove element from buffer and write to output file
unsigned int consume()
{
    pthread_mutex_lock(&cb.mutex); // Lock the mutex as we are going to modify the buffer

    while (cb.count == 0)
    {
        if (production_complete)
        {
            pthread_mutex_unlock(&cb.mutex); // Unlock the mutex before returning
            return 0;
        }
        pthread_cond_wait(&cb.not_empty, &cb.mutex); // Wait for buffer to become not empty
    }

    unsigned int item = cb.buffer[cb.head];
    cb.head = (cb.head + 1) % BUFFER_SIZE;
    cb.count--;

    pthread_cond_signal(&cb.not_full); // Signal that buffer is not full
    pthread_mutex_unlock(&cb.mutex);   // Unlock the mutex

    // Write the current buffer state to output file
    fprintf(output_file, "Consumed:[%u],BufferState:[", item);
    if (cb.count > 0)
    {
        int current = cb.head;
        for (int i = 0; i < cb.count; i++)
        {
            fprintf(output_file, "%u", cb.buffer[current]);
            if (i < cb.count - 1)
            {
                fprintf(output_file, ",");
            }
            current = (current + 1) % BUFFER_SIZE;
        }
    }
    fprintf(output_file, "]\n");

    // pthread_mutex_unlock(&cb.mutex);
    return item;
}

// Producer thread function
void *producer(void *arg)
{
    unsigned int item;

    while (fscanf(input_file, "%u", &item) == 1)
    {
        if (item == 0)
        {
            production_complete = 1;
            pthread_cond_signal(&cb.not_empty); // Signal that buffer is not empty
            break;
        }
        produce(item);
    }

    return NULL;
}

// Consumer thread function
void *consumer(void *arg)
{
    while (1)
    {
        unsigned int item = consume();
        if (production_complete && cb.count == 0)
        {
            break;
        }
    }

    return NULL;
}

int main()
{
    // Initialize buffer
    init_buffer();

    // Open input and output files
    input_file = fopen("input-part1.txt", "r");
    if (input_file == NULL)
    {
        perror("Error opening input file");
        return 1;
    }

    output_file = fopen("output-part1.txt", "w");
    if (output_file == NULL)
    {
        perror("Error opening output file");
        fclose(input_file);
        return 1;
    }

    // Create producer and consumer threads
    pthread_t producer_thread, consumer_thread;
    pthread_create(&producer_thread, NULL, producer, NULL);
    pthread_create(&consumer_thread, NULL, consumer, NULL);

    // Wait for threads to complete
    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);

    // Cleanup
    pthread_mutex_destroy(&cb.mutex);
    pthread_cond_destroy(&cb.not_full);
    pthread_cond_destroy(&cb.not_empty);
    fclose(input_file);
    fclose(output_file);

    return 0;
}