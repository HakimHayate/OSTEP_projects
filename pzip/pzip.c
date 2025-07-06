#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
	
#define LBUFSIZE 8192
#define EXTRA_SIZE 1024

int CPUs;
size_t S;

enum STATE {FREE, TAKEN, DONE, INACCESSIBLE};

typedef struct __node_t_ {
    char *buf_input;
    char *buf_output;
    int size_input;
    int size_output;
    struct __node_t *next;
    enum STATE state;
} node_t;

typedef struct __queue_t {
    int finish;
    node_t *head;
    node_t *tail;
    int size;
    pthread_cond_t cond_write;
    pthread_cond_t cond_compress, cond_prod;
    pthread_mutex_t tail_lock, head_lock;
} queue_t;

queue_t global_q;

void init_queue(queue_t *q) {
    q->size = 0;
    q->finish = 0;
    node_t *n = malloc(sizeof(node_t));
    n->state = INACCESSIBLE;
    n->next = NULL;
    q->head = q->tail = n;
    pthread_mutex_init(&q->head_lock, NULL);
    pthread_mutex_init(&q->tail_lock, NULL);
    pthread_cond_init(&q->cond_write, NULL);
    pthread_cond_init(&q->cond_compress, NULL);
    pthread_cond_init(&q->cond_prod, NULL);
}
void delete_head(queue_t *q) {
    node_t *temp = q->head;
    q->head = temp->next;
    free(temp->buf_input);
    free(temp->buf_output);
    free(temp);
    q->size--;
}

void write_out(node_t *n) {
       if (write(STDOUT_FILENO, n->buf_output, n->size_output) < 0) {
           perror("wrie");
           exit(1);
       }
}

void *writing(void *args) {
    queue_t *q = (queue_t *) args;
    while(1) {
        pthread_mutex_lock(&q->head_lock);
        node_t *n = q->head->next;
        while (!n || n->state != DONE) {
            if (q->finish && !q->size) {
                pthread_mutex_unlock(&q->head_lock);
                return NULL;
            }
            pthread_cond_wait(&q->cond_write, &q->head_lock);
            n = q->head->next;
        }
        
        write_out(n);
        delete_head(q); 
        pthread_cond_signal(&q->cond_prod);
        pthread_mutex_unlock(&q->head_lock);
    }
}



void compress_file(queue_t * q, node_t * node) {
    int offset = 0;
    char *buf = node->buf_input;
    char *temp = (char *)malloc(node->size_input);
    if (!temp) {
        perror("malloc");
        exit(1);
    }
    int occ = 1;
    char c = buf[0];
    for (int i=1; i<node->size_input; i++) {
        if (c != buf[i]){
            *(int *)(temp+offset) = occ;
            offset += sizeof(int);
            *(char *)(temp+offset) = c;
            offset += sizeof(char);
            c = buf[i];
            occ = 1;
        }
        else {
            occ++;
        }
    }
    *(int *)(temp+offset) = occ;
    offset += sizeof(int);
    *(char *)(temp+offset) = c;
    offset += sizeof(char);

    pthread_mutex_lock(&q->head_lock);
    node->buf_output = temp;
    node->size_output = offset;
    node->state = DONE;
    pthread_cond_signal(&q->cond_write);
    pthread_mutex_unlock(&q->head_lock);
}

void *compress_consumer(void *args) {
    queue_t *q = (queue_t *) args;
    while (1) {
        pthread_mutex_lock(&q->head_lock);
        node_t *n = NULL;
        while (!n || n->state != FREE) {
            n = q->head->next;
            while (n && n->state != FREE) {
                n = n->next;
            }
            if (q->finish && !n) { // if nothing coming in to queue and queue is empty we stop
                pthread_mutex_unlock(&q->head_lock);
                return NULL;
            }
            else if (n)
                break;
            pthread_cond_wait(&q->cond_compress, &q->head_lock);
        }
        n->state = TAKEN;
        pthread_mutex_unlock(&q->head_lock);
        compress_file(q, n);
    } 
}

typedef struct read_args {
    char **files;
    queue_t *q;
    int size_args;
}read_args;

void insert_queue(queue_t *q, char *buf, int size) {
    pthread_mutex_lock(&q->head_lock);
    while (q->size > S) {
        pthread_cond_wait(&q->cond_prod, &q->head_lock);
    }
    pthread_mutex_unlock(&q->head_lock);
    node_t *n = (node_t *)malloc(sizeof(node_t));
    if (!n) {
        perror("malloc");
        exit(1);
    }
    n->next = NULL;
    n->state = FREE;
    n->buf_input = buf;
    n->size_input = size;
    pthread_mutex_lock(&q->head_lock);
    q->tail->next = n;
    q->tail = n;
    q->size++;
    pthread_cond_signal(&q->cond_compress);
    pthread_mutex_unlock(&q->head_lock);
}

void *read_producer(void *arg) {
    read_args *args = (read_args *) arg;
    queue_t *q = args->q;
    char *lbuf = NULL;
    lbuf = (char *)malloc(LBUFSIZE + EXTRA_SIZE);
    int j; int k = 0;
    for (int i=1; i<args->size_args; i++) {
        int fd; 
        if ((fd = open(args->files[i], O_RDONLY)) < 0) {
            perror("fopen");
            exit(1);
        }
        struct stat st;
        if (fstat(fd, &st) < 0) {
            perror("stat");
            exit(1);
        }
        int size = st.st_size;

        char *ptr = (char *)mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (ptr == MAP_FAILED) {
            perror("map");
            exit(1);
        }
        close(fd);
        for (j = 0; j < size; j++) {
            if (k >= LBUFSIZE + EXTRA_SIZE) {
                fprintf(stderr, "buf full\n");
                exit(1);                                                        
            }
            if (k >= LBUFSIZE && ptr[j] != lbuf[k-1]) {
                insert_queue(q, lbuf, k);
                k = 0;
                lbuf = NULL;

                lbuf = (char *)malloc(LBUFSIZE + EXTRA_SIZE);
                if (!lbuf) {
                    perror("malloc");
                    exit(1);
                }

            }
            lbuf[k] = ptr[j];
            k++;
        }
        munmap(ptr, st.st_size);
    }
    if (k != 0) {
        insert_queue(q, lbuf, k); 
    }

    pthread_mutex_lock(&q->head_lock);
    q->finish = 1;
    pthread_cond_broadcast(&q->cond_compress);
    pthread_mutex_unlock(&q->head_lock);
    printf("reading thread ended\n");
    return NULL;
} 

int main (int argv, char **argc) {
	if (argv<2) {
        printf("wzip: file1 [file2 ...]\n");
		exit(1);
	}   
    CPUs = sysconf(_SC_NPROCESSORS_ONLN);
    S = 4 * CPUs;    
    read_args args;
    args.files = argc;
    args.size_args = argv;
    args.q = &global_q;

    init_queue(&global_q);
    
    pthread_t p[CPUs];
    
    pthread_create(&p[0], NULL, read_producer, (void *)&args); // Thread for reading from files
    
    for (int i = 1; i < CPUs - 1; i++) {
        pthread_create(&p[i], NULL, compress_consumer, (void *)&global_q);
    
    }
    
    pthread_create(&p[CPUs-1], NULL, writing, (void *)&global_q); 
    
    for (int i =0; i <CPUs ; i++) 
        pthread_join(p[i], NULL);
	
    return 0; 
}













