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

void copy_dynamic(DynamicArray* from, DynamicArray* to) {
    clear_array(to);
    for (int i = 0; i < from->size; i++) {
        append(to, from->data[i]);
    }
}

/*  ===============================================================
    =========================== DEFINES ===========================
    =============================================================== */
#define VOLTAS_MAX 25005
#define FAIXAS 10
#define POS_VAZIA -1
#define TEMPO_ESPERA 60000

/*  ===============================================================
    =========================== STRUCTS ===========================
    =============================================================== */
typedef struct Ciclista {
    int pos_x;
    int pos_y;
    int tempo_volta;
    int volta;
    int esta_morto;
    int id;
    pthread_t thread;
} Ciclista;

/*  ===============================================================
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
Stack* ultimos_da_volta[VOLTAS_MAX];
int ultima_alteracao_ultimos[VOLTAS_MAX];
DynamicArray ultimos_deste_turno[VOLTAS_MAX];
DynamicArray quebrados;
Stack* quebrados_temp;
Stack* ranking;
Stack* voltas_do_turno;
int todos_da_faixa[FAIXAS];

int prox_volta = 1;
Stack* acabaram_prox_volta;

int** moveu;

int threads_aguardando = 0;

pthread_mutex_t lock_coordenador = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_ciclistas = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_coordenador = PTHREAD_COND_INITIALIZER;
// pthread_cond_t** cond_posicoes;
pthread_mutex_t** lock_posicoes;
pthread_mutex_t lock_pista = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock_quebra = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock_volta = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_pista = PTHREAD_COND_INITIALIZER;
pthread_barrier_t barr_move;
pthread_mutex_t lock_faixa[FAIXAS];

/*  ===============================================================
    ===================== FUNÇÕES CONCORRENTES ====================
    =============================================================== */

void move_ciclista(Ciclista* cic, int x, int y) {
    moveu[cic->pos_x][cic->pos_y] = 1;
    moveu[x][y] = 1;
    pista[cic->pos_x][cic->pos_y] = POS_VAZIA;
    pista[x][y] = cic->id;
    cic->pos_x = x;
    cic->pos_y = y;
}

