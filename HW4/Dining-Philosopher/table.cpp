#include "table.hpp"
#include "stdio.h"

Table::Table(int n) {
    // TODO: implement table constructor (value, mutex, cond)
    value = n - 1;
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
}

void Table::wait() {
    // TODO: implement semaphore wait
    pthread_mutex_lock(&mutex);
    while (value <= 0) {
        pthread_cond_wait(&cond, &mutex);
    }
    value--;
    pthread_mutex_unlock(&mutex);
}

void Table::signal() {
    // TODO: implement semaphore signal
    pthread_mutex_lock(&mutex);
    value++;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
}

Table::~Table() {
    // TODO: implement table destructor (mutex, cond)
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
}