/*
 * memoria.c
 *
 *  Created on: 7 abr. 2019
 *      Author: whyAreYouRunning?
 */

#include <readline/readline.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <inttypes.h>
#include <stdlib.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commonsPropias/conexiones.h>
#include <pthread.h>


#define TAMANIOSEGMENTO 10 // esto va a estar en un archivo de config

typedef struct {
	t_config* config;
	t_log* logger;
} configYLogs;

char* pruebaDeRecepcion(void* buffer) {
	return (char*) buffer;
}

void envioDePrueba(int socketServidor, char* mensajeAEnviar) {

	int tamanioMensajeAEnviar = strlen(mensajeAEnviar) + 1;
	enviar(socketServidor, (void*) mensajeAEnviar, tamanioMensajeAEnviar);
}



void liberarConfigYLogs(configYLogs *archivos) {
	log_destroy(archivos->logger);
	config_destroy(archivos->config);
	free(archivos);
}

void *clienteLFS(void* arg) {
	configYLogs *archivosDeConfigYLog = (configYLogs*) arg;

	char* fileSystemIP = config_get_string_value(archivosDeConfigYLog->config, "IPFILESYSTEM");
	char* fileSystemPuerto = config_get_string_value(archivosDeConfigYLog->config, "PUERTOFILESYSTEM");

	int socketClienteKernel = crearSocketCliente(fileSystemIP,fileSystemPuerto);

	char* mensaje = (char*) arg;
	enviar(socketClienteKernel, mensaje, (strlen(mensaje)+1)*sizeof(char));

	cerrarConexion(socketClienteKernel);
}

void *servidorMemoria(void* arg){
	configYLogs *archivosDeConfigYLog = (configYLogs*) arg;

	int socketServidorMemoria = crearSocketServidor(config_get_string_value(archivosDeConfigYLog->config, "PUERTO"));

	if(socketServidorMemoria == -1) {
		cerrarConexion(socketServidorMemoria);
		pthread_exit(0);
	}

	log_info(archivosDeConfigYLog->logger, "Servidor Memoria en linea");

	while(1){
		int socketKernel = aceptarCliente(socketServidorMemoria);
//
		if(socketKernel == -1) {
			log_info(archivosDeConfigYLog->logger, "ERROR: Socket Defectuoso");
			continue;
		}


		//void* buffer = envioDePrueba("hola");
		char* mensaje = (char*) recibir(socketKernel); // interface( deserializarOperacion( buffer , 1 ) )

		clienteLFS((void*) mensaje);


		log_info(archivosDeConfigYLog->logger, "Recibi: %s", mensaje);

		free(mensaje);
		cerrarConexion(socketKernel);
	}

	cerrarConexion(socketServidorMemoria);

}

int main() {
	pthread_t threadServer; //threadCliente, threadTimedJournal, threadTimedGossiping;
	configYLogs *archivosDeConfigYLog = malloc(sizeof(configYLogs));


	archivosDeConfigYLog->config = config_create("../memoria.config");
	archivosDeConfigYLog->logger = log_create("memoria.log", "MEMORIA", 1, LOG_LEVEL_INFO);

	pthread_create(&threadServer,NULL,servidorMemoria,(void*) archivosDeConfigYLog);
	//pthread_create(&threadCliente, NULL, clienteKernel, archivosDeConfigYLog);
	//pthread_create(&threadTimedJournal, NULL, timedJournal, archivosDeConfigYLog);
	//pthread_create(&threadTimedGossiping, NULL, timedGossip, archivosDeConfigYLog);

	pthread_join(threadServer, NULL);
	//pthread_detach(threadCliente);
	//pthread_detach(threadTimedJournal);
	//pthread_detach(threadTimedGossiping);

	//liberarConfigYLogs(archivosDeConfigYLog); Esta dando segfault
	return 0;
}



