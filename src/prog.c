#define _POSIX_C_SOURCE  200112L
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/types.h>
#include <utils.h>

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t v = PTHREAD_COND_INITIALIZER;
pthread_cond_t d = PTHREAD_COND_INITIALIZER;

static int K; //numero di casse
static int C; //numero clienti
static int E; //C-E numero dopo il quale possono rientrare nuovi clienti
static int T; //massimo range da 10 a T, tempo nel supermercato cliente
static int P; //max num prodotti per cliente, min = 0
static int S;//periodo scansione direttore
static int TOT; //numero entrati clienti nel supermercato
static int CURRENT; //numero clienti nel supermercato
static int S1;//minimo clienti in coda
static int S2;//massimo clienti in coda
static int INIT; //numero casse aperte all'inizio
static int CLOSE; //chiusura supermercato
static int CASSE; //casse coinvolte (possono essere anche chiuse)

cassa* p;
cliente* p_dir;

//legge il file config.txt e assegna i valori ad un array
void lettura(int* A) {
	FILE *fd=NULL;
	int j = 0;
	char* buffer;
	if ((buffer=malloc(MAX_LEN*sizeof(char))) == NULL) {
		perror("malloc buffer"); 
		exit(EXIT_FAILURE);      
    	}
	
	if ((fd=fopen(CONF, "r")) == NULL) {
		perror("opening file");
		exit(EXIT_FAILURE);
		
    	}
  
    	while(fgets(buffer, MAX_LEN, fd) != NULL) {
		char* value;  
		if((value=strchr(buffer, '=')) == NULL) {
			perror("reading file");
			exit(EXIT_FAILURE);
		}
		value++;
		A[j] = atoi(value);
		j++;
	}
	free(buffer);
	fclose(fd);
	return;
}

//assegna i valori dell' array alle variabili globali
void assegnamento(int* A) {
	int i = 0;
	K=A[i]; i++;
	C=A[i]; i++;
	E=A[i]; i++;
	T=A[i]; i++;
	P=A[i]; i++;
	S=A[i]; i++;
	S1=A[i]; i++;
	S2=A[i]; i++;
	INIT=A[i]; i++;
	return;
}

//thread_clienti: leggere documentazione per info
void* thread_clienti(void* arg) {
	
	//unsigned int seed = time(NULL);
	cliente* new = NULL;
	new = malloc(sizeof(cliente));
	new->id=*((int*)arg);
	new->n_prod = (rand() % P) + 1;
	new->change_tail=0;
	new->time_tail=0;
	new->fast_c.tv_sec=0;
	new->fast_c.tv_nsec = ((rand() % (T - 10)) + 10) * 1000000;
	nanosleep(&new->fast_c, NULL);
	new->time_total=new->fast_c.tv_nsec;
	new->next=NULL;
	pthread_mutex_lock(&lock);
	if(new->n_prod == 0) {
		if(!add_dir(new)) {
			perror("errore aggiunta c dir");
			exit(EXIT_FAILURE);
		}
		CURRENT++;
		
	}
	else if(!select_cassa(new, K)) {
		perror("errore selezione cassa");
		 exit(EXIT_FAILURE);
	}
	
	pthread_mutex_unlock(&lock);
	
	return (void*) 17;
}

//thread direttore: leggere documentazione per info
void* thread_direttore(void* arg) {
	
	struct timespec tm;
	tm.tv_sec=0;
	tm.tv_nsec=S*1100000;
	
	while(!CLOSE || CURRENT < TOT) {
		nanosleep(&tm, NULL);
		//controlla clienti con 0 prod
		pthread_mutex_lock(&lock);
		if(!analisi_clienti()) {
			perror("errore analisi clienti");
			pthread_exit(NULL);
		}
		if(!analisi_cassa(S1, S2, K)) {
			perror("errore analisi cassa");
			pthread_exit(NULL);
		}
		pthread_mutex_unlock(&lock);
		if(CURRENT > TOT - E && !CLOSE) {
			pthread_cond_signal(&v);
		}
		
	}
	
	pthread_cond_signal(&v);
	pthread_mutex_lock(&lock);
	while(CASSE!=0) {
		pthread_cond_wait(&d, &lock);
	}
	pthread_mutex_unlock(&lock);
	casse_off();
	CASSE--;
	pthread_cond_signal(&v);
		
	return (void*)11;
	
}
//get_tail è un thread di supporto al thread cassa con lo scopo di
//aggiornare il numero di clienti in coda
void* get_tail(void* arg) {

	int n = *((int*)arg);
	struct timespec intv;
	intv.tv_sec=0;
	intv.tv_nsec=S*1000000;
	cassa* tmp = p;
	while(tmp->id!=n) {
		tmp = tmp -> next;
	}
	
	while(CURRENT < TOT || CLOSE==0) { 
		nanosleep(&intv, NULL);
		pthread_mutex_lock(&lock);
		if(!tmp->stato) {
			pthread_cond_wait(&tmp->V, &lock);
		}
		pthread_mutex_unlock(&lock);
		tmp->tail=tail(tmp);
	}
	
	return (void*) 17;
}

