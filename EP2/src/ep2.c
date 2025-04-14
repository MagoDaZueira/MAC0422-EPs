/*
NOME: Otávio Garcia Capobianco
NUSP: 15482671
EXERCÍCIO-PROGRAMA: EP2
*/

/* ===============================================================
   ========================== INCLUDES ===========================
   =============================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

/* ===============================================================
   ===================== IMPLEMENTAÇÃO QUEUE =====================
   =============================================================== */
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

typedef struct {
    Node *top;
    int size;
} Stack;

/* ===============================================================
   ===================== IMPLEMENTAÇÃO STACK =====================
   =============================================================== */
Stack* new_stack() {
    Stack *s = (Stack*)malloc(sizeof(Stack));
    s->top = NULL;
    s->size = 0;
    return s;
}

int is_stack_empty(Stack *s) {
    return s->top == NULL;
}

void push(Stack *s, void *data) {
    Node *new_node = (Node*)malloc(sizeof(Node));
    new_node->data = data;
    new_node->next = s->top;
    s->top = new_node;
    s->size++;
}

void pop(Stack *s) {
    if (is_stack_empty(s)) return;

    Node *temp = s->top;
    s->top = s->top->next;
    free(temp);
    s->size--;
}

void* top(Stack *s) {
    return s->top ? s->top->data : NULL;
}

/* ===============================================================
   ======================= ARRAY DINÂMICO ========================
   =============================================================== */
typedef struct {
    void **data;
    size_t size;
    size_t capacity;
} DynamicArray;

void init_array(DynamicArray *arr, size_t initialCapacity) {
    arr->data = malloc(initialCapacity * sizeof(void *));
    arr->size = 0;
    arr->capacity = initialCapacity;
}

void append(DynamicArray *arr, void *value) {
    if (arr->size == arr->capacity) {
        arr->capacity *= 2;
        arr->data = realloc(arr->data, arr->capacity * sizeof(void *));
    }
    arr->data[arr->size++] = value;
}

void clear_array(DynamicArray *arr) {
    arr->size = 0;
}

void clear_and_free_elements(DynamicArray *arr) {
    for (unsigned int i = 0; i < arr->size; i++) {
        free(arr->data[i]);
    }
    arr->size = 0;
}

void free_array(DynamicArray *arr) {
    free(arr->data);
    arr->data = NULL;
    arr->size = arr->capacity = 0;
}

/* ===============================================================
   =========================== DEFINES ===========================
   =============================================================== */
#define VOLTAS_MAX 25005
#define POS_VAZIA -1

/* ===============================================================
   =========================== STRUCTS ===========================
   =============================================================== */
typedef struct Ciclista {
    int posicao;
    int tempo_volta;
    int volta;
    int esta_morto;
    int quebrou;
    int id;
    pthread_t thread;
} Ciclista;

/* ===============================================================
   ====================== VARIÁVEIS GLOBAIS ======================
   =============================================================== */
int metros;
int num_ciclistas;
char mode;
int debug = 0;

long long tempo = 0;
int vivos;
int** pista;
Ciclista** ciclistas;
int acabaram_volta[VOLTAS_MAX];
DynamicArray ultimos_da_volta[VOLTAS_MAX];
DynamicArray quebrados;
Stack* quebrados_temp;
Stack* ranking;
int volta = 0;

int threads_aguardando = 0;

pthread_mutex_t lock_coordenador = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_ciclistas = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_coordenador = PTHREAD_COND_INITIALIZER;

int volta_par = 0;

/* ===============================================================
   ===================== FUNÇÕES CONCORRENTES ====================
   =============================================================== */
void* ciclista(void* arg) {
    Ciclista* self = (Ciclista*)arg;
    return NULL;
}

/* ===============================================================
   ==================== FUNÇÕES COORDENADORAS ====================
   =============================================================== */
