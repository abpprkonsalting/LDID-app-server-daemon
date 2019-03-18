/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * main.cc
 * Copyright (C) armando 2012 <armando.banos@cneuro.edu.cu>
 * 
 * main.cc is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * main.cc is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include "main.h"

int main()
{
	returnStatus = 0;
	
	nombre_servidor = new char[100];
	bzero((char*)nombre_servidor, 100);
	
	mensaje = new char[224];
	bzero((char*)mensaje, 224);
	
	Message = (char*)malloc(224);
	bzero((char*)Message, 224);
	
	comando = (char*)malloc(24);		// 23 caracteres del comando + espacio para el \0.
	bzero((char*)comando, 24);
		
	bzero(&direccion_servidor, sizeof(direccion_servidor));
	direccion_servidor.sin_family = AF_INET;
	
	puerto_escucha = 0;
	
//*******************************************************************************************************************
// Inicializaci'on del archivo debug principal
//*******************************************************************************************************************
	
	/* (Por hacer) Primero debo chequear el tama~no del archivo debug (si existe), si es muy grande renombrarlo y crear uno nuevo*/
	
	nombre_archivo_debug = new char[100];
	bzero((char*)nombre_archivo_debug, 100);
	strcpy(nombre_archivo_debug,"/root/ldid_inf.log");
	
	debug_file = fopen(nombre_archivo_debug, "a");
	if (debug_file == NULL)
	{
  		fprintf(stderr, "error abriendo el archivo de debug de la aplicación LDID!\n");	// Esto debo escribirlo para un archivo log del sistema, no s'e como lo har'e pero debe ser posible.
  		return(1);
	}
	fprintf(stderr, "archivo debug de la aplicación LDID informer abierto correctamente!\n");
	
//*******************************************************************************************************************
// lectura de parametros de trabajo desde el archivo de configuracion
//*******************************************************************************************************************
	
	nombre_archivo_configuracion = new char[100];
	bzero((char*)nombre_archivo_configuracion, 100);
	strcpy(nombre_archivo_configuracion,"/etc/ldid_inf/settings");
	returnStatus = lee_parametros(nombre_archivo_configuracion,&direccion_servidor,nombre_servidor,&puerto_escucha,(int*)&(timer.it_interval.tv_sec));
	
	strcpy(mensaje,"###REGISTRO_SERVIDOR&&:");
	strcat(mensaje,nombre_servidor);
	
//*******************************************************************************************************************
// Creaci'on del socket a trav'es del cual se va a enviar el mensaje de registro y se van a recibir las confirmaciones
//*******************************************************************************************************************
	
	listener_general = 0;
	addrlen = 0;
	tamanno_mensaje = 0;
	tamanno_datos = 0;
	semaforo = true;
	
	listener_general = socket(AF_INET, SOCK_DGRAM, 0);
	
    if (listener_general == -1) 
	{
        fprintf(debug_file, "Could not create a socket!\n");
		fflush(debug_file);
		fclose(debug_file);
        return(1);
    }
    else 
	{
        fprintf(debug_file, "Socket created!\n");
		fflush(debug_file);
    }
	
	// lose the pesky "address already in use" error message
	int yes=1;         // for setsockopt() SO_REUSEADDR, below
    if (setsockopt(listener_general, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int)) == -1) 
	{
        perror("setsockopt");
        return(1);
    }
		
	/* set up the address structure */
	/* use INADDR_ANY to bind to all local addresses   */

	bzero(&remoteaddr, sizeof(remoteaddr));
	bzero(&server_addr_general, sizeof(server_addr_general));
	server_addr_general.sin_family = AF_INET;
	server_addr_general.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr_general.sin_port = htons(puerto_escucha);

	/*  bind to the address and port with our socket   */

	returnStatus = bind(listener_general,(struct sockaddr*)&server_addr_general,sizeof(server_addr_general));
	if (returnStatus == 0)
	{
		fprintf(debug_file, "Bind completed!\n");
		fflush(debug_file);
	}
	else 
	{
		fprintf(debug_file, "Could not bind to address!\n");
		fflush(debug_file);
		fclose(debug_file);
		close(listener_general);
		return(1);
	}
	
//*******************************************************************************************************************
// Creacion del timer que interrumpe cada X segundos para enviar los mensajes de registro de direccion.
//*******************************************************************************************************************
	
	/* Install timer_handler as the signal handler for SIGVTALRM. */
	memset (&sa, 0, sizeof (sa));
	sa.sa_handler = &timer_handler;
	sigaction (  SIGALRM, &sa, NULL);
	
	/* Configure the timer to expire after 3600 s */
	timer.it_value.tv_sec = 1;
	timer.it_value.tv_usec = 0;
	/* ... and every 3600 s after that. */
	//timer.it_interval.tv_sec = 10;
	timer.it_interval.tv_usec = 0;
	setitimer (  ITIMER_REAL, &timer, NULL);
	
	
