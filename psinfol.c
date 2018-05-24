/**
 *   \file psinfo-l.c
 *   \brief base code for the program psinfo-l
 *
 *  This program prints some basic information for a given
 *  list of processes.
 *  You can use this code as a basis for implementing parallelization
 *  through the pthreads library.
 *
 *   \author: Danny Munera - Sistemas Operativos UdeA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>   // libreria para el uso de hilos
#include <semaphore.h> // libreria para el uso de semaforos

#define MAX_BUFFER 100
#define NUM_BUFFER 3 // definicion del numero de buffers, se pueden tener cargados hasta 3 estructuras del proceso
                     // "proc_info", los demás deberan esperar

//#define DEBUG
//estructura que representa la información que contiene un proceso.
typedef struct p_ {
  int pid;
  char name[MAX_BUFFER];
  char state[MAX_BUFFER];
  char vmsize[MAX_BUFFER];
  char vmdata[MAX_BUFFER];
  char vmexe[MAX_BUFFER];
  char vmstk[MAX_BUFFER];
  int voluntary_ctxt_switches;
  int nonvoluntary_ctxt_switches;
} proc_info;

//declaracion de los metodos 
void load_info(int pid);
void print_info(proc_info* pi);

// Creacion de los semaforos que se van a utilizar en el programa.
sem_t semaphoreP; // Semaforo para el productor.
sem_t semaphoreC; // Semaforo para el consumidor.
sem_t semaphore;  // semaforo para la sincronizacion sección crítica.

int iterator;
proc_info* all_proc; //arreglo que contiene las estructuras "proc_info" definidas para los procesos.

int main(int argc, char *argv[]){
  int i;
  // number of process ids passed as command line parameters
  // (first parameter is the program name)

  int n_procs = argc - 1;
  
  // validacion para la cantidad de argumentos necesarios para el coreccto funcionamiento del programa
  if(argc < 2){
    printf("Error\n");
    exit(1);
  }

  /*Allocate memory for each process info*/
  all_proc = (proc_info *)malloc(sizeof(proc_info)*n_procs);
  assert(all_proc!=NULL);


  // creacion del arreglo de los hilos que se utilizaran para la paralelización,
  // el tamaño de este sera el numero de argumentos entrados desde la consola "n_procs"
  pthread_t threads[n_procs];
  
  //inicializacion de  las variables semaforos
  sem_init(&semaphoreP,0,n_procs);
  sem_init(&semaphoreC,0,0);
  sem_init(&semaphore,0,1); //se indica que solo un proceso puede ejecutar la sección crítica a la vez

  // Get information from status file
  for(i = 0; i < n_procs; i++){
    int pid = atoi(argv[i+1]);
    //Se crean los hilos con la funcion de cargar los datos del proceso
    pthread_create(&threads[i],NULL,&load_info,pid);
  }

  //print information from all_proc buffer
  for(i = 0; i < n_procs; i++){
    sem_wait(&semC); // se  realiza la consulta al semaforo encargao del consumidor para saber si hay algo que imprimir (consumir)
    print_info(&all_proc[i%NUM_BUFFER]); // se realiza la oreracion modulo para manejar el limite de numero de buffer (este numero es limitado)
  }

  // free heap memory
  free(all_proc);

  return 0;
}

/**
 *  \brief load_info
 *
 *  Load process information from status file in proc fs
 *
 *  \param pid    (in)  process id
 *  \param myinfo (out) process info struct to be filled
 */
void load_info(int pid){
  sem_wait(&semaphoreP); //se espera la habilitacion del semaforo para empezar a producir ¿hay que producir?
  sem_wait(&semaphore); //Se espera la habilitacion para la ejecucion de la seccion critica ¿puedo ejecutar mi sección crítica?
  FILE *semaphoretatus;
  char buffer[MAX_BUFFER];
  char path[MAX_BUFFER];
  char* token;

  sprintf(path, "/proc/%d/status", pid);
  fpstatus = fopen(path, "r");
  assert(fpstatus != NULL);
#ifdef DEBUG
  printf("%s\n", path);
#endif // DEBUG
  //manejo de las iteraciones de acuerdo con el numero de buffers
  proc_info* myinfo = &all_proc[iterator%NUM_BUFFER]; 
  myinfo->pid=pid;
  while (fgets(buffer, MAX_BUFFER, fpstatus)) {
    token = strtok(buffer, ":\t");
    if (strstr(token, "Name")){
      token = strtok(NULL, ":\t");
#ifdef  DEBUG
      printf("%s\n", token);
#endif // DEBUG
      strcpy(myinfo->name, token);
    }else if (strstr(token, "State")){
      token = strtok(NULL, ":\t");
      strcpy(myinfo->state, token);
    }else if (strstr(token, "VmSize")){
      token = strtok(NULL, ":\t");
      strcpy(myinfo->vmsize, token);
    }else if (strstr(token, "VmData")){
      token = strtok(NULL, ":\t");
      strcpy(myinfo->vmdata, token);
    }else if (strstr(token, "VmStk")){
      token = strtok(NULL, ":\t");
      strcpy(myinfo->vmstk, token);
    }else if (strstr(token, "VmExe")){
      token = strtok(NULL, ":\t");
      strcpy(myinfo->vmexe, token);
    }else if (strstr(token, "nonvoluntary_ctxt_switches")){
      token = strtok(NULL, ":\t");
      myinfo->nonvoluntary_ctxt_switches = atoi(token);
    }else if (strstr(token, "voluntary_ctxt_switches")){
      token = strtok(NULL, ":\t");
      myinfo->voluntary_ctxt_switches = atoi(token);
    }
#ifdef  DEBUG
    printf("%s\n", token);
#endif
  }
  fclose(fpstatus);
  iterator++;
  sem_post(&sem); //se libera la sección crítica
  sem_post(&semC); //se le informa al comsumidor que hay un producto para consumir
}
/**
 *  \brief print_info
 *
 *  Print process information to stdout stream
 *
 *  \param pi (in) process info struct
 */
void print_info(proc_info* pi){
  printf("PID: %d \n", pi->pid);
  printf("Nombre del proceso: %s", pi->name);
  printf("Estado: %s", pi->state);
  printf("Tamaño total de la imagen de memoria: %s", pi->vmsize);
  printf("Tamaño de la memoria en la región TEXT: %s", pi->vmexe);
  printf("Tamaño de la memoria en la región DATA: %s", pi->vmdata);
  printf("Tamaño de la memoria en la región STACK: %s", pi->vmstk);
  printf("Número de cambios de contexto realizados (voluntarios"
	 "- no voluntarios): %d  -  %d\n\n", pi->voluntary_ctxt_switches,  pi->nonvoluntary_ctxt_switches);
}
