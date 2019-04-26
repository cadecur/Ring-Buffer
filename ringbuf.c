#include <pthread.h>
#include <stdio.h>
#include <time.h>
// Authors: Cade Curry & Trevor Walker

//Initializing stuff
#define SIZE 10 //size of ring buffer
pthread_t producer, consumer; //threads
pthread_mutex_t lock; //mutex (lock from using shared vars [i.e. message])
pthread_cond_t key = PTHREAD_COND_INITIALIZER; //cond_t, both threads will use

typedef struct
{
    int value; /* Value to be passed to consumer */
    int consumer_sleep; /* Time (in ms) for consumer to sleep */
    int line; /* Line number in input file */
    int print_code; /* Output code; see below */
    int quit; /* Nonzero (NZ) if consumer should exit */
} message;

int lineNum = 0; //initial line number (producer)
int finalSum = 0; //finalSum (consumer)
int unreadMessages = 0; //# of unread messages in ring

//this is the ring buffer
message ring[SIZE];

message* readNext = ring; //pointer to ring[i], where i is producer loc
message* writeNext = ring; //pointer to ring[i], where i is consumer loc
message* head = ring;
message* tail = &(ring[SIZE-1]);


//sleepHelper takes in a number of ms, then calls nanosleep 
///to sleep for that amount of time.
void sleepHelper(int ms)
{
    struct timespec timeStruct;
    timeStruct.tv_sec = ms / 1000;
    timeStruct.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&timeStruct, NULL);
}


void *produce(){
    int val,cSleep, pSleep, pCode;
    while(1){
        pthread_mutex_lock(&lock);
        if(unreadMessages == SIZE){ //if ring full, wait for signal
            pthread_cond_wait(&key, &lock);
        }

        ++lineNum; //increment line
        int result = scanf("%d %d %d %d", &val, &pSleep, &cSleep, &pCode);

        if(result == EOF){ //if last line, nonzero quit code
            readNext->quit = 1;
            unreadMessages = unreadMessages + 1; //increment unreadMessages
            break;
        }
        else{ //if not last line
            pthread_mutex_unlock(&lock);
            sleepHelper(pSleep); //sleep before placing message in ring
            pthread_mutex_lock(&lock);
            
            //populate current message's instance vars
            readNext->line = lineNum;
            readNext->value = val;
            readNext->consumer_sleep = cSleep;
            readNext->print_code = pCode;

            if (readNext == tail){
                readNext = head; //wrap pointer
            }
            else {
                readNext = (readNext + 1); //increment our pointer
            }
            unreadMessages = unreadMessages + 1; //increment unreadMessages

            pthread_mutex_unlock(&lock);
            pthread_cond_signal(&key);
            if (pCode == 1 || pCode == 3){
                //producer status message
                printf("Produced %d from input line %d\n", val, lineNum);
            }
        }

    }

    pthread_mutex_unlock(&lock);
    pthread_cond_signal(&key);
    return NULL;

}


void *consume(){

    while(1){

        pthread_mutex_lock(&lock); //locks mutex
        if (unreadMessages == 0){ //if ring empty
            pthread_cond_wait(&key, &lock); //waits for signal from producer
        }

        message temp; //create temp message
        temp = *writeNext; //copy over struct instance variables
        pthread_mutex_unlock(&lock); //unlocks mutex
        pthread_cond_signal(&key); //tells producer it can work

        if (temp.quit != 0){ //if nonzero quit code
            printf("Final sum is %d\n", finalSum); //we are done!
            return NULL;
        }

        int pCode = temp.print_code;
        int val = temp.value;
        int napTime = temp.consumer_sleep;
        int lineNum = temp.line;

        if (writeNext == tail){
            writeNext = head; //wrap pointer
        }
        else {
            writeNext = (writeNext + 1); //increment our pointer
        }

        unreadMessages = unreadMessages - 1; //mark as read

        //sleep for specified amount of time
        sleepHelper(napTime);
        
        //add to sum
        finalSum = finalSum + val;

        //print message if 2or3 print code
        if (pCode == 2 || pCode == 3){
            printf("Consumed %d from input line %d; sum = %d\n", val, 
                                            lineNum, finalSum);
        }
    }

    return NULL;

}


int main()
{

    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }
    
    pthread_create(&producer, NULL, produce, NULL);
    pthread_create(&consumer, NULL, consume, NULL);

    pthread_join(producer, NULL);
    pthread_join(consumer, NULL);

    pthread_mutex_destroy(&lock);

    return 0;
}
