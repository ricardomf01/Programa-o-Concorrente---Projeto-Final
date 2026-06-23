#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define CAPACIDADE_MESA 5   // Capacidade máxima da mesa
#define PORCOES_MAX 10      // Capacidade máxima do caldeirão
#define N_CANIBAIS1 9       // Quantidade de guerreiros na Vila 1
#define N_CANIBAIS2 8       // Quantidade de guerreiros na Vila 2


// ----- Variáveis Globais de controle de concorrência -----


// Mutex para garantir exclusão de grupos rivais na mesa
pthread_mutex_t mutex_mesa = PTHREAD_MUTEX_INITIALIZER;

// Catraca para anti-starvation
sem_t sem_catraca;

// Controle dos lugares na mesa + contador de lugares ocupados
sem_t sem_mesa; 
int cadeiras_ocupadas = 0;
pthread_mutex_t mutex_cont_cadeiras = PTHREAD_MUTEX_INITIALIZER;

// Lightswitch para a Vila 1
int vila1_cont = 0;
pthread_mutex_t mutex_vila1 = PTHREAD_MUTEX_INITIALIZER;

// Lightswitch para a Vila 2
int vila2_cont = 0;
pthread_mutex_t mutex_vila2 = PTHREAD_MUTEX_INITIALIZER;

// Lightswitch para os Caciques
int cacique_cont = 0;
pthread_mutex_t mutex_cacique = PTHREAD_MUTEX_INITIALIZER;

// Controles do Caldeirão e do Cozinheiro
int porcoes = PORCOES_MAX;
int esperando_comida = 0;
pthread_mutex_t mutex_caldeirao = PTHREAD_MUTEX_INITIALIZER;
sem_t sem_cozinheiro;
sem_t sem_comida_pronta;


// ----- Comportamento do Cozinheiro -----


void* cozinheiro(void* arg) {
    while(1) {
        sem_wait(&sem_cozinheiro); 
        
        pthread_mutex_lock(&mutex_caldeirao);
        // Apenas o cozinheiro acessa o caldeirão durante o preparo
        printf("[ - COZINHEIRO]: Acordou e está preparando a comida! ---\n");
        sleep(4); 

        porcoes = PORCOES_MAX; 
        printf("[ - COZINHEIRO]: Caldeirão cheio (%d porções)! Acordando famintos. ---\n", PORCOES_MAX);
        
        // Acorda exatamente o número de pessoas que estavam esperando
        for(int i = 0; i < esperando_comida; i++) {
            sem_post(&sem_comida_pronta);
        }
        esperando_comida = 0; // Zera a fila
        
        pthread_mutex_unlock(&mutex_caldeirao);
    }
    return NULL;
}


// ----- Comportamento dos Guerreiros -----
/*
As funções 'guerreiro_vila1' e 'guerreiro_vila2' são quase idênticas,
diferenciando-se apenas na identificação da vila e nas respectivas variáveis de controle.
*/


void* guerreiro_vila1(void* arg) {
    int id = *(int*)arg;
    while(1) {
        sleep(2); 

        // --- Entrada: garante exclusão entre grupos rivais (catraca + lightswitch) ---
        sem_wait(&sem_catraca); 

        pthread_mutex_lock(&mutex_vila1);
        vila1_cont++;
        if (vila1_cont == 1) { 
            pthread_mutex_lock(&mutex_mesa); 
        }
        pthread_mutex_unlock(&mutex_vila1);

        sem_post(&sem_catraca); 

        // --- ZONA DE ALIMENTAÇÃO ---
        sem_wait(&sem_mesa); 
        
        // Atualiza as cadeiras de forma segura ao sentar
        pthread_mutex_lock(&mutex_cont_cadeiras);
        cadeiras_ocupadas++;
        printf("[Vila 1 - Guerreiro %d] Sentou na mesa. Cadeiras livres: %d\n", id, CAPACIDADE_MESA - cadeiras_ocupadas);
        pthread_mutex_unlock(&mutex_cont_cadeiras);

        // Acessa o caldeirão para se servir
        pthread_mutex_lock(&mutex_caldeirao);
        while (porcoes == 0) {
            esperando_comida++;
            if (esperando_comida == 1) { 
                // Apenas o primeiro faminto bate na porta do cozinheiro
                printf("[Vila 1 - Guerreiro %d] Caldeirão vazio! Acordando cozinheiro...\n", id);
                sem_post(&sem_cozinheiro);
            }
            pthread_mutex_unlock(&mutex_caldeirao); 
            sem_wait(&sem_comida_pronta);           
            pthread_mutex_lock(&mutex_caldeirao);   
        }
        porcoes--;
        printf("[Vila 1 - Guerreiro %d] Se serviu. Porções restantes: %d\n", id, porcoes);
        pthread_mutex_unlock(&mutex_caldeirao);

        sleep(2); // Tempo comendo
        
        // Atualiza as cadeiras de forma segura ao levantar
        pthread_mutex_lock(&mutex_cont_cadeiras);
        cadeiras_ocupadas--;
        printf("[Vila 1 - Guerreiro %d] Terminou de comer e saiu da mesa. Cadeiras livres: %d\n", id, CAPACIDADE_MESA - cadeiras_ocupadas);
        //if (!cadeiras_ocupadas){printf("v^\n");} //print que indica a troca de grupo na mesa
        pthread_mutex_unlock(&mutex_cont_cadeiras);
        
        sem_post(&sem_mesa); 

        // --- Saída: gerência da exclusão mútua (apaga lightswitch) ---
        pthread_mutex_lock(&mutex_vila1);
        vila1_cont--;
        if (vila1_cont == 0) { 
            pthread_mutex_unlock(&mutex_mesa);
        }
        pthread_mutex_unlock(&mutex_vila1);
    }
    return NULL;
}

