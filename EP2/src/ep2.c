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
   ===================== IMPLEMENTAÇÃO STACK =====================
   =============================================================== */
typedef struct Node {
    void *data;        // Elemento deste nó
    struct Node *next; // Ponteiro para o próximo nó
} Node;

typedef struct {
    Node *top; // Elemento do topo
    int size;  // Quantidade de elementos
} Stack;

// Inicializa a pilha
Stack* new_stack() {
    Stack *s = (Stack*)malloc(sizeof(Stack));
    s->top = NULL;
    s->size = 0;
    return s;
}

// Pilha vazia ou não
int is_stack_empty(Stack *s) {
    return s->top == NULL;
}

// Adiciona elemento ao topo da pilha
void push(Stack *s, void *data) {
    Node *new_node = (Node*)malloc(sizeof(Node));
    new_node->data = data;
    new_node->next = s->top;
    s->top = new_node;
    s->size++;
}

// Remove elemento do topo da pilha
void pop(Stack *s) {
    if (is_stack_empty(s)) return;

    Node *temp = s->top;
    s->top = s->top->next;
    free(temp);
    s->size--;
}

// Retorna o elemento do topo da pilha, caso exista
void* top(Stack *s) {
    return s->top ? s->top->data : NULL;
}

/* ===============================================================
   ======================= ARRAY DINÂMICO ========================
   =============================================================== */
typedef struct {
    void **data;     // Vetor com os elementos
    size_t size;     // Quantos elementos tem no vetor
    size_t capacity; // Quanto espaço está alocado para o vetor
} DynamicArray;

// Inicializa o vetor
void init_array(DynamicArray *arr, size_t initialCapacity) {
    arr->data = malloc(initialCapacity * sizeof(void *));
    arr->size = 0;
    arr->capacity = initialCapacity;
}

// Adiciona um elemento na última posição, realocando memória caso necessário
void append(DynamicArray *arr, void *value) {
    if (arr->size == arr->capacity) {
        arr->capacity *= 2;
        arr->data = realloc(arr->data, arr->capacity * sizeof(void *));
    }
    arr->data[arr->size++] = value;
}

// Deixa o vetor vazio (size determina até onde vão seus elementos)
void clear_array(DynamicArray *arr) {
    arr->size = 0;
}

// Deixa o vetor vazio, liberando a memória de seus elementos
void clear_and_free_elements(DynamicArray *arr) {
    for (unsigned int i = 0; i < arr->size; i++) {
        free(arr->data[i]);
    }
    arr->size = 0;
}

// Libera a memória ocupada pelo vetor
void free_array(DynamicArray *arr) {
    free(arr->data);
    arr->data = NULL;
    arr->size = arr->capacity = 0;
}

// Copia um vetor dinâmico para um outro, elemento por elemento
void copy_dynamic(DynamicArray* from, DynamicArray* to) {
    clear_array(to);
    for (int i = 0; i < from->size; i++) {
        append(to, from->data[i]);
    }
}

/*  ===============================================================
    =========================== DEFINES ===========================
    =============================================================== */
#define VOLTAS_MAX 25005 // Máximo de voltas relevantes à lógica de eliminação
#define FAIXAS 10        // Quantia de faixas da pista
#define POS_VAZIA -1     // Valor que indica uma posição sem ciclista

/*  ===============================================================
    =========================== STRUCTS ===========================
    =============================================================== */
typedef struct Ciclista {
    int pos_x;        // Metragem
    int pos_y;        // Faixa
    int tempo_volta;  // Tempo em que concluiu sua última volta
    int volta;        // Volta em que está
    int esta_morto;   // Eliminado ou não
    int id;           // Inteiro que o representa
    pthread_t thread; // Thread "ciclista" correspondente
} Ciclista;

/*  ===============================================================
    ====================== VARIÁVEIS GLOBAIS ======================
    =============================================================== */

// Valores dados por input
int metros;
int num_ciclistas;
char modo;
int debug = 0;

// Contadores
long long tempo = 0; // Incrementa de 1 em 1, cada incremento = 60ms
int vivos;
int prox_volta = 1; // Próxima a ser imprimida

// Matrizes de posições da pista
int** pista; // Quem está ocupando cada posição
int** moveu; // Indica se o ciclista de uma certa posição já fez o que ia fazer naquela etapa

// Vetor com todos os ciclistas
Ciclista** ciclistas;

