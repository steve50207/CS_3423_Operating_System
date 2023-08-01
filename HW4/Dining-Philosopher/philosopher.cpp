#include "philosopher.hpp"


Philosopher::Philosopher(int id, Fork *leftFork, Fork *rightFork, Table *table) :id(id), cancelled(false), leftFork(leftFork), rightFork(rightFork), table(table) {
    srand((unsigned) time(&t1));
}

void Philosopher::start() {
    // TODO: start a philosopher thread
    pthread_create(&t, NULL, run, this);
}

int Philosopher::join() {
    // TODO: join a philosopher thread
    return pthread_join(t, NULL);
}

int Philosopher::cancel() {
    // TODO: cancel a philosopher thread
    cancelled = true;
    return pthread_cancel(t);
}

void Philosopher::think() {
    int thinkTime = rand() % MAXTHINKTIME + MINTHINKTIME;
    sleep(thinkTime);
    printf("Philosopher %d is thinking for %d seconds.\n", id, thinkTime);
}

void Philosopher::eat() {
    printf("Philosopher %d is eating.\n", id);
    sleep(EATTIME);
}

void Philosopher::pickup(int id) {
    // TODO: implement the pickup interface, the philosopher needs to pick up the left fork first, then the right fork
    leftFork->wait();
    rightFork->wait();
    if(id!=(PHILOSOPHERS-1)){
        printf("Philosopher %d picked up left fork %d & right fork %d.\n", id, id, id+1);
    }else if(id==PHILOSOPHERS-1){
        printf("Philosopher %d picked up left fork %d & right fork %d.\n", id, id, 0);
    }
}

void Philosopher::putdown(int id) {
    // TODO: implement the putdown interface, the philosopher needs to put down the left fork first, then the right fork
    leftFork->signal();
    rightFork->signal();
    if(id!=(PHILOSOPHERS-1)){
        printf("Philosopher %d put down left fork %d & right fork %d.\n", id, id, id+1);
    }else if(id==PHILOSOPHERS-1){
        printf("Philosopher %d put down left fork %d & right fork %d.\n", id, id, 0);
    }
}

void Philosopher::enter() {
    // TODO: implement the enter interface, the philosopher needs to join the table first
    table->wait();
    printf("Philosopher %d entered the table.\n", id);
}

void Philosopher::leave() {
    // TODO: implement the leave interface, the philosopher needs to let the table know that he has left
    table->signal();
    printf("Philosopher %d left the table.\n", id);
}

void* Philosopher::run(void* arg) {
    // TODO: complete the philosopher thread routine. 
    Philosopher *p = (Philosopher*) arg;
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    while (!p->cancelled) {
        p->think();
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        p->enter();
        p->pickup(p->id);
        p->eat();
        p->putdown(p->id);
        p->leave();
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    }
    return NULL;
}