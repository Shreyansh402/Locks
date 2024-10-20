#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>

void *reader(void *arg) {
    // your code here
    return NULL;
}

void *writer(void *arg) {
    // your code here
    return NULL;
}

int main(int argc, char **argv) {
    
    //Do not change the code below to spawn threads
    if(argc != 3) return 1;
    int n = atoi(argv[1]);
    int m = atoi(argv[2]);
    pthread_t readers[n], writers[m];

    // Create reader and writer threads
    for (long i = 0; i < n; i++) pthread_create(&readers[i], NULL, reader, NULL);        
    for (long i = 0; i < m; i++) pthread_create(&writers[i], NULL, writer, NULL);

    // Wait for all threads to complete
    for (int i = 0; i < n; i++) pthread_join(readers[i], NULL);
    for (int i = 0; i < m; i++) pthread_join(writers[i], NULL);

    return 0;
}