//paga_spesa prende una cassa come parametro e effettua il pagamento del primo cliente
//sulla coda. I dati relativi alla cassa e al cliente vengono aggiornati. Alla fine della procedura,
//vengono scritti sul file di log i dati relativi al cliente.
int paga_spesa(cassa *t) {

	if(t == NULL) {
		return 0;
	}
	if(t->cl == NULL) {
		return 0;
	}
	t->num_clienti++;
	cliente* tmp=NULL;
	tmp = t->cl;
	tmp->fast_c.tv_nsec = (tmp->n_prod) * (t->fast.tv_nsec);
	nanosleep(&tmp->fast_c, NULL); //durata pagamento spesa
	t->time_total+=tmp->fast_c.tv_nsec;//aumento tempo totale pag cassa, DA GUARDARE
	t->prod_total+=tmp->n_prod;
	cliente* scan = NULL;
	if( t->cl->next != NULL) {
		scan = t->cl->next;
	}
	//aumento tempo di coda clienti in cassa
	while(scan!=NULL) {
		scan->time_tail+=tmp->fast_c.tv_nsec;
		scan = scan->next;
	}
	//tempo totale cliente
	tmp->time_total+=(tmp->fast_c.tv_nsec + tmp->time_tail);
	//eventuali scrittute su log
	scrittura_cl(tmp);
	t->cl = t->cl->next;
	CURRENT++;
	free(tmp);
	return 1;
}

//thread_cassa: per maggiori info vedere documentazione
void* thread_cassa(void* arg) {


	//unsigned int seed = time(NULL);
	int n = *((int*)arg);
	cassa* new = NULL;
	new = malloc(1*sizeof(cassa));
	new->id=n;
	new->fast.tv_sec=0;
	new->fast.tv_nsec = ((rand() % (60)) + 20) * 1000000;
	if(INIT > n ) {
		new->stato = 1;
		
	}
	else {
		new->stato=0;
		
	}
	new->tail=0;
	new->num_clienti = 0;
	new->time_total = 0;
	new->num_off = 0;
	new->media_time=0;
	new->prod_total=0;
	
	pthread_cond_init(&new->V, NULL);
	new->next=NULL;
	new->cl = NULL;
	pthread_mutex_lock(&lock);
	if(!inserisci_cassa(new)) {
		perror("errore inserimento cassa");
		pthread_exit(NULL);
	}
	CASSE++;
	if(CASSE == K) {
		pthread_cond_broadcast(&v);
	}
	pthread_mutex_unlock(&lock);
	printf("nuova cassa\n");
	pthread_t support;
	if (pthread_create(&support, NULL, get_tail, &n) != 0) {
		
	    fprintf(stderr, "pthread_create failed\n");
	    exit(EXIT_FAILURE);
	}
		
	
	while(CURRENT < TOT || CLOSE==0) {
	
		pthread_mutex_lock(&lock);
		while(new->cl == NULL && CLOSE == 0) {
			pthread_cond_wait(&new->V, &lock);
		}
		if(new->cl!=NULL) {
			paga_spesa(new);
		}
		pthread_mutex_unlock(&lock);
		
	}
	scrittura_ca(new);
	if (pthread_join(support, NULL) == -1) {
		    fprintf(stderr, "pthread_join failed\n");
	}
	
	casse_lll();
	
	CASSE--;
	pthread_cond_signal(&d);

	return (void*) 17;
}



//flag per i segnali SIGQUIT e SIGHUP
static volatile sig_atomic_t sigintflag  =  0;

