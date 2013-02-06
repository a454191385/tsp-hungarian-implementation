/**
 * Hungarian implementation
 * Elvis Ciotti
 * elvis@phpntips.com
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h> /*per rand*/

#define M 1500
#define N 1500

#define OUTFILE "out_ungherese.txt"

/* CODA: gli elementi entrano da sinistra(prec_p) ed escono da destra (succ_p) */
typedef struct elemento_coda {
    int indice_k;
    int direzione; /* 'o' origine    'd' destinazione */
    struct elemento_coda *succ_p, *prec_p;
} elemento_coda_t;

/* VARS GLOBALI */

int MCosti[M][N];
int MCosti_iniz[M][N];
int X[M][N];

int m, n;
int nstampa;

int a[M];
int ps[M]; /*  ps[1] ï¿½ il primo elemento */
int u_duale[M];

int b[N];
int pd[N];
int v_duale[N];
int s; /*partenza     cammino aumentante*/
int t; /*destinazione cammino aumentante*/
FILE *fout = NULL;

elemento_coda_t *ingresso_coda_stazioni = NULL;
elemento_coda_t *uscita_coda_stazioni = NULL;
elemento_coda_t *ingresso_coda_cammino = NULL;
elemento_coda_t *uscita_coda_cammino = NULL;

/* PROTOTIPI FUNZIONI*/
void acquisisci(int argc, char **argv);
void reset_etichette();
void stampa();
void normalizza();
void soluzione_iniziale_duale();
int esistono_a_esposti();
int cammino_aumentante();
void aggiorna_soluzione_duale();
void incrementa_flusso();
int minimo(int a, int b);
int costo_totale();
void stampa_soluz_duale();
void errore(char *str_err);

/**/
void metti_in_coda(elemento_coda_t **uscita_p, elemento_coda_t **ingresso_p, int indice_k, int direzione);
elemento_coda_t *togli_da_coda(elemento_coda_t **uscita_p, elemento_coda_t **ingresso_p);
int is_coda_vuota(elemento_coda_t *uscita_p, elemento_coda_t *ingresso_p);
void reset_coda(elemento_coda_t **uscita_p, elemento_coda_t **ingresso_p);

/*
 * MAIN
 */
int main(int argc, char **argv) {
    int temp = 0;
    int countercicli = 0;
    double tempo_esecuz, inizio;

    acquisisci(argc, argv);
    nstampa = 0;

    fout = fopen(OUTFILE, "w");
    fprintf(fout, "**** METODO UNGHERESE ****    n\nValori inseriti:\n");
    stampa();

    inizio = clock();
    normalizza();
    soluzione_iniziale_duale();

    /*finchï¿½ ci sono stazioni esposte in partenza*/
    while (esistono_a_esposti() != -1) {
        /* aggiorno soluzione duale (I J + - e delta) finchï¿½ non esistono cammini aumentanti */
        while (!cammino_aumentante())
            aggiorna_soluzione_duale();
        /*a questo punto esiste un cammino*/
        incrementa_flusso();
    }
    tempo_esecuz = clock() - inizio; /* in CLOCKS_PER_SEC */

    stampa();

    fprintf(fout, "costo totale %d\nTempo di esecuzione %lf\n", costo_totale(), tempo_esecuz);

    fclose(fout);
    printf("Risultato in OUTFILE\n");
    /*system("pause");*/
    return 0;
}

/**
 * Acquisizione da file o shell
 * 
 * @param argc
 * @param argv
 */
