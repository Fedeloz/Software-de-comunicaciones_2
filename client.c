#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/select.h>
#include <string.h>
#include <errno.h>

                //  ANCHOR  Aux Funciones

ssize_t write_n(int fd, char * mensaje, size_t longitud_mensaje);

/**
 * Los clientes reciben como argumento la fifo por donde escucha el 
 * servidor y, solicita descripción de eventos al usuario hasta que 
 * éste no introduce la cadena "fin". Entonces se desconecta y termina.
 */

                //  ANCHOR  Main

int main(int argc, char * argv[]){

                //Variables Main

	int fifo;
	int longMensaje;
	const int MAXBUFFER = 256;
	char buffer[MAXBUFFER];

	if (argc < 2){
		printf("Uso: %s <nombreFifo>\n",argv[0]);
		exit(1);
	}

	sprintf(buffer, "/tmp/");
  strcat(buffer, argv[1]);
	printf("Se va a conectar a la fifo1: %s\n", buffer);

	//creamos las fifos de entrada y salida para los clientes
	// se asumen creadas para facilitarlo


                // ANCHOR FIFO Creación

    umask(0);
    mkfifo(buffer, 0666);
    printf("Fifo creada.\n");
    if((fifo = open(buffer, O_WRONLY | O_APPEND)) < 0){
        perror("Open fifo");
        exit(-1);
    }

                //  ANCHOR  Enviar	
	
	printf("Introduce el suceso a enviar:\n");
	scanf("%s",buffer);
	while(strcmp(buffer,"fin") != 0){
		//calculamos la longitud, para enviarla previamente al servidor
		longMensaje = strlen(buffer);
		
		/*if (write_n(
			fifo,
			(char *)&longMensaje,
			sizeof(longMensaje)) != sizeof(longMensaje))
		{
			perror("write_n: longMensaje");
			exit(1);
		}*/
		
		//enviamos ahora el comando
		if (write_n(
			fifo,
			buffer,
			longMensaje) != longMensaje)
		{
			perror("write_n: comando");
			exit(1);
		}
	
		printf("Introduce el suceso a enviar: \n");
		//leidos = read(0,buffer,MAXBUFFER);
		//buffer[leidos-1] = '\0';
		scanf("%s",buffer);
	}
	longMensaje = 0;
	write_n(fifo, (char*)longMensaje, sizeof(int));
	write_n(fifo, "0", 1);

	//cerramos
	if (close(fifo) < 0 ) {
		perror("close");
		return -1;
	}
	
	return 0;
}

                //  ANCHOR  Aux Funciones

ssize_t write_n(int fd, char * mensaje, size_t longitud_mensaje) {
  ssize_t a_escribir = longitud_mensaje;
  ssize_t total_escrito = 0;
  ssize_t escrito;
  
  do {
    errno = 0;
    escrito = write(fd, mensaje + total_escrito, a_escribir);
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