void* ciclista(void* arg) {
    Ciclista* self = (Ciclista*)arg;
    int velocidade = 0;
    int recarga = 0;
    unsigned int seed;
    int rng;
    int next_x;
    int vai_mover;
    int x_velho;
    int ja_moveu;

    pthread_mutex_lock(&lock_coordenador);
    pthread_mutex_unlock(&lock_coordenador);
    while (1) {
        ja_moveu = 0;
        vai_mover = velocidade == 1 || (recarga >= 1 && velocidade == 0);
        x_velho = self->pos_x;
        next_x = (self->pos_x + 1) % metros;
        
        pthread_mutex_lock(&lock_faixa[self->pos_y]);
        todos_da_faixa[self->pos_y] = todos_da_faixa[self->pos_y] && vai_mover;
        pthread_mutex_unlock(&lock_faixa[self->pos_y]);

        pthread_barrier_wait(&barr_move);

        if (modo == 'i') pthread_mutex_lock(&lock_pista);
        if (todos_da_faixa[self->pos_y]) {
            moveu[self->pos_x][self->pos_y] = 1;
            moveu[next_x][self->pos_y] = 1;
            pista[next_x][self->pos_y] = self->id;
            self->pos_x = next_x;
            ja_moveu = 1;
            recarga = 0;
        }
        if (modo == 'i') pthread_mutex_unlock(&lock_pista);

        pthread_barrier_wait(&barr_move);
        if (pista[x_velho][self->pos_y] == self->id && x_velho != self->pos_x) {
            pista[x_velho][self->pos_y] = POS_VAZIA;
        }
        pthread_barrier_wait(&barr_move);

        if (modo == 'i') pthread_mutex_lock(&lock_pista);
        if (vai_mover && !ja_moveu) {
            if (modo == 'i') {
                if (pista[next_x][self->pos_y] == POS_VAZIA) {
                    move_ciclista(self, next_x, self->pos_y);
                } 
                else {
                    while (!moveu[next_x][self->pos_y]) {
                        pthread_cond_wait(&cond_pista, &lock_pista);
                    }
                    int conseguiu = 0;
                    for (int y = self->pos_y; y < FAIXAS; y++) {
                        if (y != self->pos_y && pista[self->pos_x][y] != POS_VAZIA) {
                            while (!moveu[self->pos_x][y]) pthread_cond_wait(&cond_pista, &lock_pista);
                            if (pista[self->pos_x][y] != POS_VAZIA) break;
                        }
                        if (pista[next_x][y] == POS_VAZIA) {
                            move_ciclista(self, next_x, y);
                            conseguiu = 1;
                            break;
                        }
                    }
                    if (!conseguiu) moveu[self->pos_x][self->pos_y] = 1;
                }
            }

            recarga = 0;
        }
        else {
            moveu[self->pos_x][self->pos_y] = 1;
        }

        if (!vai_mover) recarga++;

        if (modo == 'i') {
            pthread_cond_broadcast(&cond_pista);
            pthread_mutex_unlock(&lock_pista);
        }

        pthread_barrier_wait(&barr_move);

        moveu[self->pos_x][self->pos_y] = 0;

        pthread_barrier_wait(&barr_move);

        if (modo == 'i') {
            pthread_mutex_lock(&lock_pista);
            int y = self->pos_y;
            while (y > 0 && !(pista[self->pos_x][y-1] != POS_VAZIA && moveu[self->pos_x][y-1])) {
                if (pista[self->pos_x][y-1] == POS_VAZIA) {
                    y--;
                    continue;
                }
                while (!moveu[self->pos_x][y-1]) pthread_cond_wait(&cond_pista, &lock_pista);
            }
            move_ciclista(self, self->pos_x, y);
            pthread_cond_broadcast(&cond_pista);
            pthread_mutex_unlock(&lock_pista);
        }

        if (self->pos_x == 0 && next_x == self->pos_x) {
            seed = time(NULL) ^ (unsigned int)pthread_self();
            int quebrou = 0;
            if (self->volta % 5 == 0) {
                rng = rand_r(&seed) % 100;
                if (rng < 10) {
                    pthread_mutex_lock(&lock_quebra);
                    push(quebrados_temp, self);
                    pthread_mutex_unlock(&lock_quebra);
                    quebrou = 1;
                }
            }

            pthread_mutex_lock(&lock_volta);
            if (self->volta == prox_volta) {
                push(acabaram_prox_volta, self);
            }
            if (!quebrou) {
                int volta = self->volta;
                acabaram_volta[volta]++;
                if (ultima_alteracao_ultimos[volta] != tempo / 60) {
                    clear_array(&ultimos_deste_turno[volta]);
                    ultima_alteracao_ultimos[volta] = tempo / 60;
                }
                append(&ultimos_deste_turno[volta], self);
                push(voltas_do_turno, &volta);
            }
            pthread_mutex_unlock(&lock_volta);

            self->tempo_volta = tempo;
            self->volta++;
            
            rng = rand_r(&seed) % 100;
            if (velocidade) velocidade = rng < 45;
            else            velocidade = rng < 75;
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

        if (self->esta_morto) break;
    }

    return NULL;
}

/*  ===============================================================
    ==================== FUNÇÕES COORDENADORAS ====================
    =============================================================== */

int volta_par = 2;
void destruir_ciclistas() {
    while (!is_stack_empty(quebrados_temp)) {
        Ciclista* cic = top(quebrados_temp);
        cic->esta_morto = 1;
        cic->volta--;
        pista[cic->pos_x][cic->pos_y] = POS_VAZIA;
        vivos--;
        append(&quebrados, cic);
        pop(quebrados_temp);
    }
    while (acabaram_volta[volta_par] >= vivos && vivos > 1) {
        DynamicArray* validos = malloc(sizeof(DynamicArray)); init_array(validos, 8);
        while (!is_stack_empty(ultimos_da_volta[volta_par])) {
            int valido = 0;
            DynamicArray* ultimos = (DynamicArray*)top(ultimos_da_volta[volta_par]);
            for (unsigned int i = 0; i < ultimos->size; i++) {
                if (!((Ciclista*)ultimos->data[i])->esta_morto) {
                    printf("achou\n");
                    valido = 1;
                    break;
                }
            }
            pop(ultimos_da_volta[volta_par]);
            if (!valido) {
                continue;
            }
            for (unsigned int i = 0; i < ultimos->size; i++) {
                if (!((Ciclista*)ultimos->data[i])->esta_morto) {
                    append(validos, (Ciclista*)ultimos->data[i]);
                }
            }
            break;
        }
        int escolhido = rand() % validos->size;
        Ciclista* cic = validos->data[escolhido];
        cic->esta_morto = 1;
        cic->volta--;
        pista[cic->pos_x][cic->pos_y] = POS_VAZIA;
        vivos--;
        push(ranking, cic);
        clear_array(validos);
        volta_par += 2;
    }
}

void adiciona_ultimos_volta() {
    while (!is_stack_empty(voltas_do_turno)) {
        int volta = *(int*)top(voltas_do_turno);
        if (volta < VOLTAS_MAX) {
            DynamicArray* copia = malloc(sizeof(DynamicArray));
            init_array(copia, 4);
            copy_dynamic(&ultimos_deste_turno[volta], copia);
            push(ultimos_da_volta[volta], copia);
        }
        pop(voltas_do_turno);
    }
}

void resultados_finais() {
    int indice = 1;
    printf("\nPlacar Final:\n");
    while (!is_stack_empty(ranking)) {
        Ciclista* proximo = top(ranking);
        printf("%do: Ciclista %d - Última volta: %d - Última chegada: %fs\n", indice, proximo->id, proximo->volta, proximo->tempo_volta/1000.0);
        pop(ranking);
        indice++;
    }
    if (quebrados.size == 0) printf("\nNenhum ciclista quebrou\n");
    else                     printf("\nQuebrados:\n");
    for (unsigned int i = 0; i < quebrados.size; i++) {
        printf("Ciclista %d - Última volta: %d\n", ((Ciclista*)quebrados.data[i])->id, ((Ciclista*)quebrados.data[i])->volta);
    }
}

int conta_espaco() {
    int res = 1;
    int temp = num_ciclistas;
    while (temp > 0) {
        res++;
        temp /= 10;
    }
    return res;
}
int espaco;
void mostrar_debug() {
    printf("\n");
    for (int faixa = FAIXAS-1; faixa >= 0; faixa--) {
        for (int i = metros - 1; i >= 0; i--) {
            if (pista[i][faixa] == POS_VAZIA)
                fprintf(stderr, "%*s", espaco, ".");
            else
                fprintf(stderr, "%*d", espaco, pista[i][faixa]);
        }
        printf("\n");
    }
}

unsigned int indice_quebrados = 0;
void mostrar_informacoes() {
    if (!is_stack_empty(acabaram_prox_volta) || indice_quebrados < quebrados.size)
        printf("\nTempo = %fs\n", tempo/1000.0);

    int alguem_acabou = 0;
    while (!is_stack_empty(acabaram_prox_volta)) {
        alguem_acabou = 1;
        Ciclista* proximo = (Ciclista*)top(acabaram_prox_volta);
        printf("Ciclista %d acabou volta %d", proximo->id, proximo->volta-1);
        pop(acabaram_prox_volta);
        is_stack_empty(acabaram_prox_volta) ? printf("\n") : printf(" | ");
    }

    if (alguem_acabou) {
        for (int i = 0; i < num_ciclistas; i++) {
            if (!ciclistas[i]->esta_morto) {
                printf("Ciclista %d: %dm\n", i, ciclistas[i]->pos_x);
            }
        }
        prox_volta++;
    }

    if (indice_quebrados < quebrados.size) printf("Ciclistas que quebraram: ");
    while (indice_quebrados < quebrados.size) {
        printf("Ciclista %d", ((Ciclista*)quebrados.data[indice_quebrados++])->id);
        indice_quebrados < quebrados.size ? printf(" | ") : printf("\n");
    }
}

void gerar_ordem_aleatoria(int* vetor, int n) {
    for (int i = 0; i < n; i++) {
        vetor[i] = i;
    }

    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = vetor[i];
        vetor[i] = vetor[j];
        vetor[j] = temp;
    }
}

