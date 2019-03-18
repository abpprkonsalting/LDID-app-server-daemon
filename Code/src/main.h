#include <iostream>
#include <fstream>
#include <istream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <signal.h>


using namespace std;

int returnStatus;			// Variable para recibir el resultado de las funciones.
FILE* debug_file;
char* nombre_archivo_debug;
char* nombre_archivo_configuracion;
char* nombre_servidor;
char* mensaje;
sockaddr_in direccion_servidor;

char* Message;
int puerto_escucha;
char* comando;
int listener_general;
socklen_t addrlen;
size_t tamanno_mensaje;
size_t tamanno_datos;
bool semaforo;
sockaddr_in remoteaddr;
sockaddr_in server_addr_general;

int socket_envio;

struct sigaction sa;
struct itimerval timer;
bool retorno_timer;
int comp_string;


int lee_parametros(char* archivo,sockaddr_in* ptr_direccion_servidor,char* nombre,int* puerto_c,int* tiempo);
void timer_handler(int signum);
