
/*
 * This program acts a server for clients to connect with in order to submit 
 * individual words to be checked for correct spelling. The program accepts a 
 * word from the client, then searches a dictionary for a match. If a match is 
 * found within the dictionary the submitted word is returned to the user along 
 * with the comment â€œcorrectâ€, or if no match is found â€œincorrectâ€. When the 
 * client would like to disconnect from the server they must type â€œ**â€ and press
 * enter before closing their program. 

 *Command line arg1:
 * This program may accept a custom dictionary at startup. It must be given in  
 * the form â€œname.txtâ€. If no filename is given a default dictionary will be used. 
 *Command line arg2:
 * This program may accept a custom port number at startup. It must be given as the
 * the 2nd command line argument. If no custom dictionary is required enter â€œ1â€ as 
 * command line argument 1, then enter your custom port number as argument 2.	
 */

/*The mustex code, including the struct and functions, is located withing the 
  sbuf.h header file.  */

#include "csapp.h"
//#include "sbuf.h"
#include <pthread.h>
//#include"try_struct.h"
#include "LockCV.h"

#define DEFAULT_DICTIONARY "dictionary.txt"
#define DEFAULT_PORT_STR "9999"
#define QUEUE_SIZE 5  //size of mutex queue
#define NTHREADS 2    //number of worker threads
#define MAX_WORD 50
#define DICT_LENGTH 256
#define WORD_LENGTH 50


char **dictionary = NULL;
int nwords = 0;
//sbuf_t work_queue;

void *worker_thread();
char **get_words(FILE *fp, int *n);


boundedBuffer workQ; //workQ is struct of type boundedBuffer
#define QSIZE 10

void print_buffer(boundedBuffer *sp);
void sbuf_init(boundedBuffer *sp, int n);
void sbuf_deinit(boundedBuffer *sp);
void put(boundedBuffer *sp, int item);
int get(boundedBuffer *sp);

int main(int argc, char** argv) {

    int i, listenfd, connfd, port;
    pthread_t thread_pool[NTHREADS];

    /* Initializes a mutex work_queue which ensures single thread modification.
       Insertion of client connected sockets performed by main thread.
       Removal of sockets for service performed by worker threads */
    //sbuf_init(&work_queue, QUEUE_SIZE);
    sbuf_init(&workQ,QSIZE);

    /* Fills thread_pool with threads used for servicing clients*/
    for (i = 0; i < NTHREADS; i++) {
        pthread_create(&thread_pool[i], NULL, worker_thread, NULL);
    }

    /* Checks for command line arguments.
     * Arg1: Accepts a custom dictionary text file.
     * Arg2: Accepts a custom port number.
     * If not args are detected default dictionary and port number are used. */
    if (argv[1] != NULL) {
        if (argv[1] == 1) {
            FILE *fp = fopen(DEFAULT_DICTIONARY, "r");
                dictionary = get_words(fp, &nwords);
    fclose(fp);
            if (argv[2] != NULL) {
                port = atoi(argv[2]);
            } else {
                port = atoi(DEFAULT_PORT_STR);
            }
        } else {
            FILE *fp = fopen(argv[1], "r");
                dictionary = get_words(fp, &nwords);
    fclose(fp);
            if (argv[2] != NULL) {
                port = atoi(argv[2]);
            } else {
                port = atoi(DEFAULT_PORT_STR);
            }
        }
    } else {
        FILE *fp = fopen(DEFAULT_DICTIONARY, "r");
        port = atoi(DEFAULT_PORT_STR);
            dictionary = get_words(fp, &nwords);
    fclose(fp);
    }

    printf("words in dictionary: %d\n", nwords);

    /* Waits for an incoming request for connection from client. */
    listenfd = open_listenfd(port);


    /* This loop will continue to run, creating connected sockets (connfd) 
       and inserting them into the work_queue, until one of two events causes
       it to pause.
        Event 1: There are no current requests for connection from clients, in
       which case the accept() fn cause the loop to pauses and wait for a req.
        Event 2: The work_queue is full, in which case this main thread will go
       to sleep until a slot open up and it is signaled to continue.*/
    while (1) {
        /* connfd is the connected socket, which acts as a pipe
           between the client and server for info to pass */
        connfd = accept(listenfd, NULL, NULL);
        printf("new client\n");

        /* Inserts the connfd into the work_queue.*/
        //sbuf_insert(&work_queue, connfd);
        put(&workQ, connfd);

        //print_buffer(&workQ);
    }
    return (EXIT_SUCCESS);
}

