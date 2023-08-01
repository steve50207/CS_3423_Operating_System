#ifndef TABLE_HPP
#define TABLE_HPP

#include <pthread.h>

class Table {
public:
    Table(int n);
    void wait();
    void signal();
    ~Table();

private:
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int value;

};



#endif // TABLE_HPP