void* guerreiro_vila2(void* arg) {
    int id = *(int*)arg;
    while(1) {
        sleep(2); 

        // --- Entrada: garante exclusão entre grupos rivais (catraca + lightswitch) ---
        sem_wait(&sem_catraca); 

        pthread_mutex_lock(&mutex_vila2);
        vila2_cont++;
        if (vila2_cont == 1) { 
            pthread_mutex_lock(&mutex_mesa); 
        }
        pthread_mutex_unlock(&mutex_vila2);

        sem_post(&sem_catraca); 

        // --- ZONA DE ALIMENTAÇÃO ---
        sem_wait(&sem_mesa); 
        
        // Atualiza as cadeiras de forma segura ao sentar
        pthread_mutex_lock(&mutex_cont_cadeiras);
        cadeiras_ocupadas++;
        printf("[Vila 2 - Guerreiro %d] Sentou na mesa. Cadeiras livres: %d\n", id, CAPACIDADE_MESA - cadeiras_ocupadas);
        pthread_mutex_unlock(&mutex_cont_cadeiras);

        // Acessa o caldeirão para se servir
        pthread_mutex_lock(&mutex_caldeirao);
        while (porcoes == 0) {
            esperando_comida++;
            if (esperando_comida == 1) { 
                // Apenas o primeiro faminto bate na porta do cozinheiro
                printf("[Vila 2 - Guerreiro %d] Caldeirão vazio! Acordando cozinheiro...\n", id);
                sem_post(&sem_cozinheiro);
            }
            pthread_mutex_unlock(&mutex_caldeirao); 
            sem_wait(&sem_comida_pronta);           
            pthread_mutex_lock(&mutex_caldeirao);   
        }
        porcoes--;
        printf("[Vila 2 - Guerreiro %d] Se serviu. Porções restantes: %d\n", id, porcoes);
        pthread_mutex_unlock(&mutex_caldeirao);

        sleep(2); // Tempo comendo
        
        // Atualiza as cadeiras de forma segura ao levantar
        pthread_mutex_lock(&mutex_cont_cadeiras);
        cadeiras_ocupadas--;
        printf("[Vila 2 - Guerreiro %d] Terminou de comer e saiu da mesa. Cadeiras livres: %d\n", id, CAPACIDADE_MESA - cadeiras_ocupadas);
        //if (!cadeiras_ocupadas){printf("v^\n");} //print que indica a troca de grupo na mesa
        pthread_mutex_unlock(&mutex_cont_cadeiras);

        sem_post(&sem_mesa); 

        // --- Saída: gerência da exclusão mútua (apaga lightswitch) ---
        pthread_mutex_lock(&mutex_vila2);
        vila2_cont--;
        if (vila2_cont == 0) { 
            pthread_mutex_unlock(&mutex_mesa);
        }
        pthread_mutex_unlock(&mutex_vila2);
    }
    return NULL;
}

// ----- Comportamento dos Caciques -----
/*
A função 'cacique' também é quase idêntica às funções dos guerreiros,
diferenciando-se apenas nas respectivas variáveis de controle.
Na prática eles atuam como uma terceira vila de apenas 2 indivíduos pois
eles não podem comer junto dos outros guerreiros mas podem dividir a mesa entre si.
*/