//gestione dei segnali, in caso di SIGHUP la variabile globale CLOSE viene modificata
static void sighandler(int sig) {
    
    switch(sig) {
    case SIGHUP: {
	if (sigintflag == 0) { 
	sigintflag = 1; 
	CLOSE++; }
	else {
		abort();
	}
    } break;
    case SIGQUIT: {
	if (sigintflag == 0) { 
	sigintflag = 1; 
	//esegue codice 
	}
	else {
		abort();
	}
    } break;
    
    default:{	
	abort();
    }
    }
}


int main() {

   	//int ciclo = 0; //uso con valgrind
	// installo un unico signal handler per tutti i segnali che mi interessano
	struct sigaction sa;
	// resetto la struttura
	memset (&sa, 0, sizeof(sa));   
	sa.sa_handler = sighandler;
	if (sigaction(SIGQUIT,  &sa, NULL) == -1) {
		perror("sigaction SIGINT");
	}
	if (sigaction(SIGHUP,  &sa, NULL) == -1) {
		perror("sigaction SIGINT");
	}
	//gestico i parametri iniziali
	int A[9];
	lettura(A);
	assegnamento(A);
	
	//setto il valore iniziale alle variabili globali rimanenti
	TOT=C;
	CURRENT=0;
	CLOSE=0;
	CASSE=0;	
	
	int DIM = K + C + 1;
	pthread_t *th;
	th = malloc(DIM*sizeof(pthread_t));
	//th = malloc(1024*sizeof(pthread_t));
	int* a;
	a = malloc(DIM*sizeof(int));
	//a = malloc(1024*sizeof(int));
	int i = 0;

	//creo threads casse
	for(i=0; i<K; i++) {
		a[i]=i;
		
		if (pthread_create(&th[i], NULL, thread_cassa, &a[i]) != 0) {
			
		    fprintf(stderr, "pthread_create failed\n");
		    exit(EXIT_FAILURE);
		}
	}
	//creo thread direttore
	pthread_create(&th[i], NULL, thread_direttore, &a[i]);
	i++;
	
	printf("inizio: %d\n", i);
	//creo threads clienti
    	for(i=i;i<DIM; i++) {
    		a[i]=i;
    		if (pthread_create(&th[i], NULL, thread_clienti, &a[i]) != 0) {
			   fprintf(stderr, "pthread_create failed\n");
			   exit(EXIT_FAILURE);
			}
	}
	//ciclo nel quale vengono creati i thread clienti se le condizioni lo permettono
	while(1) {

		/*
		 if (ciclo >= 10) {    
			CLOSE++;      // uso con valgrind
		}
		ciclo++; 
		*/

		//se la variable CLOSE assume il valore true, esco dal ciclo while
		//questo significa che non verranno più creati thread clienti 
		if(CLOSE) {
			break;
		}
		
		pthread_mutex_lock(&lock);
		//se ci sono ancora troppi clienti nel supermercato o il supermercato non è chiuso, 
		//il processo va in attesa passiva 
		while(TOT - E >= CURRENT && !CLOSE) {
			pthread_cond_wait(&v, &lock);
		}
		pthread_mutex_unlock(&lock);
		//se è possibile far entrare nuovi clienti nel supermercato ed esso non risulta chiuso,
		//vengono creati nuovi thread clienti
		if(TOT - E < CURRENT && !CLOSE ) {
			printf("Entrano nuovi clienti...\n");
			TOT = TOT+E;
			th=realloc(th, (DIM+E+1)*sizeof(pthread_t));
			a=realloc(a, (DIM+E+1)*sizeof(int));
			
			for(i=DIM; i< (DIM+E); i++){
				a[i]=i;
				if (pthread_create(&th[i], NULL, thread_clienti, &a[i]) != 0) {
	   				fprintf(stderr, "pthread_create failed\n");
	    				exit(EXIT_FAILURE);
				}
			}
			DIM=DIM+E;
			}
	}
	
	pthread_mutex_lock(&lock);
	//attesa chiusura casse
	while(CASSE>=0) {
		printf("Attesa chiusura casse\n");
		pthread_cond_wait(&v, &lock);
	}
	pthread_mutex_unlock(&lock);

	printf("Chiusura casse effettuata, totale attori nel processo: %d\n", i);

	for(i=0;i<DIM; i++) {
		if (pthread_join(th[i], NULL) == -1) {
		    fprintf(stderr, "pthread_join failed\n");
		}
	}

	free(th);
	free(a);

	return 0;
	
}

	
