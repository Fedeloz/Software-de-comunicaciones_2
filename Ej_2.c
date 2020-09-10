#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>

                // Estados

#define ILDE 0
#define LOWER 1
#define UPPER 2
#define FILTRO 3

                // Eventos

#define MENSAJE 10
#define TO_LOWER 11
#define TO_UPPER 12
#define KEYWORD 13
#define QUIT 14
#define SIGINT_ 15

                // Variables globales

#define T 128
int evento_recibido;
int estado;
int conectado;
int p[2];
int fifo;
char key[T];
char mensaje[T];

                // Manejadores de señal

void m_SIGUSR1(int signo);
void m_SIGUSR2(int signo);
void m_SIGQUIT(int signo);
void m_SIGINT(int signo);
void m_SIGALRM(int signo);

                // Funciones auxiliares

int espera_evento();
int keyword(char *mensaje, char *clave);
char *strEstado(int estado);
char *strEvento(int evento);
void filtrado(char *msg, char *keyword, char *c);
ssize_t read_n(int fifo, char * mensaje, size_t longitud_mensaje);
ssize_t write_n(int fifo1, char * mensaje, size_t longitud_mensaje);

                // ANCHOR Main

int main(int argc, char *argv[]){

                // Precondición argumento de entrada

    if(argc < 2){
        printf("Uso: %s <nombrefifo>\n", argv[0]);
        exit(-1);
    }

                // ANCHOR pipe

    if(pipe(p) < 0){
        perror("pipe");
        exit(-1);
    }

                // Registro de manejadores de señal

    if(signal(SIGUSR1, m_SIGUSR1) == SIG_ERR){
        perror("Registro SIGUSR1\n");
        exit(-1);
    }
    if(signal(SIGUSR2, m_SIGUSR2) == SIG_ERR){
        perror("Registro SIGUSR2\n");
        exit(-1);
    }
    if(signal(SIGQUIT, m_SIGQUIT) == SIG_ERR){
        perror("Registro SIGQUIT\n");
        exit(-1);
    }
    if(signal(SIGINT, m_SIGINT) == SIG_ERR){
        perror("Registro SIGINT\n");
        exit(-1);
    }
    if(signal(SIGALRM, m_SIGALRM) == SIG_ERR){
        perror("Registro SIGINT\n");
        exit(-1);
    }
    

                // Variables main

    char b[T];
    conectado = 0;

                // Variables para controlar la máquina de estados

    estado = ILDE;

                // ANCHOR fifo

    sprintf(b, "/tmp/");
    strcat(b, argv[1]);
	printf("Se va a conectar a la fifo1: %s\n", b);

    fifo = open(b, O_RDONLY);
	if (fifo < 0){
		perror("open");
		exit(1);
	}

    conectado = 1;    

    sprintf(b, "El pid es: %d\n", getpid());
    write(1, b, strlen(b));

    sprintf(b, "El filtrado de mensajes va a comenzar. Estado: %s, esperando evento...\n", strEstado(estado));
    write(1, b, strlen(b));

                // Máquina de estados

    while(conectado > 0){        // ANCHOR while

        printf("Estado actual: %s\n", strEstado(estado));
        evento_recibido = espera_evento();
        printf("Evento recibido: %s\n", strEvento(evento_recibido));

        switch(estado){

            case ILDE:      // ANCHOR ILDE

                switch(evento_recibido){
                    case QUIT:
                    break;

                    case MENSAJE:
                        sprintf(b, "El mensaje es: %s\n", mensaje);
                        write(1, b, strlen(b));
                    break;

                    case TO_LOWER:
                        estado = LOWER;
                    break; 

                    case TO_UPPER:
                        estado = UPPER;
                    break;

                    case KEYWORD:
                        estado = FILTRO;
                    break;

                    case SIGINT_:
                        conectado = 0;
                    break;

                    default:
                        conectado = 0;
                        /*sprintf(b, "Evento no esperado en estado ILDE: %d\n", evento_recibido);
                        write(1, b, strlen(b));
                        exit(-1);*/
                }

            break;

            case LOWER:     // ANCHOR LOWER

                switch(evento_recibido){
                    case TO_LOWER:
                    break;

                    case MENSAJE:
                        for(int i = 0; i < strlen(mensaje); i++){
                            mensaje[i] = tolower(mensaje[i]);
                        }
                        printf("El mensaje en minúsculas es: %s\n", mensaje);
                    break;

                    case TO_UPPER:
                        estado = UPPER;
                    break;

                    case KEYWORD:
                        estado = FILTRO;
                    break;

                    case QUIT: 
                        estado = ILDE;
                    break;

                    case SIGINT_:
                        conectado = 0;
                    break;

                    default:
                        conectado = 0;
                        /*sprintf(b, "Evento no esperado en estado LOWER: %d\n", evento_recibido);
                        write(1, b, strlen(b));
                        exit(-1);*/
                }

            break;

            case UPPER:     // ANCHOR UPPER

                switch(evento_recibido){

                    case TO_UPPER:
                    break;
                    
                    case MENSAJE:
                        for(int i = 0; i < strlen(mensaje); i++){
                            mensaje[i] = toupper(mensaje[i]);
                        }
                        printf("El mensaje en mayúsculas es: %s\n", mensaje);

                    break;

                    case TO_LOWER:
                        estado = LOWER;
                    break; 

                    case KEYWORD:
                        estado = FILTRO;
                    break;

                    case QUIT: 
                        estado = ILDE;
                    break;

                    case SIGINT_:
                        conectado = 0;
                    break;

                    default:
                        conectado = 0;
                        /*sprintf(b, "Evento no esperado en estado UPPER: %d\n", evento_recibido);
                        write(1, b, strlen(b));
                        exit(-1);*/
                }

            break;

            case FILTRO:        // ANCHOR FILTRO
                switch(evento_recibido){
                    case KEYWORD:
                    break;

                    case MENSAJE:
                        filtrado(mensaje, key, b); 
                    break;

                    case TO_LOWER:
                        estado = LOWER;
                    break; 

                    case TO_UPPER:
                        estado = UPPER;
                    break;

                    case QUIT: 
                        estado = ILDE;
                    break;

                    case SIGINT_:
                        conectado = 0;
                    break;

                    default:
                        conectado = 0;
                        /*sprintf(b, "Evento no esperado en estado FILTRO: %d\n", evento_recibido);
                        write(1, b, strlen(b));
                        exit(-1);*/
                }

            break;

            default:
                sprintf(b, "Estado no esperado : %d\n", estado);
                write(1, b, strlen(b));
        }
    }
    sprintf(b, "Fin de la máquina!\n");
    write(1, b, strlen(b));

                // ANCHOR close fifo

    if(close(fifo) < 0){
        perror("Close fifo1");
        exit(-1);
    }

                // ANCHOR close pipe

    if(close(p[0]) < 0){
        perror("Close pipe lectura");
        exit(-1);
    }

    if(close(p[1]) < 0){
        perror("Close pipe escritura");
        exit(-1);
    }


    return 0;
}

                // ANCHOR Manejadores