void acquisisci(int argc, char **argv) {

    int i, j, min, max;
    FILE *fdata;
    int numero;
    int modalita_ab = 0;

    if (argc == 1) /*nessun argomento, genero casualmente */ {
        /*imposto seme casuale */
        srand((unsigned int) time(NULL));

        printf("Nessun file di input specificato.\nI dati verranno generati casualmente.\n\n");

        do {
            printf(" Numero partenze: ");
            scanf("%d", &m);
            printf(" Numero destinazioni: ");
            scanf("%d", &n);
        } while (m < 1 || n < 1);

        /*chiedo modalitï¿½ aquisizione a[] e b[]*/
        do {
            printf(" Specificare valori delle partenze manualmente[0] o casualmente[1] ? : ");
            scanf("%d", &modalita_ab);
        } while (modalita_ab != 0 && modalita_ab != 1);


        if (modalita_ab == 0) {
            /*generazione manuale di a[] e b[]*/
            printf("  Acquisizione delle %d partenze\n", m);
            for (i = 0; i < m; i++) {
                printf("   Inserisci a[%d]: ", i);
                scanf("%d", &(a[i]));
            }
            printf(" Acquisizione delle %d destinazioni\n", n);
            for (j = 0; j < n; j++) {
                printf("  Inserisci b[%d]: ", j);
                scanf("%d", &(b[j]));
            }
        } else /*generazione casuale*/ {
            printf("  Valore minimo per partenze e destinazioni: ");
            scanf("%d", &min);
            printf("  Valore massimo per partenze e destinazioni: ");
            scanf("%d", &max);

            /* genero a[] */
            for (i = 0; i < m; i++) {
                a[i] = (rand() % (max - min)) + min;
            }

            /* genero b[] */
            for (j = 0; j < n; j++) {
                b[j] = (rand() % (max - min + 1)) + min;
                /*printf("%d ",b[j]);*/
            }
            /*printf("  }\n");*/
        }

        /*chiedo range Cij*/
        printf(" Valore minimo per i costi: ");
        scanf("%d", &min);
        printf("  Valore massimo per i costi: ");
        scanf("%d", &max);

        for (i = 0; i < m; i++)
            for (j = 0; j < n; j++) {
                numero = (rand() % (max - min)) + min;
                MCosti[i][j] = numero;
                MCosti_iniz[i][j] = numero;
            }

    } else /* almeno un argomento, considero solo il primo (nome file da aprire) */ {
        fdata = fopen(argv[1], "r");
        if (fdata == NULL)
            errore("File di input non esistente o non accessibile !");

        /* Leggi il numero di righe e colonne  */
        if (fscanf(fdata, "%d %d", &m, &n) == EOF)
            errore("impossibile leggere numero di righe  colonne!");

        /* scan costi */
        for (i = 0; i < m; i++)
            for (j = 0; j < n; j++) {
                if (fscanf(fdata, "%d", &numero) == EOF)
                    errore("Fine inaspettata dei dati, ricontrolla che il numero di righe e colonne corrisponda !");
                MCosti[i][j] = numero;
                MCosti_iniz[i][j] = numero;
            }

        /* Leggo partenze */
        for (i = 0; i < m; i++)
            if (fscanf(fdata, "%d", &(a[i])) == EOF)
                errore("Impossibile leggere i valori delle partenze !");

        /* Leggo destinazioni */
        for (j = 0; j < n; j++)
            if (fscanf(fdata, "%d", &(b[j])) == EOF)
                errore("Impossibile leggere i valori delle destinazioni !");
    }

    /* azzero i flussi Xij */
    for (i = 0; i <= m; i++)
        for (j = 0; j <= n; j++)
            X[i][j] = 0;

    reset_etichette();
}

/*
 * reset ps e pd 
 */
void reset_etichette() {
    int i, j;
    /*inizializzo a -1 le etichette per la ricerca cammini*/
    for (i = 0; i <= m; i++)
        ps[i] = -1;

    for (j = 0; j <= n; j++)
        pd[j] = -1;

}

/**
 * Stampo costi, flussi, partenze, destinazioni, etichette
 */
void stampa() {
    int i, j;

    fprintf(fout, "\n ");
    /*stampo vettore b destinazioni */
    for (j = 0; j < n; j++)
        fprintf(fout, "%6d", b[j]);
    fprintf(fout, "\n");

    /*stampo costi, flussi e vettore a partenze*/
    for (i = 0; i < m; i++) {
        fprintf(fout, "%3d", a[i]);
        for (j = 0; j < n; j++)
            if (X[i][j] != 0)
                fprintf(fout, "%4d,%d", MCosti[i][j], X[i][j]);
            else
                fprintf(fout, "%4d  ", MCosti[i][j]);

        if (ps[i] != -1)
            fprintf(fout, "%3d", ps[i]);

        fprintf(fout, "\n");
    }

    /*stampo vettore pd*/
    fprintf(fout, " ");
    for (j = 0; j < n; j++) {
        if (pd[j] != -1)
            fprintf(fout, "%6d", pd[j]);
        else
            fprintf(fout, "      ");
    }
    fprintf(fout, "\n\n");

}

/*
 * Stampa soluzione duale, di ausilio alla funzione sopra
 * stampa vettori U e V 
 */
