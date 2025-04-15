/*
NOME: Otávio Garcia Capobianco
NUSP: 15482671
EXERCÍCIO-PROGRAMA: EP2
*/

/* ===============================================================
   ========================== INCLUDES ===========================
   =============================================================== */
#define _GNU_SOURCE
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
#define FAIXAS 10
#define POS_VAZIA -1

/* ===============================================================
   =========================== STRUCTS ===========================
   =============================================================== */
typedef struct Ciclista {
    int pos_x;
    int pos_y;
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
char modo;
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
Stack* voltas_a_imprimir;

int** moveu;

int threads_aguardando = 0;

pthread_mutex_t lock_coordenador = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_ciclistas = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_coordenador = PTHREAD_COND_INITIALIZER;
// pthread_cond_t** cond_posicoes;
pthread_mutex_t** lock_posicoes;
pthread_mutex_t lock_pista = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock_quebra = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_pista = PTHREAD_COND_INITIALIZER;
// pthread_barrier_t barr_move;


/* ===============================================================
   ===================== FUNÇÕES CONCORRENTES ====================
   =============================================================== */

void move_ciclista(Ciclista* cic, int x, int y) {
    moveu[cic->pos_x][cic->pos_y] = 1;
    pista[cic->pos_x][cic->pos_y] = POS_VAZIA;
    pista[x][y] = cic->id;
    cic->pos_x = x;
}

void* ciclista(void* arg) {
    Ciclista* self = (Ciclista*)arg;
    int velocidade = 0;
    int recarga = 0;
    unsigned int seed = time(NULL) ^ (uintptr_t)pthread_self();
    int rng;
    int next_x;
    while (1) {
        next_x = (self->pos_x + 1) % metros;
        if ((recarga >= 1 && velocidade == 1) || (recarga >= 2 && velocidade == 0)) {
            if (modo == 'i') {
                pthread_mutex_lock(&lock_pista);
                if (pista[next_x][self->pos_y] == POS_VAZIA) {
                    move_ciclista(self, next_x, self->pos_y);
                } 
                else {
                    if (!moveu[next_x][self->pos_y]) {
                        pthread_cond_wait(&cond_pista, &lock_pista);
                    }
                    int y = self->pos_y;
                    while (y < FAIXAS && pista[next_x][y] != POS_VAZIA) {
                        if (pista[self->pos_x][y] == POS_VAZIA) {
                            y++;
                            continue;
                        }
                        if (!moveu[self->pos_x][y]) {
                            pthread_cond_wait(&cond_pista, &lock_pista);
                            if (pista[self->pos_x][y] != POS_VAZIA) break;
                        }
                    }

                    if (pista[next_x][y] == POS_VAZIA) {
                        move_ciclista(self, next_x, y);
                    }
                    else {
                        moveu[self->pos_x][self->pos_y] = 1;
                    }
                }
            }

            recarga = 0;
        }
        else {
            moveu[self->pos_x][self->pos_y] = 1;
            recarga++;
        }

        if (modo == 'i') {
            pthread_cond_broadcast(&cond_pista);
            pthread_mutex_unlock(&lock_pista);
        }

        // pthread_barrier (antes da segunda parte do movimento)

        moveu[self->pos_x][self->pos_y] = 0;
        if (modo == 'i') {
            int y = self->pos_y;
            pthread_mutex_lock(&lock_pista);
            while (y >= 0 && !(pista[self->pos_x][self->pos_y] != POS_VAZIA && moveu[self->pos_x][self->pos_y])) {
                if (pista[self->pos_x][y] == POS_VAZIA) {
                    y--;
                    continue;
                }
                if (!moveu[self->pos_x][y]) {
                    pthread_cond_wait(&cond_pista, &lock_pista);
                    if (pista[self->pos_x][y] != POS_VAZIA) break;
                }
            }
            pthread_cond_broadcast(&cond_pista);
            pthread_mutex_unlock(&lock_pista);
        }

        if (self->pos_x == 0 && next_x == self->pos_x) {
            acabaram_volta[self->volta]++;
            append(&ultimos_da_volta[self->volta], self);
            self->tempo_volta = tempo;
            self->volta++;

            rng = rand_r(&seed) % 100;
            if (velocidade) velocidade = rng < 45;
            else            velocidade = rng < 75;

            if (self->volta % 5 == 0) {
                rng = rand_r(&seed) % 100;
                pthread_mutex_lock(&lock_quebra);
                if (rng < 10) push(quebrados_temp, self);
                pthread_mutex_unlock(&lock_quebra);
            }
        }

        pthread_mutex_lock(&lock_coordenador);
        threads_aguardando++;
        if (threads_aguardando >= vivos) {
            pthread_cond_signal(&cond_coordenador);
        }
        while (threads_aguardando > 0) {
            pthread_cond_wait(&cond_ciclistas, &lock_coordenador);
        }
        pthread_mutex_unlock(&lock_coordenador);
    }

    return NULL;
}

/* ===============================================================
   ==================== FUNÇÕES COORDENADORAS ====================
   =============================================================== */
int volta_par = 2;
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
    for (int faixa = 0; faixa < FAIXAS; faixa++) {
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
void mostrar_informacoes() {
    int mostrar_pos = 0;
    while (!is_stack_empty(voltas_a_imprimir)) {
        mostrar_pos = 1;
        Ciclista* proximo = (Ciclista*)top(voltas_a_imprimir);
        printf("Ciclista %d acabou volta %d", proximo->id, proximo->volta);
        pop(voltas_a_imprimir);
        is_stack_empty(voltas_a_imprimir) ? printf("\n") : printf(" | ");
    }

    if (mostrar_pos) {
        for (int i = 0; i < num_ciclistas; i++) {
            if (!ciclistas[i]->esta_morto) {
                printf("Ciclista %d: %dm\n", i, ciclistas[i]->pos_x);
            }
        }
    }

    if (indice_quebrados < quebrados.size) printf("Ciclistas que quebraram: ");
    while (indice_quebrados < quebrados.size) {
        printf("Ciclista %d", ((Ciclista*)quebrados.data[indice_quebrados++])->id);
        indice_quebrados < quebrados.size ? printf("\n") : printf(" | ");
    }
}

// int indice_volta = 1;
// unsigned int indice_quebrados = 0;
// void mostrar_informacoes() {
//     while (acabaram_volta[indice_volta] >= vivos) {
//         printf("Volta: %d [t = %3fs]\n", indice_volta, tempo/1000.0);
//         for (int i = 0; i < num_ciclistas; i++) {
//             if (!ciclistas[i]->esta_morto) {
//                 printf("Ciclista %d: %dm\n", i, ciclistas[i]->pos_x);
//             }
//         }

//         if (indice_quebrados < quebrados.size) printf("Ciclistas que quebraram: ");
//         while (indice_quebrados < quebrados.size) printf("%d ", ((Ciclista*)quebrados.data[indice_quebrados++])->id);
//         printf("\n");

//         indice_volta++;
//     }
// }


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
        else       mostrar_informacoes();
    }

    for (int i = 0; i < num_ciclistas; i++) {
        if (!ciclistas[i]->esta_morto) {
            push(ranking, ciclistas[i]);
            pthread_cancel(ciclistas[i]->thread);
        }
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
    modo = argv[3][0];
    if (argc == 5) debug = !strcmp(argv[4], "-debug");

    vivos = num_ciclistas;
    ciclistas = malloc(num_ciclistas * sizeof(Ciclista*));
    for (int i = 0; i < num_ciclistas; i++) {
        ciclistas[i] = malloc(sizeof(Ciclista));
        ciclistas[i]->esta_morto = 0;
        ciclistas[i]->pos_x = 0;
        ciclistas[i]->volta = 0;
        ciclistas[i]->quebrou = 0;
        ciclistas[i]->id = i;
    }

    pista = malloc(metros * sizeof(int*));
    for (int i = 0; i < metros; i++) {
        pista[i] = malloc(FAIXAS * sizeof(int));
        for (int j = 0; j < FAIXAS; j++) {
            pista[i][j] = POS_VAZIA;
        }
    }

    moveu = malloc(metros * sizeof(int*));
    for (int i = 0; i < metros; i++) {
        moveu[i] = malloc(FAIXAS * sizeof(int));
        for (int j = 0; j < FAIXAS; j++) {
            moveu[i][j] = 0;
        }
    }
    

    init_array(&quebrados, 16);
    for (int i = 0; i < VOLTAS_MAX; i++) {
        init_array(&ultimos_da_volta[i], 4);
    }

    ranking = new_stack();
    voltas_a_imprimir = new_stack();

    coordenador();

    return 0;
}
