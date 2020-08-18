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

//scrive i valori del cliente nel file di logs
void scrittura_cl(cliente* tmp) {
	
	FILE *fd=NULL;
	
	if ((fd=fopen(O, "a")) == NULL) {
		perror("opening file");
		 exit(EXIT_FAILURE);
	}
	float time_total = (float)tmp->time_total/10000000000.000;
	float time_tail = (float)tmp->time_tail/10000000000.000;
	
 	fprintf(fd,"%d %d %.3f %.3f %d\n",tmp->id, tmp->n_prod, time_total, time_tail, tmp->change_tail);
	fclose(fd);
	return;
}


//funzione per far scegliere al cliente c la cassa su cui andare
int select_cassa (cliente *c, int k) {
	unsigned int seed = time(NULL);
	if(c == NULL || p == NULL) {
		return 0;
	}
	int n = (rand_r(&seed) % k) + 1;
	cassa* tmp = p;
	//printf("%d\n", n);
	while(n!=1) {
	
		tmp = tmp -> next;
		n--;
	}
	//se la cassa è chiusa continuo fino a che non ne trovo una aperta
	while(tmp->stato == 0) {
		tmp = tmp -> next;
	}
	if(tmp->cl == NULL) {
		tmp->cl = c;
		pthread_cond_broadcast(&tmp->V);
		return 1;
	}
	else {	
		
		cliente* tmp2 = tmp->cl;
		while(tmp2->next!=NULL) {
			tmp2 = tmp2 -> next;
		}
		tmp2->next = c;
	}
	return 1;
}

//aggiunge il cliente con 0 prodotti alla rispettiva lista
int add_dir(cliente* c){
 
	if(c == NULL) {
		perror("errore cliente add dir");
		return 0;
	}

	if(p_dir == NULL) {
		p_dir = c;
		return 1;
	}
	else {
		cliente* tmp = p_dir;
		while(tmp->next!=NULL) {
			tmp = tmp->next;
		}
		tmp->next = c;
	}
	return 1;
}

//elimina i clienti con 0 prodotti dalla rispettiva lista
int analisi_clienti() {
	cliente* tmp = p_dir;
	cliente* prec;
	while(tmp!=NULL) {
		scrittura_cl(tmp);
		prec = tmp;
		tmp = tmp->next;
		free(prec);
	}
	return 1;
}





