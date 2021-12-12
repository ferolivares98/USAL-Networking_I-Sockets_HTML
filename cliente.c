/*

** Fichero: cliente.c
** Autores:
** Fernando Olivares Naranjo DNI 54126671N
** Diego Sánchez Martín DNI 54126671N
*/


#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h> //Cabecera para el protocolo UDP
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <time.h>

extern int errno;

//Macros comunes
#define PUERTO 6121
#define BUFFERSIZE 1024
//Macros del UDP
#define TIMEOUT 6
#define MAXHOST 512
#define ADDRNOTFOUND	0xffffffff	/* value returned for unknown host */
#define RETRIES	5		/* number of times to retry before givin up */

void handler()
{
 printf("Alarma recibida \n");
}

int main(int argc, char *argv[])
{
    int s;				/* Variable con la que crearemos el socket */
   	struct addrinfo hints, *res;
    long timevar;			/* contains time returned by time() */
    struct sockaddr_in myaddr_in;	/* Para la dirección local del socket */
    struct sockaddr_in servaddr_in;	/* Para iniciar los datos del servidor al que nos conectaremos */
	  int addrlen, i, j, errcode;
    /* This example uses BUFFERSIZE byte messages. */
	  char buffer[BUFFERSIZE];
    FILE *ficheroDeOrdenes;
    char crlf[] = "\r\n";

    // Variables que tiene el cliente UDP que no tiene el TCP

    struct in_addr reqaddr;		/* for returned internet address */
    int n_retry;
    struct sigaction vec;
   	char hostname[MAXHOST];

  /* Comprobación de parámetros introduccidos por el usuario */
	if (argc != 4 || (strcmp(argv[2], "UDP") == 1 && strcmp(argv[2], "TCP") == 1))
  {
      //Aquí no hacemos la comprobación de la k porque las ordenes están
      //en la carpeta con los ficheros de ordenes y por línea de comandos
      //sólo vamos a lanzar el cliente y el servidor, la comprobación de la k
      //la haremos más adelante dentro del propio código.
    	fprintf(stderr, "Uso correcto del programa: \n");
      fprintf(stderr, "%s nombreServidor TCP/UDP ficheroDeOrdenes\n", argv[0]);
    	exit(1);
	}

  if ((ficheroDeOrdenes = (fopen(argv[3], "r"))) == NULL)
  {
    fprintf(stderr, "Error al abrir el fichero de ordenes especificado %s", argv[4]);
    exit(1);
  }

  /* Comprobar en la línea de comandos el protocolo introducido TCP o UDP */
  if(strcmp(argv[2], "TCP") == 0){
  	/* El cliente crea el socket de tipo SOCK_STREAM por ser TCP */
  	s = socket (AF_INET, SOCK_STREAM, 0);
    /* Comprobación de errores de la creación del socket */
  	if (s == -1) {
  		perror(argv[0]);
  		fprintf(stderr, "%s: unable to create socket\n", argv[0]);
  		exit(1);
  	}

  	/* Borramos / formateamos las estructuras con las direcciones */
  	memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
  	memset ((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));

  	/* Set up the peer address to which we will connect. */
  	servaddr_in.sin_family = AF_INET;

  	/* Get the host information for the hostname that the
  	 * user passed in. */
        memset (&hints, 0, sizeof (hints));
        hints.ai_family = AF_INET;
   	 /* esta funci�n es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta*/
      errcode = getaddrinfo (argv[1], NULL, &hints, &res); //Obtiene la ip asociada al nombre que le pasamos como primer argumento
      if (errcode != 0){
  			/* No encuentra el nombre. Hace return indicando el error */
  		    fprintf(stderr, "%s: No es posible resolver la IP de %s\n", argv[0], argv[1]);
  		    exit(1);
      } else {
  		/* Copia la dirección del host */
  		    servaddr_in.sin_addr = ((struct sockaddr_in *) res->ai_addr)->sin_addr;
  	  }
      freeaddrinfo(res);

      /* Numero de puerto del servidor en orden de red*/
  	   servaddr_in.sin_port = htons(PUERTO);
  		/* Intenta conectarse al servidor remoto a la direccion
       * que acaba de calcular en getaddrinfo, para ello le indicamos
       * el socket que acabamos de crear 's', la direccion del servidor remoto
       * 'serveraddr_in', y el tamaño de la estructura de tipo sockaddr_in
  		 */
  	if (connect(s, (const struct sockaddr *)&servaddr_in, sizeof(struct sockaddr_in)) == -1) {
  		perror(argv[0]);
  		fprintf(stderr, "%s: unable to connect to remote\n", argv[0]);
  		exit(1); /*Comprobacion de errores del connect */
  	}
  		/* Aquí ya tenemos el socket conectado con el servidor,
       * si queremos escribir el puerto efímero que me ha dado
       * el sistema para dialogar con el servidor podemos usar la
       * función getsockname(), dado el manejador del socket 's', me devuelve
       * la estructura de tipo sockaddr (myaddr_in) que le pasamos con
       * la ip y el puerto efímero del cliente (rellena la estrucura que acabamos de comentar)
       * Para ponerla por pantalla si la necesitamos.
       * También deberemos usar un puntero 'addrlen' para que se nos devuelva la longitud
       * de la dirección que nos devuelve getsockname.
  		 */
  	addrlen = sizeof(struct sockaddr_in);
  	if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1) {
  		perror(argv[0]);
  		fprintf(stderr, "%s: unable to read socket address\n", argv[0]);
  		exit(1);
  	 }
  	/* Print out a startup message for the user. */
  	time(&timevar);
  	/* The port number must be converted first to host byte
  	 * order before printing.  On most hosts, this is not
  	 * necessary, but the ntohs() call is included here so
  	 * that this program could easily be ported to a host
  	 * that does require it.
  	 */
  	printf("Connected to %s on port %u at %s", argv[1], ntohs(myaddr_in.sin_port), (char *) ctime(&timevar));

    /* Una vez conectados usaremos el handler/manejador del socket que hemos creado
    * 's' para dialogar con el servidor mediante las funciones "send()" y "recv()"
    */

  	/* Empezamos la parte de la práctica de 2021 HTTPS
    * leemos el fichero de ordenes hasta que la funcion
    * fgets() nos devuelva null */
    char lineasOrdenes[3][150]; //Cada una de las líneas que haya en el fichero de ordenes que indiquemos en cliente
    char orden[10];
    char fichero[150];
    char k[10];
    char *linea; //Cada línea del fichero de órdenes
    int contador; //Contador para ir separando las líneas en la variable anterior
    char peticionCliente[BUFFERSIZE];
    char respuestaServidor[BUFFERSIZE];

    while(fgets(buffer, BUFFERSIZE, ficheroDeOrdenes) != NULL)
    {
      strcpy(orden, "");
      strcpy(fichero, "");
      strcpy(k, "");
      /*for(i=0; i<3; i++){
        lineasOrdenes[i]="";
      }*/
      //contador = 0;
      linea = strtok(buffer, " "); //Obtenemos los tokens separándolos por el delimitador " "
      while(linea != NULL) //Mientras haya tokens
      {
        strcpy(orden, linea); //Copiamos el token para tratarlo individualmente
        linea = strtok(NULL, " ");
        if(linea!=NULL)
        {
          strcpy(fichero, linea); //Copiamos el token para tratarlo individualmente
          linea = strtok(NULL, " ");
        }
        if(linea!=NULL)
        {
          strcpy(k, linea); //Copiamos el token para tratarlo individualmente
          linea = strtok(NULL, " ");
        }
        /*if(!strcmp(k, ""))
        {
          linea = strtok(fichero, crlf);
          strcpy(fichero, linea);
        }*/
        //contador = contador + 1;
      }

      //Primer trozo
      strcpy(peticionCliente, orden);
      strcat(peticionCliente, " ");
      strcat(peticionCliente, fichero);
      strcat(peticionCliente, " ");
      strcat(peticionCliente, "HTML/1.1");
      strcat(peticionCliente, crlf);

      //Segundo trozo
      strcat(peticionCliente, "Host: ");
      strcat(peticionCliente, argv[1]);
      strcat(peticionCliente, crlf);

      //Tercer trozo
      if(k[0]=='k')
      {
        strcat(peticionCliente, "Connection: ");
        strcat(peticionCliente, "keep-alive");
        strcat(peticionCliente, crlf);
      }

      //Final
      strcat(peticionCliente, crlf);

      if(send(s, peticionCliente, strlen(peticionCliente), 0) != strlen(peticionCliente))
      {
        fprintf(stderr, "%s: Error en la funcion send()",	argv[0]);
  			exit(1);
      }
    }

    /* Despues de realizar la función send() llamamos a shutdown() y le
    * pasamos el parámetro 1 para cerrar el socket solo de nuestro lado,
    * es decir, le indicamos al servidor que no vamos a enviar más datos
    * por este lado, pero que sí estamos abiertos para recibir información
    */

  	if (shutdown(s, 1) == -1) {
  		perror(argv[0]);
  		fprintf(stderr, "%s: unable to shutdown socket\n", argv[0]);
  		exit(1);
  	}

    /* Despues de indicarle al servidor que no enviaremos más datos pero
    * que sí estamos abiertos a recibirlos, esperamos dicha información y
    * para ello usamos la función recv(), que devuelve 0 cuando se hayan enviado
    * todos los datos por parte del servidor y la conexión se haya cerrado por
    * su parte.
    */

  		if (recv(s, respuestaServidor, BUFFERSIZE, 0) == -1) {
        perror(argv[0]);
  			fprintf(stderr, "%s: error reading result\n", argv[0]);
  			exit(1);
  		}
  			/* The reason this while loop exists is that there
  			 * is a remote possibility of the above recv returning
  			 * less than BUFFERSIZE bytes.  This is because a recv returns
  			 * as soon as there is some data, and will not wait for
  			 * all of the requested data to arrive.  Since BUFFERSIZE bytes
  			 * is relatively small compared to the allowed TCP
  			 * packet sizes, a partial receive is unlikely.  If
  			 * this example had used 2048 bytes requests instead,
  			 * a partial receive would be far more likely.
  			 * This loop will keep receiving until all BUFFERSIZE bytes
  			 * have been received, thus guaranteeing that the
  			 * next recv at the top of the loop will start at
  			 * the begining of the next reply.
  			 */

  		printf("%s\n", respuestaServidor);


      /* Print message indicating completion of task. */
  	time(&timevar);
  	printf("All done at %s", (char *)ctime(&timevar));


  } else {

    //Aquí empieza el cliente de UDP

    /* Create the socket. */
      s = socket (AF_INET, SOCK_DGRAM, 0);
      if (s == -1) {
        perror(argv[0]);
        fprintf(stderr, "%s: unable to create socket\n", argv[0]);
        exit(1);
      }

        /* clear out address structures */
      memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
      memset ((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));

          /* Bind socket to some local address so that the
         * server can send the reply back.  A port number
         * of zero will be used so that the system will
         * assign any available port number.  An address
         * of INADDR_ANY will be used so we do not have to
         * look up the internet address of the local host.
         */
      myaddr_in.sin_family = AF_INET;
      myaddr_in.sin_port = 0;
      myaddr_in.sin_addr.s_addr = INADDR_ANY;
      if (bind(s, (const struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
        perror(argv[0]);
        fprintf(stderr, "%s: unable to bind socket\n", argv[0]);
        exit(1);
         }
        addrlen = sizeof(struct sockaddr_in);
        if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1) {
                perror(argv[0]);
                fprintf(stderr, "%s: unable to read socket address\n", argv[0]);
                exit(1);
        }

                /* Print out a startup message for the user. */
        time(&timevar);
                /* The port number must be converted first to host byte
                 * order before printing.  On most hosts, this is not
                 * necessary, but the ntohs() call is included here so
                 * that this program could easily be ported to a host
                 * that does require it.
                 */
        printf("Connected to %s on port %u at %s", argv[1], ntohs(myaddr_in.sin_port), (char *) ctime(&timevar));

      /* Set up the server address. */
      servaddr_in.sin_family = AF_INET;
        /* Get the host information for the server's hostname that the
         * user passed in.
         */
          memset (&hints, 0, sizeof (hints));
          hints.ai_family = AF_INET;
       /* esta funci�n es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta*/
        errcode = getaddrinfo (argv[1], NULL, &hints, &res);
        if (errcode != 0){
          /* Name was not found.  Return a
           * special value signifying the error. */
        fprintf(stderr, "%s: No es posible resolver la IP de %s\n",
            argv[0], argv[1]);
        exit(1);
          }
        else {
          /* Copy address of host */
        servaddr_in.sin_addr = ((struct sockaddr_in *) res->ai_addr)->sin_addr;
       }
         freeaddrinfo(res);
         /* puerto del servidor en orden de red*/
       servaddr_in.sin_port = htons(PUERTO);

       /* Registrar SIGALRM para no quedar bloqueados en los recvfrom */
        vec.sa_handler = (void *) handler;
        vec.sa_flags = 0;
        if ( sigaction(SIGALRM, &vec, (struct sigaction *) 0) == -1) {
                perror(" sigaction(SIGALRM)");
                fprintf(stderr,"%s: unable to register the SIGALRM signal\n", argv[0]);
                exit(1);
            }

        n_retry=RETRIES;

      while (n_retry > 0) {
        /* Send the request to the nameserver. */
            char orden[100];
            strcpy(orden, "GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n");
            if (sendto (s, argv[2], strlen(argv[2]), 0, (struct sockaddr *)&servaddr_in,
            sizeof(struct sockaddr_in)) == -1) {
                perror(argv[0]);
                fprintf(stderr, "%s: unable to send request\n", argv[0]);
                exit(1);
              }
        /* Set up a timeout so I don't hang in case the packet
         * gets lost.  After all, UDP does not guarantee
         * delivery.
         */
          alarm(TIMEOUT);
        /* Wait for the reply to come in. */
            if (recvfrom (s, &reqaddr, sizeof(struct in_addr), 0,
                (struct sockaddr *)&servaddr_in, &addrlen) == -1) {
            if (errno == EINTR) {
                /* Alarm went off and aborted the receive.
                 * Need to retry the request if we have
                 * not already exceeded the retry limit.
                 */
                 printf("attempt %d (retries %d).\n", n_retry, RETRIES);
                 n_retry--;
                        }
                else  {
            printf("Unable to get response from");
            exit(1);
                    }
                  }
            else {
                alarm(0);
                /* Print out response. */
                if (reqaddr.s_addr == ADDRNOTFOUND)
                   printf("Host %s unknown by nameserver %s\n", argv[2], argv[1]);
                else {
                    /* inet_ntop para interoperatividad con IPv6 */
                    if (inet_ntop(AF_INET, &reqaddr, hostname, MAXHOST) == NULL)
                       perror(" inet_ntop \n");
                    printf("Address for %s is %s\n", argv[2], hostname);
                    }
                break;
                }
      }

        if (n_retry == 0) {
           printf("Unable to get response from");
           printf(" %s after %d attempts.\n", argv[1], RETRIES);
           }
   }
}