void destruir_ciclistas() {
    while (acabaram_volta[volta_par] >= vivos) {
        int escolhido = rand() % acabaram_volta[volta_par];
        ciclistas[escolhido]->esta_morto = 1;
        vivos--;
        pthread_cancel(ciclistas[escolhido]->thread);
        volta_par += 2;
    }

    while (!is_stack_empty(quebrados_temp)) {
        Ciclista* proximo = top(quebrados_temp);
        proximo->esta_morto = 1;
        vivos--;
        pthread_cancel(proximo->thread);
        append(&quebrados, proximo);
        pop(quebrados_temp);
    }
}

void resultados_finais() {
    int indice = 1;
    while (!is_stack_empty(ranking)) {
        Ciclista* proximo = top(ranking);
        printf("%do: Ciclista %d - Última volta: %d - Última chegada: %3fs\n", indice, proximo->id, proximo->volta, proximo->tempo_volta/1000.0);
        pop(ranking);
    }
    if (quebrados.size == 0) printf("\nNenhum ciclista quebrou\n");
    else                     printf("\nQuebrados:\n");
    for (unsigned int i = 0; i < quebrados.size; i++) {
        printf("Ciclista %d - Última volta: %d\n", ((Ciclista*)quebrados.data[i])->id, ((Ciclista*)quebrados.data[i])->volta);
    }
}

void mostrar_debug() {
    for (int faixa = 0; faixa < 10; faixa++) {
        for (int i = metros - 1; i >= 0; i--) {
            if (pista[i][faixa] == POS_VAZIA)
                printf("%5s", ".");
            else
                printf("%5d", pista[i][faixa]);
        }
        printf("\n");
    }
}

unsigned int indice_quebrados = 0;
void mostrar_posicoes() {
    while (acabaram_volta[volta] >= vivos) {
        printf("Volta: %d [t = %3fs]\n", volta, tempo/1000.0);
        for (int i = 0; i < num_ciclistas; i++) {
            if (!ciclistas[i]->esta_morto) {
                printf("Ciclista %d: %dm\n", i, ciclistas[i]->posicao);
            }
        }

        if (indice_quebrados < quebrados.size) printf("Ciclistas que quebraram: ");
        while (indice_quebrados < quebrados.size) printf("%d ", ((Ciclista*)quebrados.data[indice_quebrados++])->id);
        printf("\n");

        volta++;
    }
}


void coordenador() {
    return;
    for (int i = 0; i < num_ciclistas; i++) {
        pthread_t thread;
        pthread_create(&thread, NULL, ciclista, ciclistas[i]);
    }
    while (vivos >= 2) {
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

    for (int i = 0; i < num_ciclistas; i++) {
        if (!ciclistas[i]->esta_morto) push(ranking, ciclistas[i]);
    }
    resultados_finais();
}

int main(int argc, char *argv[]) {
    if (argc < 4 || argc > 5) {
        printf("Uso correto: ep2 <metros> <ciclistas> <i|e (modo)> -debug(opcional)\n");
        return 1;
    }

    metros = atoi(argv[1]);
    num_ciclistas = atoi(argv[2]);
    mode = argv[3][0];
    if (argc == 5) debug = !strcmp(argv[4], "-debug");

    vivos = num_ciclistas;
    ciclistas = malloc(num_ciclistas * sizeof(Ciclista*));
    for (int i = 0; i < num_ciclistas; i++) {
        ciclistas[i] = malloc(sizeof(Ciclista));
        ciclistas[i]->esta_morto = 0;
        ciclistas[i]->posicao = 0;
        ciclistas[i]->volta = 0;
        ciclistas[i]->quebrou = 0;
        ciclistas[i]->id = i;
    }

    pista = malloc(metros * sizeof(int*));
    for (int i = 0; i < metros; i++) {
        pista[i] = malloc(10 * sizeof(int));
        for (int j = 0; j < 10; j++) {
            pista[i][j] = POS_VAZIA;
        }
    }
    

    init_array(&quebrados, 16);
    for (int i = 0; i < VOLTAS_MAX; i++) {
        init_array(&ultimos_da_volta[i], 4);
    }

    ranking = new_stack();

    coordenador();

    return 0;
}
