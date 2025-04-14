/*
NOME: Otávio Garcia Capobianco
NUSP: 15482671
EXERCÍCIO-PROGRAMA: EP2
*/

/* =========================== INCLUDES =========================== */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

/* ===================== IMPLEMENTAÇÃO QUEUE ===================== */
typedef struct Node {
    void *data;
    struct Node *next;
} Node;

typedef struct {
    Node *front;
    Node *back;
    int size;
} Queue;

Queue* new_queue() {
    Queue *q = (Queue*)malloc(sizeof(Queue));
    q->front = q->back = NULL;
    q->size = 0;
    return q;
}

int is_queue_empty(Queue *q) {
    return q->front == NULL;
}

void enqueue(Queue *q, void *data) {
    Node *new_node = (Node*)malloc(sizeof(Node));
    new_node->data = data;
    new_node->next = NULL;

    if (is_queue_empty(q)) {
        q->front = new_node;
    } else {
        q->back->next = new_node;
    }
    q->back = new_node;
    q->size++;
}

void dequeue(Queue *q) {
    if (is_queue_empty(q)) return;

    Node* temp = q->front;
    q->front = q->front->next;

    if (q->front == NULL) {
        q->back = NULL;
    }

    free(temp);
    q->size--;
}

void* front(Queue *q) {
    return q->front ? q->front->data : NULL;
}

/* ====================== VARIÁVEIS GLOBAIS ====================== */
int metros;
int ciclistas;
char mode;
int debug = 0;

long long tempo = 0;
int vivos;
int** pista;
int* posicoes;
int* esta_morto;
int acabaram_volta[25005];
int volta = 0;

int threads_aguardando = 0;

pthread_mutex_t lock_coordenador = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_ciclistas = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_coordenador = PTHREAD_COND_INITIALIZER;

void destruir_ciclistas() {
    return;
}

void resultados_finais() {
    return;
}

void mostrar_debug() {
    return;
}

void mostrar_posicoes() {
    while (acabaram_volta[volta] >= vivos) {
        printf("Volta: %d [t = %d]\n", volta, tempo);
        for (int i = 0; i < ciclistas; i++) {
            if (!esta_morto[i]) {
                printf("Ciclista %d: %dm\n", i, posicoes[i]);
            }
        }
        volta++;
    }
}

void coordenador() {
    while (ciclistas >= 2) {
        pthread_mutex_lock(&lock_coordenador);
        pthread_cond_broadcast(&cond_ciclistas);
        while (threads_aguardando < vivos) {
            pthread_cond_wait(&cond_coordenador, &lock_coordenador);
        }
        threads_aguardando = 0;
        pthread_mutex_unlock(&lock_coordenador);

        destruir_ciclistas();

        usleep(60);
        tempo += 60;

        if (debug) mostrar_debug();
        else       mostrar_posicoes();
    }

    resultados_finais();
}

int main(int argc, char *argv[]) {
    if (argc < 4 || argc > 5) {
        printf("Uso correto: ep2 <metros> <ciclistas> <i|e (modo)> -debug(opcional)\n");
        return 1;
    }

    metros = atoi(argv[1]);
    ciclistas = atoi(argv[2]);
    mode = argv[3];
    if (argc == 5) debug = !strcmp(argv[4], "-debug");

    vivos = ciclistas;
    
    esta_morto = malloc(ciclistas * sizeof(int));
    pista = malloc(metros * sizeof(int*));
    for (int i = 0; i < metros; i++) {
        pista[i] = malloc(10 * sizeof(int));
    }

    coordenador();

    return 0;
}