void stampa_soluz_duale() {
    int i, j;

    fprintf(fout, "u=");
    for (i = 0; i < m; i++)
        fprintf(fout, "%d ", u_duale[i]);
    fprintf(fout, "\nv=");
    for (j = 0; j < n; j++)
        fprintf(fout, "%d ", v_duale[j]);

    fprintf(fout, "\n");

}

/* 
 * rendo somma invii uguale alla somma spedizioni
 * rendo somma invii == somma spedizioni
 */
void normalizza() {
    int somma_a = 0, somma_b = 0, i, j;

    /*per ogni riga*/
    for (i = 0; i < m; i++)
        somma_a += a[i];
    for (j = 0; j < n; j++)
        somma_b += b[j];

    if (somma_a > somma_b) {
        /*imposto ultima colonna*/
        for (i = 0; i < m; i++)
            MCosti[i][n] = MCosti_iniz[i][n] = 0;
        /*aggiungo destinazione fittizia*/
        b[n] = somma_a - somma_b;
        /*aumento destinazioni*/
        n = n + 1;
        fprintf(fout, "Aggiunta destinazione !\n");
    } else if (somma_b > somma_a) {
        /*imposto ultima riga*/
        for (j = 0; j < n; j++)
            MCosti[m][j] = MCosti_iniz[m][j] = 0;
        /*aggiungo partenza fittizia*/
        a[m] = somma_b - somma_a;
        /*aumento partenza*/
        m = m + 1;
        fprintf(fout, "Aggiunta partenza !\n");
    }
}

/**
 * Creo soluzione iniziale duale con minimi di riga e colonna. Miglioro soluzione rp
 */
void soluzione_iniziale_duale() {
    int min, i, j;
    /*per ogni riga*/
    for (i = 0; i < m; i++) {
        /*cerco il minimo di riga*/
        min = MCosti[i][0];
        for (j = 1; j < n; j++)
            if (MCosti[i][j] < min) min = MCosti[i][j];

        /*scrivo il minimo nel vettore u^*/
        u_duale[i] = min;

        /*tolgo minimo alla riga*/
        for (j = 0; j < n; j++)
            MCosti[i][j] = MCosti[i][j] - min;
    }

    /*per ogni colonna*/
    for (j = 0; j < n; j++) {
        /*cerco il minimo di colonna*/
        min = MCosti[0][j];
        for (i = 1; i < m; i++)
            if (MCosti[i][j] < min) min = MCosti[i][j];

        /*scrivo il minimo nel vettore v^*/
        v_duale[j] = min;

        /*tolgo minimo alla colonna*/
        for (i = 0; i < m; i++)
            MCosti[i][j] = MCosti[i][j] - min;
    }

    fprintf(fout, "RP con costi = Cij-Uij-Vij:\n");
    stampa();
    stampa_soluz_duale();

    /* miglioro soluzione PR */
    for (i = 0; i < m; i++)
        for (j = 0; j < n; j++)
            if (MCosti[i][j] == 0) {
                min = minimo(a[i], b[j]);
                X[i][j] = min;
                a[i] -= min;
                b[j] -= min;
            }

    fprintf(fout, "Soluzione iniziale di RP");
    stampa();
}

/*
 */
int minimo(int a, int b) {
    if (a < b)
        return a;
    else
        return b;

}

/* 
 * restituisce indice della prima stazione_esposta e segnala se non ve ne sono
 * restituisce indice della prima stazione_esposta | -1 se non ne esistono
 */
int esistono_a_esposti() {
    int i;
    for (i = 0; i < m; i++)
        if (a[i] != 0) return i;
    return -1; /*non sono uscito prima, quindi non esistono esposte*/
}

/* 
 * step2 algo ungherese. Cerca il cammino aumentante e lo ricostruisce.
 * 
 * 1=cammino trovato 0 =cammino non trovato  
 */
