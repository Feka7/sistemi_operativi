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

//scrive i valori della cassa nel file di logs
void scrittura_ca(cassa* tmp) {
	
	FILE *fd=NULL;
	
	if ((fd=fopen(O, "a")) == NULL) {
		perror("opening file");
		 exit(EXIT_FAILURE);
	}
	
	tmp->time_total=tmp->time_total/10000000;
	float time_total = tmp->time_total/1000.000;
	float media_time;
	if(tmp->prod_total == 0) {
		media_time = 0;
	}
	else {
		media_time=(float)time_total/tmp->prod_total;
	}
 	fprintf(fd,"%d %d %d %.3f %.3f %d\n",tmp->id, tmp->prod_total, tmp->num_clienti, time_total, media_time, tmp->num_off);
	fclose(fd);
	return;
}
//restituisce il numero di clienti in coda nella cassa passata per parametro
int tail(cassa *t) {
	
	int sum=0;
	if(t->cl == NULL) {
		return 0;
	}
	else {
	
	cliente* tmp=t->cl->next;
	
	while(tmp!=NULL) {
		sum++;
		tmp=tmp->next;
	}
	}
	return sum;
}

//riassegna i clienti nella cassa passata come parametro in altre code
int riassegna_clienti(cassa* t, int k) {
	if(t->cl == NULL) {
		return 1;
	}
	else {
	cliente* tmp = t->cl->next;
	cliente* prec;
	while(tmp!=NULL) {
		prec = tmp;
		prec->change_tail++;
		tmp = tmp->next;
		prec->next=NULL;
		if((!select_cassa(prec, k))) {
			return 0;
		}
	}
	t->cl->next=NULL;
	}			
	return 1;
}

//funzione che modificfa stato cassa. Se viene chiusa, chiama riassegna_clienti
int modifica_cassa(int id, int s, int k) {
	if(p==NULL) {
		return 0;
	}
	cassa* tmp = p;
	while(tmp->id!=id) {
		tmp = tmp -> next;
	}
	tmp->stato=s;
	if(tmp->stato == 0) {
		riassegna_clienti(tmp, k);
		tmp->num_off++;
	}
	return 1;
}
//inserisce la cassa nella lista circolare contenente le altre casse
int inserisci_cassa(cassa* t) {

	if(t == NULL) {
		perror("errore inserimento cassa");
		return 0;
	}	
	cassa* tmp = p;
	if(tmp==NULL) {
		p = t;
		return 1;
	}
	else {
		while(tmp->next!=NULL && tmp->next!=p) {
			tmp = tmp->next;
		}
		tmp->next = t;
		t->next = p;
		return 1;
	}
	
}

//verifica le condizioni di apertura/chiusura di una cassa
int analisi_cassa(int s1, int s2, int k) {
	int min = 0, max = 0, curr_min = 0, curr_off = -1;
	cassa* tmp=p;
	if(p==NULL) {
		perror("nessuna cassa");
		return 0;
	}
	//scansiono tutti i nodi e prendo valori
	while(tmp->next!=p) {
		if(tmp->stato==0) {
			curr_off=tmp->id;
		}
		else if(tmp->tail < s1) {
			min++;
			curr_min = tmp->id;
		}
		else if(tmp->tail > s2) {
			max++;
		}
		tmp = tmp->next;
	}

	//printf("min: %d e max: %d\n", min, max);	
	//verifico condizioni per decidere se rimuovere o aggiungere
	if(min > 1 && max == 0) {
		if(!modifica_cassa(curr_min, 0, k)) {
			perror("errore analisi cassa");
			return 0;
		}
	}
	if(max > 0 && min < 2 && curr_off >=0) {
		if(!modifica_cassa(curr_off, 1, k)) {
			perror("errore analisi cassa");
			return 0;
		}
	}	

	return 1;
}
//funzione che sblocca le casse rimaste in attesa passiva alla chiusura del supermercato
void casse_lll() {
	cassa* n = p;
	if(n==NULL) {
		return;
	}
	cassa* tmp;
	if((tmp = n->next) != NULL) {
		while(n != tmp) {
			pthread_cond_broadcast(&tmp->V);
			tmp = tmp->next;
		}
	}
	pthread_cond_broadcast(&tmp->V); 
	return;
}
//funzione che elimina le casse presenti nella lista circolare 
void casse_off() {
	cassa* n = p;
	if(n==NULL) {
		return;
	}
	cassa* tmp;
	cassa* prec;
	if((tmp = n->next) != NULL) {
		while(n != tmp) {
			prec = tmp;
			tmp = tmp->next;
			free(prec);
		}
	}
	free(n);
	return;
}