void posicoes_iniciais() {
    int* ordem = malloc(num_ciclistas * sizeof(int));
    gerar_ordem_aleatoria(ordem, num_ciclistas);
    for (int i = 0; i < num_ciclistas; i++) {
        int x = ordem[i] / 5;
        int y = ordem[i] % 5;
        ciclistas[i]->pos_x = x;
        ciclistas[i]->pos_y = y;
        pista[x][y] = ciclistas[i]->id;
    }
}

void zera_vetores() {
    for (int i = 0; i < metros; i++) {
        for (int j = 0; j < FAIXAS; j++) {
            moveu[i][j] = 0;
        }
    }
    for (int i = 0; i < FAIXAS; i++) {
        todos_da_faixa[i] = 1;
    }
}


void coordenador() {
    espaco = conta_espaco();

    pthread_mutex_lock(&lock_coordenador);
    posicoes_iniciais();

    if (debug) mostrar_debug();
    
    for (int i = 0; i < num_ciclistas; i++) {
        pthread_t thread;
        ciclistas[i]->thread = thread;
        pthread_create(&thread, NULL, ciclista, ciclistas[i]);
    }

    pthread_barrier_init(&barr_move, NULL, vivos);
    while (vivos >= 2) {
        // printf("loop\n");
        pthread_barrier_destroy(&barr_move);
        pthread_barrier_init(&barr_move, NULL, vivos);
        
        zera_vetores();
        pthread_cond_broadcast(&cond_ciclistas);
        while (threads_aguardando < vivos) {
            pthread_cond_wait(&cond_coordenador, &lock_coordenador);
        }
        threads_aguardando = 0;

        adiciona_ultimos_volta();
        destruir_ciclistas();

        usleep(TEMPO_ESPERA);
        tempo += 60;

        if (debug) mostrar_debug();
        else       mostrar_informacoes();
    }

    pthread_mutex_unlock(&lock_coordenador);

    for (int i = 0; i < num_ciclistas; i++) {
        if (!ciclistas[i]->esta_morto) {
            push(ranking, ciclistas[i]);
            ciclistas[i]->esta_morto = 1;
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
        ciclistas[i]->volta = 1;
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
        ultimos_da_volta[i] = new_stack();
        init_array(&ultimos_deste_turno[i], 4);
    }

    for (int i = 0; i < FAIXAS; i++) {
        pthread_mutex_init(&lock_faixa[i], NULL);
    }

    ranking = new_stack();
    quebrados_temp = new_stack();
    voltas_do_turno = new_stack();
    acabaram_prox_volta = new_stack();

    coordenador();

    return 0;
}