void m_SIGUSR1(int signo){      //Manejador de SIGUSR1
    evento_recibido = TO_LOWER;
    if(write_n(p[1], (char*) &evento_recibido, sizeof(evento_recibido)) < 0){
        perror("write_n 1");
        exit(-1);
    }
    signal(SIGUSR1, m_SIGUSR1);
}
void m_SIGUSR2(int signo){      //Manejador de SIGUSR2
    evento_recibido = TO_UPPER;
    if(write_n(p[1], (char*) &evento_recibido, sizeof(evento_recibido)) < 0){
        perror("write_n 2");
        exit(-1);
    }
    signal(SIGUSR2, m_SIGUSR2);
}
void m_SIGQUIT(int signo){      //Manejador de SIGINT
    evento_recibido =QUIT;
    if(write_n(p[1], (char*) &evento_recibido, sizeof(evento_recibido)) < 0){
        perror("write_n 3");
        exit(-1);
    }
    signal(SIGQUIT, m_SIGQUIT);
}
void m_SIGINT(int signo){      //Manejador de SIGINT
    evento_recibido = SIGINT_;
    if(write_n(p[1], (char*) &evento_recibido, sizeof(evento_recibido)) < 0){
        perror("write_n 4");
        exit(-1);
    }
    signal(SIGINT, m_SIGINT);
}
void m_SIGALRM(int signo){      //Manejador de SIGINT
    evento_recibido = QUIT;
    if(write_n(p[1], (char*) &evento_recibido, sizeof(evento_recibido)) < 0){
        perror("write_n 3");
        exit(-1);
    }
    signal(SIGALRM, m_SIGALRM);
}

                // Fundiones auxiliares