void* cacique(void* arg) {
    int id = *(int*)arg; 
    while(1) {
        sleep(3); 

        // --- Entrada: garante exclusão entre grupos rivais (catraca + lightswitch) ---
        sem_wait(&sem_catraca); 

        pthread_mutex_lock(&mutex_cacique);
        cacique_cont++;
        if (cacique_cont == 1) { 
            pthread_mutex_lock(&mutex_mesa); 
        }
        pthread_mutex_unlock(&mutex_cacique);

        sem_post(&sem_catraca); 

        // --- ZONA DE ALIMENTAÇÃO ---
        sem_wait(&sem_mesa); 
        
        // Atualiza as cadeiras de forma segura ao sentar
        pthread_mutex_lock(&mutex_cont_cadeiras);
        cadeiras_ocupadas++;
        printf("[>> CACIQUE %d] Sentou na mesa. Cadeiras livres: %d <<\n", id, CAPACIDADE_MESA - cadeiras_ocupadas);
        pthread_mutex_unlock(&mutex_cont_cadeiras);

        // Acessa o caldeirão para se servir
        pthread_mutex_lock(&mutex_caldeirao);
        while (porcoes == 0) {
            esperando_comida++;
            if (esperando_comida == 1) { 
                // Apenas o primeiro faminto bate na porta do cozinheiro
                printf("[>> CACIQUE %d] Caldeirão vazio! Acordando cozinheiro...\n", id);
                sem_post(&sem_cozinheiro);
            }
            pthread_mutex_unlock(&mutex_caldeirao); 
            sem_wait(&sem_comida_pronta);           
            pthread_mutex_lock(&mutex_caldeirao);   
        }
        porcoes--;
        printf("[>> CACIQUE %d] Se serviu. Porções restantes: %d <<\n", id, porcoes);
        pthread_mutex_unlock(&mutex_caldeirao);

        sleep(3); // Tempo comendo
        
        // Atualiza as cadeiras de forma segura ao levantar
        pthread_mutex_lock(&mutex_cont_cadeiras);
        cadeiras_ocupadas--;
        printf("[>> CACIQUE %d] Terminou de comer e saiu da mesa. Cadeiras livres: %d <<\n", id, CAPACIDADE_MESA - cadeiras_ocupadas);
        //if (!cadeiras_ocupadas){printf("v^\n");} //print que indica a troca de grupo na mesa
        pthread_mutex_unlock(&mutex_cont_cadeiras);
        
        sem_post(&sem_mesa); 

        // --- Saída: gerência da exclusão mútua (apaga lightswitch) ---
        pthread_mutex_lock(&mutex_cacique);
        cacique_cont--;
        if (cacique_cont == 0) { 
            pthread_mutex_unlock(&mutex_mesa);
        }
        pthread_mutex_unlock(&mutex_cacique);
    }
    return NULL;
}

// --------------------------------------------------------
//  Função main: inicializa os semáforos e cria as threads
// --------------------------------------------------------
int main() {
    srand(time(NULL));

    // Inicialização dos Semáforos
    sem_init(&sem_catraca, 0, 1);     
    sem_init(&sem_mesa, 0, CAPACIDADE_MESA);        
    sem_init(&sem_cozinheiro, 0, 0);  
    sem_init(&sem_comida_pronta, 0, 0); 

    pthread_t thread_cozinheiro;
    pthread_t threads_vila1[N_CANIBAIS1];
    pthread_t threads_vila2[N_CANIBAIS2];
    pthread_t threads_caciques[2];

    int ids_vila1[N_CANIBAIS1], ids_vila2[N_CANIBAIS2], ids_caciques[2];

    // Criação das threads
    pthread_create(&thread_cozinheiro, NULL, cozinheiro, NULL);

    for (int i = 0; i < 2; i++) {
        ids_caciques[i] = i + 1;
        pthread_create(&threads_caciques[i], NULL, cacique, &ids_caciques[i]);
    }

    for (int i = 0; i < N_CANIBAIS1; i++) {
        ids_vila1[i] = i + 1;
        pthread_create(&threads_vila1[i], NULL, guerreiro_vila1, &ids_vila1[i]);
    }
    
    for (int i = 0; i < N_CANIBAIS2; i++) {
        ids_vila2[i] = i + 1;
        pthread_create(&threads_vila2[i], NULL, guerreiro_vila2, &ids_vila2[i]);
    }


    // Aguardando as threads
    for (int i = 0; i < N_CANIBAIS1; i++) {
        pthread_join(threads_vila1[i], NULL);
    }

    for (int i = 0; i < N_CANIBAIS2; i++) {
        pthread_join(threads_vila2[i], NULL);
    }

    pthread_join(threads_caciques[0], NULL);
    pthread_join(threads_caciques[1], NULL);
    pthread_join(thread_cozinheiro, NULL);

    return 0;
}