// Vetores de gerenciamento de voltas finalizadas
int acabaram_volta[VOLTAS_MAX];               // Quantos acabaram uma dada volta
Stack* acabaram_prox_volta;                   // Ciclistas que acabaram a próxima volta a ser imprimida
Stack* ultimos_da_volta[VOLTAS_MAX];          // Os últimos ciclistas a acabarem uma dada volta (empates agrupados)
DynamicArray ultimos_deste_turno[VOLTAS_MAX]; // Agrupamento de ciclistas que terminaram uma dada volta neste tick do relógio 
int ultima_alteracao_ultimos[VOLTAS_MAX];     // Último instante de tempo em que o vetor anterior foi alterado

// Vetores de gerenciamento de impressão
DynamicArray quebrados; // Ciclistas que quebraram
Stack* quebrados_temp;  // Versão temporária mandada para o vetor acima
Stack* ranking;         // Ciclistas eliminados, do mais recente para o mais antigo
Stack* voltas_do_turno; // Voltas que foram completadas num dado tick do relógio

// Vetor que evita deadlock quando uma dada faixa está cheia
int qtde_faixa[FAIXAS]; // Quantos de uma faixa vão se mover neste turno

// Comunicação com a entidade central
int threads_aguardando = 0; // Quantos ciclistas finalizaram seu turno

// Mutexes
pthread_mutex_t lock_coord = PTHREAD_MUTEX_INITIALIZER;  // Comunicação com a entidade central
pthread_mutex_t lock_pista = PTHREAD_MUTEX_INITIALIZER;  // Abordagem ingênua, trava pista inteira
pthread_mutex_t** lock_posicoes;                         // Abordagem eficiente, trava uma única posição
pthread_mutex_t lock_quebra = PTHREAD_MUTEX_INITIALIZER; // Gerenciamento de ciclistas quebrados
pthread_mutex_t lock_volta = PTHREAD_MUTEX_INITIALIZER;  // Gerenciamento de voltas concluídas
pthread_mutex_t lock_faixa[FAIXAS];                      // Um mutex para cada faixa (etapa inicial, evita deadlock com a faixa inteira cheia)

// Conds
pthread_cond_t cond_ciclistas = PTHREAD_COND_INITIALIZER;   // Comunicação com a entidade central
pthread_cond_t cond_coordenador = PTHREAD_COND_INITIALIZER; // Comunicação com a entidade central
pthread_cond_t cond_pista = PTHREAD_COND_INITIALIZER;       // Abordagem ingênua, um cond para a pista inteira
pthread_cond_t** cond_posicoes;                             // Abordagem eficiente, um cond para cada posição

// Barreiras
pthread_barrier_t barr_move; // Separa cada etapa da movimentação dos ciclistas

/*  ===============================================================
    ===================== FUNÇÕES CONCORRENTES ====================
    =============================================================== */

// Move um ciclista para uma posição da pista (assume mutex trancado)
void move_ciclista(Ciclista* cic, int x, int y) {
    moveu[cic->pos_x][cic->pos_y] = 1;
    moveu[x][y] = 1;
    pista[cic->pos_x][cic->pos_y] = POS_VAZIA;
    pista[x][y] = cic->id;
    cic->pos_x = x;
    cic->pos_y = y;
}