int espera_evento(){
    int leidos = read(fifo, mensaje, T);     // Abrir fifo y leer
    if(leidos < 0){
        if(errno == EINTR){
            if(read_n(p[0], (char*) &evento_recibido, sizeof(int)) != sizeof(int)){
                perror("read_n");
                exit(-1);
            }
        }else{
            printf("Cliente desconectado\n");
            perror("read");
            exit(-1);
        }    
    }else if(leidos == 0){
        conectado = 0;
    }else{
        mensaje[leidos] = '\0';
        //printf("Leido: %s\n", mensaje);
        if(keyword(mensaje, key) == 0){
            evento_recibido = MENSAJE;
        }else{
            evento_recibido = KEYWORD;
            printf("Poniendo alarma...\n");
            alarm(20);
        }
    }
    return evento_recibido;
}

int keyword(char *mensaje, char *clave){
    int encontrado = 0;
    if( (mensaje[0] == '/') && (mensaje[1] == 'k') && (mensaje[2] == 'e') && (mensaje[3] == 'y') && (mensaje[4] == ':') ){
        for(unsigned i = 5; i < strlen(mensaje); i++){
            clave[i-5] = mensaje[i];
        }
        clave[strlen(mensaje) -6] = '\0';
        encontrado = 1;
    }
    return encontrado;
}

void filtrado(char *msg, char *keyword, char *c){
    char aux[T];
    sprintf(c, msg);        // Se copia msg en c
    if(strlen(keyword) < strlen(msg)){      // Solo se entra si keyword cabe en msg
        for(unsigned i = 0; i < strlen(msg); i++){        // Recorremos msg mientras quepa keyword
            sprintf(aux, &msg[i]);
            aux[strlen(keyword)] = '\0';        // Aux es el fragmento de msg con tamaño keyword desde cada posición
            if(strcmp(keyword, aux) == 0){       // Ahora que tienen el mismo tamaño comparamos
                for(unsigned j = 0; j < strlen(keyword); j++){
                    c[j + i] = '*';
                }
            }
        }
    }
    printf("Mensaje filtrado: %s\n",c);
}

char *strEstado(int estado){        //Devolvemos un puntero a char con el estado en el que estamos
    switch(estado){
        case 0:
            return "ILDE";
        break;
        
        case 1:
            return "LOWER";
        break;

        case 2:
            return "UPPER";
        break;

        case 3:
            return "FILTRO";
        break;

        default:
            return "Estado no válido";
    }
}
char *strEvento(int evento){        //Devolvemos un puntero a char con el evento que llega
    switch(evento){
        case 10:
            return "MENSAJE";
        break;
        case 11:
            return "TO_LOWER";
        break;
        case 12:
            return "TO_UPPER";
        break;

        case 13:
            return "KEYWORD";
        break;

        case 14:
            return "QUIT";
        break;
        
        case 15:
            return "SIGINT";
        break;

        default:
            return "Evento no válido";
    }
}
ssize_t read_n(int fifo, char * mensaje, size_t longitud_mensaje) {     // read_n
  ssize_t a_leer = longitud_mensaje;
  ssize_t total_leido = 0;
  ssize_t leido;
  
  do {
    errno = 0;
    leido = read(fifo, mensaje + total_leido, a_leer);
    if (leido >= 0) {
        total_leido += leido;
        a_leer -= leido;
    }
    }while((
        (leido > 0) && (total_leido < longitud_mensaje)) ||
        (errno == EINTR));
  
    if (total_leido > 0) {
        return total_leido;
    }else {
    /* Para permitir que devuelva un posible error en la llamada a read() */
    return leido;
  }
}

                //  ANCHOR write_n

ssize_t write_n(int fifo1, char * mensaje, size_t longitud_mensaje) {
  ssize_t a_escribir = longitud_mensaje;
  ssize_t total_escrito = 0;
  ssize_t escrito;
  
  do {
    errno = 0;
    escrito = write(fifo1, mensaje + total_escrito, a_escribir);
    if (escrito >= 0) {
      total_escrito += escrito ;
      a_escribir -= escrito ;
    }
  } while(
        ((escrito > 0) && (total_escrito < longitud_mensaje)) ||
        (errno == EINTR));
  
  if (total_escrito > 0) {
    return total_escrito;
  } else {
    /* Para permitir que devuelva un posible error de la llamada a write */
    return escrito;
  }
}