int cammino_aumentante() {
    int i, j;

    elemento_coda_t *elem_coda_temp;
    int k;
    int direz;
    int cammino_trovato = 0;

    t = -1;
    reset_etichette(); /*serve ???? */
    /*reset coda stazioni*/
    reset_coda(&uscita_coda_stazioni, &ingresso_coda_stazioni);


    /*trovo prima stazione di partenza esposta*/
    s = esistono_a_esposti();

    ps[s] = 0;
    metti_in_coda(&uscita_coda_stazioni, &ingresso_coda_stazioni, s, 'o');

    fprintf(fout, "\nRicerco cammino...\n");
    while (cammino_trovato != 1 && !is_coda_vuota(uscita_coda_stazioni, ingresso_coda_stazioni)) {
        /* tolgo ultimo elemento dalla coda (primo elemento K da LIST 
         * secondo procedura cam.aument.) */
        elem_coda_temp = togli_da_coda(&uscita_coda_stazioni, &ingresso_coda_stazioni);
        k = elem_coda_temp->indice_k;
        direz = elem_coda_temp->direzione;

        /* se k ï¿½ una origine ... */
        if (direz == 'o') {
             /* per ogni el. della riga*/
            for (j = 0; j < n && !cammino_trovato; j++)
                /* se ha costo nullo, non ï¿½ stata etichettata ... */
                if (MCosti[k][j] == 0 && pd[j] == -1) {
                    /*metto etichetta*/
                    pd[j] = k + 1; 
                    if (b[j] == 0)
                        metti_in_coda(&uscita_coda_stazioni, &ingresso_coda_stazioni, j, 'd');
                    else {
                        t = j;
                        cammino_trovato = 1;
                        /*TROVATO CAM.AUM. => esco dal ciclo principale*/
                    }
                }
        }  else {
            /* per ogni el. della colonna*/
            for (i = 0; i < m; i++)
                /* se ha costo nullo, non ï¿½ stata etichettata e c'ï¿½ un flusso */
                if (MCosti[i][k] == 0 && ps[i] == -1 && X[i][k] > 0) {
                    ps[i] = k + 1;
                    metti_in_coda(&uscita_coda_stazioni, &ingresso_coda_stazioni, i, 'o');
                }
        }
    }

    stampa();

    /* se ho trovato un cammino... */
    if (cammino_trovato) {
        fprintf(fout, "Cammino trovato !\n");
        
        /* cammino trovato (t ï¿½ stato giï¿½ impostato)*/
        return 1; 
    } else {
        t = 0;
        fprintf(fout, "Cammino NON trovato, aggiorno la soluzione !\n");
        
        /*cammino non esistente*/
        return 0; 
    }

}

/* 
 * step4 dellÕalgoritmo.
 * cerco il minimo fra elemento in I+ e J-, aumento di delta gli u^ in I+, diminuisco di delta i v^ in J-, 
 * diminuisco di delta i costi in I+ e J-, umento di delta i costi in I- e J+
 */
void aggiorna_soluzione_duale() {
    int i, j;

    int delta = 32000;

    /*cerco il minimo fra elemento in I+ e J-*/
    for (i = 0; i < m; i++)
        for (j = 0; j < n; j++)
            if (ps[i] != -1 && pd[j] == -1 && MCosti[i][j] < delta)
                delta = MCosti[i][j];
    fprintf(fout, "delta=%d (riquadra sopra I+ J-)\n", delta);

    /*aumento di delta gli u^ in I+*/
    for (i = 0; i < m; i++)
        if (ps[i] != -1)
            u_duale[i] += delta;

    /*diminuisco di delta i v^ in J-*/
    for (j = 0; j < n; j++)
        if (pd[j] != -1)
            v_duale[j] -= delta;

    /*diminuisco di delta i costi in I+ e J-*/
    for (i = 0; i < m; i++)
        for (j = 0; j < n; j++)
            if (ps[i] != -1 && pd[j] == -1)
                MCosti[i][j] -= delta;

    /*aumento di delta i costi in I- e J+*/
    for (i = 0; i < m; i++)
        for (j = 0; j < n; j++)
            if (ps[i] == -1 && pd[j] != -1)
                MCosti[i][j] += delta;

    stampa_soluz_duale();
}

/* 
 * step3 algoritmo
 * trova cammino all'indietro e incrementa flusso
 */