// Thread principal dos ciclistas
void* ciclista(void* arg) {

    // Variáveis do ciclista
    Ciclista* self = (Ciclista*)arg; // Struct do ciclista
    int velocidade = 0;              // 0 => 30km/h ; 1 => 60km/h
    int recarga = 0;                 // Se velocidade = 0, recarga = 1 => mover
    unsigned int seed;               // Cada ciclista atualiza sua própria seed de rng
    int rng;                         // Guarda valores decididos aleatoriamente
    int next_x;                      // Próxima posição x na pista
    int vai_mover, conseguiu_mover;  // Controle sobre a movimentação
    int x_velho, y_velho;            // Posição no início do tick do relógio

    // Jeito gambiarrístico de esperar a entidade central fazer o setup incial
    pthread_mutex_lock(&lock_coord);
    pthread_mutex_unlock(&lock_coord);

    // Loop principal da thread
    while (!self->esta_morto) {

        // Prepara variáveis no início do turno
        conseguiu_mover = 0;
        vai_mover = velocidade == 1 || (recarga >= 1 && velocidade == 0);
        x_velho = self->pos_x;
        y_velho = self->pos_y;
        next_x = (self->pos_x + 1) % metros;

        // Conta quantos de uma dada faixa vão se mover
        if (vai_mover) {
            pthread_mutex_lock(&lock_faixa[self->pos_y]);
            qtde_faixa[self->pos_y]++;
            pthread_mutex_unlock(&lock_faixa[self->pos_y]);
        }
        
        pthread_barrier_wait(&barr_move);

        // Caso a faixa esteja cheia e todos dela se moverão, move para a frente.
        // Aqui, não travamos mutexes nem removemos o id da poisção anterior,
        // Pois sabemos que todos irão para a frente, e não haverão posições vazias
        if (qtde_faixa[self->pos_y] == metros) {
            moveu[self->pos_x][self->pos_y] = 1;
            moveu[next_x][self->pos_y] = 1;
            pista[next_x][self->pos_y] = self->id;
            self->pos_x = next_x;
            conseguiu_mover = 1;
        }

        pthread_barrier_wait(&barr_move);

        x_velho = self->pos_x;
        
        // Tranca a pista inteira na abordagem ingênua
        if (modo == 'i') pthread_mutex_lock(&lock_pista);
        
        // Caso valha, tentará ir para a frente
        if (vai_mover && !conseguiu_mover) {

            // Abordagem ingênua (pista já está trancada)
            if (modo == 'i') {
                // Para cada faixa mais externa disponível, vê se pode ir pra frente
                for (int y = self->pos_y; y < FAIXAS; y++) {

                    // Posição acima tem alguém
                    if (y != self->pos_y && pista[self->pos_x][y] != POS_VAZIA) {
                        // Espera o cara de cima finalizar seu turno
                        while (!moveu[self->pos_x][y]) pthread_cond_wait(&cond_pista, &lock_pista);
                        // Se ele ainda tá lá, é impossível ultrapassar
                        if (pista[self->pos_x][y] != POS_VAZIA) break;
                    }

                    // Caso naquele y a posição da frente esteja preenchida
                    if (pista[next_x][y] != POS_VAZIA) {
                        // Espera o cara da frente finalizar seu turno
                        while (!moveu[next_x][y]) pthread_cond_wait(&cond_pista, &lock_pista);
                    }

                    // Caso naquele y a posição da frente esteja vazia, se move para ela
                    if (pista[next_x][y] == POS_VAZIA) {
                        move_ciclista(self, next_x, y);
                        conseguiu_mover = 1;
                        break;
                    }

                    // Caso chegue aqui, não conseguiu ir para a frente, e tentará ultrapassar no próximo y
                }

                // Caso não tenha conseguido, marca que acabou esta etapa de seu movimento
                // Note que, quando ele consegue se mover, a função move_ciclista já marca isso
                if (!conseguiu_mover) moveu[self->pos_x][self->pos_y] = 1;
            }
            
            else {
                for (int y = self->pos_y; y < FAIXAS; y++) {
                    pthread_mutex_lock(&lock_posicoes[self->pos_x][y]);
                    if (y != self->pos_y && pista[self->pos_x][y] != POS_VAZIA) {
                        while (!moveu[self->pos_x][y]) pthread_cond_wait(&cond_posicoes[self->pos_x][y], &lock_posicoes[self->pos_x][y]);
                        if (pista[self->pos_x][y] != POS_VAZIA) {
                            pthread_mutex_unlock(&lock_posicoes[self->pos_x][y]);
                            break;
                        }
                    }
                    pthread_mutex_unlock(&lock_posicoes[self->pos_x][y]);
                    pthread_mutex_lock(&lock_posicoes[next_x][y]);
                    if (pista[next_x][y] != POS_VAZIA) {
                        while (!moveu[next_x][y]) pthread_cond_wait(&cond_posicoes[next_x][y], &lock_posicoes[next_x][y]);
                    }
                    if (pista[next_x][y] == POS_VAZIA) {
                        move_ciclista(self, next_x, y);
                        conseguiu_mover = 1;
                        pthread_mutex_unlock(&lock_posicoes[next_x][y]);
                        break;
                    }
                    pthread_mutex_unlock(&lock_posicoes[next_x][y]);
                }
                if (!conseguiu_mover) moveu[self->pos_x][self->pos_y] = 1;
            }
        }
        else {
            moveu[self->pos_x][self->pos_y] = 1;
        }
        
        if (conseguiu_mover) recarga = 0;
        else                 recarga++;
        
        if (modo == 'i') {
            pthread_cond_broadcast(&cond_pista);
            pthread_mutex_unlock(&lock_pista);
        }
        else {
            pthread_mutex_lock(&lock_posicoes[x_velho][y_velho]);
            pthread_cond_broadcast(&cond_posicoes[x_velho][y_velho]);
            pthread_mutex_unlock(&lock_posicoes[x_velho][y_velho]);
        }
        
        pthread_barrier_wait(&barr_move);

        moveu[self->pos_x][self->pos_y] = 0;
        
        pthread_barrier_wait(&barr_move);
        
        if (modo == 'i') {
            int y = self->pos_y;
            pthread_mutex_lock(&lock_pista);
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
        else {
            int y_temp = self->pos_y;
            int y = self->pos_y;
            if (y > 0) pthread_mutex_lock(&lock_posicoes[self->pos_x][y-1]);
            while (y > 0 && !(pista[self->pos_x][y-1] != POS_VAZIA && moveu[self->pos_x][y-1])) {
                if (pista[self->pos_x][y-1] == POS_VAZIA) {
                    y--;
                    pthread_mutex_unlock(&lock_posicoes[self->pos_x][y]);
                    if (y > 0) pthread_mutex_lock(&lock_posicoes[self->pos_x][y-1]);
                    continue;
                }
                while (!moveu[self->pos_x][y-1]) {
                    pthread_cond_wait(&cond_posicoes[self->pos_x][y-1], &lock_posicoes[self->pos_x][y-1]);
                }
            }
            pthread_mutex_lock(&lock_posicoes[self->pos_x][y_temp]);
            move_ciclista(self, self->pos_x, y);
            pthread_cond_broadcast(&cond_posicoes[self->pos_x][y_temp]);
            pthread_mutex_unlock(&lock_posicoes[self->pos_x][y_temp]);
            if (y > 0) pthread_mutex_unlock(&lock_posicoes[self->pos_x][y-1]);
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
            if (!quebrou && self->volta < VOLTAS_MAX) {
                int volta = self->volta;
                acabaram_volta[volta]++;
                if (ultima_alteracao_ultimos[volta] != tempo) {
                    clear_array(&ultimos_deste_turno[volta]);
                    ultima_alteracao_ultimos[volta] = tempo;
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

        pthread_mutex_lock(&lock_coord);
        threads_aguardando++;
        if (threads_aguardando >= vivos) {
            pthread_cond_signal(&cond_coordenador);
        }
        while (threads_aguardando > 0) {
            pthread_cond_wait(&cond_ciclistas, &lock_coord);
        }
        pthread_mutex_unlock(&lock_coord);
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
        printf("%do: Ciclista %d - Última volta: %d - Última chegada: %.2fs\n", indice, proximo->id, proximo->volta, (proximo->tempo_volta*60)/1000.0);
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
        printf("\nTempo = %.2fs\n", (tempo*60)/1000.0);

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
        qtde_faixa[i] = 0;
    }
}


void coordenador() {
    espaco = conta_espaco();

    pthread_mutex_lock(&lock_coord);
    posicoes_iniciais();

    if (debug) mostrar_debug();
    
    for (int i = 0; i < num_ciclistas; i++) {
        pthread_t thread;
        ciclistas[i]->thread = thread;
        pthread_create(&thread, NULL, ciclista, ciclistas[i]);
    }

    pthread_barrier_init(&barr_move, NULL, vivos);
    while (vivos >= 2) {
        pthread_barrier_destroy(&barr_move);
        pthread_barrier_init(&barr_move, NULL, vivos);
        
        zera_vetores();
        pthread_cond_broadcast(&cond_ciclistas);
        while (threads_aguardando < vivos) {
            pthread_cond_wait(&cond_coordenador, &lock_coord);
        }
        threads_aguardando = 0;

        adiciona_ultimos_volta();
        destruir_ciclistas();

        tempo++;

        if (debug) mostrar_debug();
        else       mostrar_informacoes();
    }

    pthread_mutex_unlock(&lock_coord);

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

    srand(time(NULL));

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

    lock_posicoes = malloc(metros * sizeof(pthread_mutex_t*));
    for (int i = 0; i < metros; i++) {
        lock_posicoes[i] = malloc(FAIXAS * sizeof(pthread_mutex_t));
        for (int j = 0; j < FAIXAS; j++) {
            pthread_mutex_init(&lock_posicoes[i][j], NULL);
        }
    }

    cond_posicoes = malloc(metros * sizeof(pthread_cond_t*));
    for (int i = 0; i < metros; i++) {
        cond_posicoes[i] = malloc(FAIXAS * sizeof(pthread_cond_t));
        for (int j = 0; j < FAIXAS; j++) {
            pthread_cond_init(&cond_posicoes[i][j], NULL);
        }
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