//*******************************************************************************************************************
// Ciclo de espera por mensajes que lleguen en el puerto de escucha
//*******************************************************************************************************************
	
	while (1)
	{
		addrlen = sizeof(remoteaddr);
		bzero((char*)Message, 224);
		returnStatus = recvfrom(listener_general, (char*)Message, 224, 0,(struct sockaddr*)&remoteaddr, &addrlen);
		if ((returnStatus == -1) && (!retorno_timer))
		{
    		fprintf(debug_file, "Could not receive message!\n");
			fflush(debug_file);
			retorno_timer = false;
		}
		else if (returnStatus != -1)
		{
			semaforo = false;	// Si el timer interrumpe a partir de este momento no har'a nada.
			
			char* final = Message + returnStatus - 1;
			*final = 0;
			
			// Las dos líneas de arriba son para garantizar que el último caracter que está en el buffer Message es un \0
			
			if (htonl(remoteaddr.sin_addr.s_addr) == htonl(direccion_servidor.sin_addr.s_addr))
			{
				//Extraer el encabezamiento de comando del mensaje recibido.
				bzero((char*)comando, 24);
				strncpy((char*)comando,(char*)Message,23);
			
				comp_string = strcmp((char*)comando,"###RECIBIDO_OK&&&&&&&&&");
				if (comp_string == 0)
				{
					fprintf(debug_file, "Recibida una confirmacion de mensaje desde el servidor LDID\n");
					fflush(debug_file);
				}
			}

			semaforo = true;		// A partir de aquí se atienden las interrupciones del timer de nuevo.
		}
	}
	close(listener_general);
	fclose (debug_file);
	return 0;
}

int lee_parametros(char* archivo,sockaddr_in* ptr_direccion_servidor,char* nombre,int* puerto_c,int* tiempo)
{
//*******************************************************************************************************************
// Funcion de lectura de parametros de trabajo
//*******************************************************************************************************************

	ssize_t bytes_read;
	size_t a_leer = 100;
	char* direccion_servidor_text = new char[100];
	bzero((char*)direccion_servidor_text, 100);
	char* puerto_servidor = new char[100];
	bzero((char*)puerto_servidor, 100);
	int len;
	char* comentario = new char[100];
	//bzero((char*)comentario, 100);

	FILE* archivo1 = fopen(archivo, "r+ t");
	if (archivo1 == NULL)
	{
  		/* open failed. Deallocate buffer before returning. */
  		fprintf(debug_file, "error abriendo el archivo de configuracion!\n");
		fflush(debug_file);
  		return(1);
	}
	fprintf(debug_file, "archivo de configuracion abierto correctamente!\n");
	fflush(debug_file);
	
	char* buffer_port_c = (char*) malloc (6);
	if (buffer_port_c == NULL)
	{
		fprintf(debug_file, "no se pudo crear la variable buffer_port_c!\n");
		fflush(debug_file);
 		return(1);
	}
	char* tiempo_text = (char*) malloc (50);
	if (tiempo_text == NULL)
	{
		fprintf(debug_file, "no se pudo crear la variable tiempo_text!\n");
		fflush(debug_file);
 		return(1);
	}
	
	/* Read the data. */
	int compara;
	do
	{
		bytes_read = getline (&direccion_servidor_text,&a_leer,archivo1);
		bzero((char*)comentario, 100);
		strcpy(comentario,direccion_servidor_text);
		strcpy((char*)(comentario+1),"\0");
		compara = strcmp((char*)comentario,"#");
	} while (compara == 0);
	len = strlen(direccion_servidor_text);
	strcpy((char*)(direccion_servidor_text+len-1),"\0");
	
	ptr_direccion_servidor->sin_addr.s_addr = inet_addr(direccion_servidor_text);
	
	bytes_read = getline (&puerto_servidor,&a_leer,archivo1);
	len = strlen(puerto_servidor);
	strcpy((char*)(puerto_servidor+len-1),"\0");
	
	ptr_direccion_servidor->sin_port  = htons(atoi(puerto_servidor));
	
	bytes_read = getline (&nombre,&a_leer,archivo1);
	len = strlen(nombre);
	strcpy((char*)(nombre+len-1),"\0");
	
	bytes_read = getline (&buffer_port_c,&a_leer,archivo1);
	len = strlen(buffer_port_c);
	strcpy((char*)(buffer_port_c+len-1),"\0");
	
	*puerto_c = atoi(buffer_port_c);
	
	bytes_read = getline (&tiempo_text,&a_leer,archivo1);
	len = strlen(tiempo_text);
	strcpy((char*)(tiempo_text+len-1),"\0");
	
	*tiempo = atoi(tiempo_text);
		
	fprintf(stderr,"direccion: %s\n",direccion_servidor_text);
	fprintf(stderr,"puerto: %s\n",puerto_servidor);
	fprintf(stderr,"nombre del servidor: %s\n",nombre);
	fprintf(stderr,"puerto de recepcion de mensajes: %s\n",buffer_port_c);
	fprintf(stderr,"tiempo entre actualizaciones: %s\n",tiempo_text);
	fflush(debug_file);
	

	/* Everything’s fine. Close the file and return the buffer. */
	free(buffer_port_c);
	free(tiempo_text);
	fclose (archivo1);
	return(0);
}

void timer_handler(int signum)
{
//******************************************************************************************************************
// Esta funci'on interrumpe cada x segundos para enviar al servidor el mensaje de registro. 
//******************************************************************************************************************

	if (semaforo)	// El sem'aforo debe estar en verde para poder entrar en esta funcion (esto no hac'ia falta pero no est'a de m'as)
	{
	
		returnStatus = sendto(listener_general, (char*)mensaje, strlen((char*)mensaje)+1
						  ,0,(struct sockaddr*)&direccion_servidor,sizeof((struct sockaddr_in)direccion_servidor));
		if (returnStatus == -1)
		{
			fprintf(debug_file,"No se pudo enviar el mensaje: %s al servidor con direccion %s\n",(char*)mensaje,inet_ntoa(direccion_servidor.sin_addr));
			fflush(debug_file);
		}
		else
		{
			fprintf(debug_file,"Enviado el mensaje: %s al servidor con direccion %s\n",(char*)mensaje,inet_ntoa(direccion_servidor.sin_addr));
			fflush(debug_file);
		}
		retorno_timer = true;
	}
}