void incrementa_flusso() {
    int passo = 0;
    int k;
    int flusso;
    int continua = 1;
    elemento_coda_t *elem_coda_temp;
    
    /*ricostruisco cammino all'indietro*/
    fprintf(fout, "ricostruisco cammino : fine <-");
    reset_coda(&ingresso_coda_cammino, &uscita_coda_cammino);
    k = t;
    flusso = minimo(a[s], b[t]);

    for (passo = 0; continua; passo++)
        /* destinazione */ 
        if (passo % 2 == 0) {
            metti_in_coda(&uscita_coda_cammino, &ingresso_coda_cammino, pd[k] - 1, k);
            fprintf(fout, "[%d-%d]", pd[k], k + 1);
            k = pd[k] - 1;
            if (k == s) continua = 0;

        } else {
            metti_in_coda(&uscita_coda_cammino, &ingresso_coda_cammino, ps[k] - 1, k);
            fprintf(fout, "[%d-%d]", ps[k], k + 1);
            flusso = minimo(flusso, X[k][ps[k] - 1]);
            k = ps[k] - 1;

        }
    fprintf(fout, "<- inizio \nflusso minimo=%d\n", flusso);
    /*aggiorno partenza e origine*/

    a[s] -= flusso;
    b[t] -= flusso;
    /*aggiorno flussi nel cammino*/
    for (passo = 0; !is_coda_vuota(uscita_coda_cammino, ingresso_coda_cammino); passo++) {
        elem_coda_temp = togli_da_coda(&uscita_coda_cammino, &ingresso_coda_cammino);
        /*printf("[%d,%d]", elem_coda_temp->indice_k +1 , elem_coda_temp->direzione +1);*/

        if (passo % 2 == 0) {
            X[elem_coda_temp->indice_k][elem_coda_temp->direzione] += flusso;
        } else {
            X[elem_coda_temp->direzione][elem_coda_temp->indice_k] -= flusso;
        }
    }

    fprintf(fout, "flussi aggiornati.\n");
    reset_etichette();
}

/*
 * calcola costo totale
 */
int costo_totale() {
    int t = 0, i, j;

    for (i = 0; i < m; i++)
        for (j = 0; j < n; j++)
            t = t + MCosti_iniz[i][j] * X[i][j];

    return t;
}

void errore(char *str_err) {
    fprintf(fout, "ERRORE: %s", str_err);
    exit(0);
}

/*
 * Funzioni per gestire la coda dei cammini 
 * (indicate come LIST nellÕalgoritmo descritto a lezione)
 * 
 * @param uscita_p
 * @param ingresso_p
 * @param indice_k
 * @param direzione
 */
void metti_in_coda(elemento_coda_t **uscita_p, elemento_coda_t **ingresso_p, int indice_k, int direzione) {
    elemento_coda_t *nuovo_p;
    nuovo_p = (elemento_coda_t *) malloc(sizeof (elemento_coda_t));
    nuovo_p->indice_k = indice_k;
    nuovo_p->direzione = direzione;
    nuovo_p->succ_p = NULL;

    /*se esistono giï¿½ elementi in coda ..*/
    if (*ingresso_p != NULL) {
        /*aggangio in coda*/    
        (*ingresso_p)->succ_p = nuovo_p; 
    } else {
        /*primo inserimento nella coda vuota*/
        *uscita_p = nuovo_p;
    }
    /* in ogni caso l'ingresso ï¿½ il nuovo elemento*/
    *ingresso_p = nuovo_p; 
}

/*
 * restituisce puntatore a primo elemento in coda (memoria giï¿½ allocata) e avanza puntatore uscita al secondo elemento
 * 
 * @param uscita_p
 * @param ingresso_p
 * @return 
 */
elemento_coda_t *togli_da_coda(elemento_coda_t **uscita_p, elemento_coda_t **ingresso_p) {
    elemento_coda_t *elem_p;
    elem_p = *uscita_p; /*punto elem_p al primo elemento */
    if (*uscita_p != NULL) {
        /*sposto il puntatore all'uscita sul secondo elemento */
        *uscita_p = (*uscita_p)->succ_p; 
        /* se ho tolto l'ultimo elemento, l'ingresso lo pongo a NULL */
        if (*uscita_p == NULL) 
            *ingresso_p = NULL;
    }
    return (elem_p);
}

/* 
 * 1|0 se la coda vuota o no.  basta passare puntatori ingresso e uscita per valore 
 * 
 * @param uscita_p
 * @param ingresso_p
 * @return 
 */
int is_coda_vuota(elemento_coda_t *uscita_p, elemento_coda_t *ingresso_p) {
    return (uscita_p == NULL && ingresso_p == NULL);
}

/**
 * @param uscita_p
 * @param ingresso_p
 */
void reset_coda(elemento_coda_t **uscita_p, elemento_coda_t **ingresso_p) {
    /*da migliorare liberando memoria*/
    *uscita_p = *ingresso_p = NULL;
}


