#if !defined(UTILS_H)
#define UTILS_H
#include <stdio.h>

#define CONF "./conf/config.txt"
#define MAX_LEN 1024
#define O "./logs/log.txt"

//i clienti sono rappresentati come nodi di una lista
typedef struct _cliente {
	int id; //fatto
	int n_prod; //fatto
	long time_tail; //fatto
	long time_total;//fatto
	int change_tail;//numero cambio code,fatto
	struct timespec fast_c;
	struct _cliente *next;
} cliente;

//le casse sono rappresentate come una lista circolare, ogni nodo ha una lista di clienti
typedef struct _cassa {
	int id; //fatto
	struct timespec fast;
	int stato;
	int tail;//clienti in coda attualmente //ok
	int num_clienti;//numero totale clienti analizzati,fatto
	long media_time;//tempo totale servizio fatto
	long time_total;//tempo totale aperta, DA FARE!
	int num_off;//numero di volte off,//ok
	int prod_total; //numero totale prodotti elab, fatto
	pthread_cond_t V;
	//tempo servizio ogni cliente??
	struct _cassa *next;
	struct _cliente *cl;
} cassa;

extern cassa* p;
extern cliente* p_dir;


void scrittura_cl(cliente* tmp);

int select_cassa (cliente *c, int k);

void casse_off();

int analisi_cassa();

int analisi_clienti();

void* thread_cassa(void* arg);

void* thread_clienti(void* arg);

int tail(cassa *t);

int add_dir(cliente* c);

void casse_lll();

int inserisci_cassa(cassa* t);

void scrittura_ca(cassa* tmp);

#endif