/*This function iterates through the dictionary, comparing each word
  against param input. Checking to make sure the length of param input is
  equal to an matched entries in the dictionary is necessary to ensure a input
  is not incorrectly marked as a match in the case that it is a group of letters
  contained within a larger word (ie input=appl would match with dic=apple).
  @param input: String to search the dictionary for.
  @param len: Length of input.
  @returns 0: when param input is found within the dictionary file AND
              param len equals the length of the dictionary entry.
  @returns 1: when param input is not found within the dictionary file.
 */
int check_spelling(char *input, int len) {
    int i;
    int strcmpV = 0;
    
    printf("dict[22] = %s ", dictionary[22]);
    int dictLen = strlen(dictionary[22]);
    printf("length = %d ", dictLen);
    
    printf("input = %s ", input);
    printf("length = %d ", len);

    for (i = 0; i < nwords; i++) {
        strcmpV = strncmp(input, dictionary[i], len - 1);
        if (strcmpV == 0 && (strlen(input) == strlen(dictionary[i]))) {
            return 0;
        }
    }
    return 1;
}

/* This is the function executed by newly created worker threads. It removes a
  connected socket from the mutex protected work_queue. It then receives words
  from the client and feeds them to the check_spelling() function. If a 0 is
  returned the word is returned to the client with " correct" attached, or if a
  1 is returned " incorrect" is attached. This will process will continue
  until the client submits the string "**", in which case the connection is
  broken and the worker thread running the function attempts to remove a new
  socket from work_queue. 
  The thread executing this function will go to sleep if the work_queue is empty
  and will be woken up when there is a socket to be serviced.*/
void *worker_thread() {
    pthread_detach(pthread_self());
    while (1) {
        while (1) {
            print_buffer(&workQ);
            int connfd = get(&workQ);        //sbuf_remove(&work_queue);

            while (1) {
                char buf[MAX_WORD];
                int x, bufLen, msg = 0;
                char exit[] = "Signing off";

                recv(connfd, buf, MAX_WORD, 0);
                bufLen = strlen(buf);

                /* When "**" is received from client the coekt is closed.*/
                x = strncmp("**", buf, bufLen - 1);
                if (x == 0) {
                    send(connfd, exit, sizeof (exit), 0);
                    printf("client has exited\n");
                    close(connfd);
                    break;
                }

                msg = check_spelling(buf, bufLen);
                char reply[bufLen + 12];
                buf[bufLen - 1] = '\0'; //strips '\n'

                /* Format the reply to client depending on the value
                   returned by spell_check()*/
                if (msg == 0) {
                    strcpy(reply, buf);
                    strcat(reply, " correct");
                } else {
                    strcpy(reply, buf);
                    strcat(reply, " incorrect");
                }

                send(connfd, reply, sizeof (reply), 0);
            }
            close(connfd);
        }
    }
}

/* This function opens a text file and creates a double pointer. Then on a loop,
 * imports a word from file, removes the "\n" character, adds the word to the
 * double pointer. If the word entries in the double pointer approach initial
 * memory capacity, more memory is allocated.
 * @param fp: Text file to import words from. 
 * *param n:   */
char **get_words(FILE *fp, int *n) {

    char **words = NULL;
    char buf[WORD_LENGTH + 1] = {0};
    int maxlen = DICT_LENGTH;

    (words = calloc(maxlen, sizeof *words));

    while (fgets(buf, WORD_LENGTH + 1, fp)) {
        /* get word length */
        size_t wordlen = strlen(buf);

        /* strip '\n' */
        if (buf[wordlen - 1] == '\n') {
            buf[--wordlen] = 0;
        }

        /* allocate/copy */
        words[(*n)++] = strdup(buf);

        /* realloc as required, update maxlen */
        if (*n == maxlen) {
            void *temp = realloc(words, maxlen * 2 * sizeof *words);
            words = temp;
            memset(words + maxlen, 0, maxlen * sizeof *words);
            maxlen *= 2;
        }
    }
    return words;
}

/*the following code was take from:
 Computer Systems: A Programmer's Perspective, 2/Ed
Randal E. Bryant and David R. O'Hallaron*/
int open_listenfd(int port) {
    int listenfd, optval = 1;
    struct sockaddr_in serveraddr;

    /* Create a socket descriptor */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;
    /* Eliminates "Address already in use" error from bind */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
            (const void *) &optval, sizeof (int)) < 0)
        return -1;

    /* Listenfd will be an end point for all requests to port
    on any IP address for this host */
    bzero((char *) &serveraddr, sizeof (serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short) port);
    if (bind(listenfd, (SA *) & serveraddr, sizeof (serveraddr)) < 0)
        return -1;

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, LISTENQ) < 0)
        return -1;
    return listenfd;
}

