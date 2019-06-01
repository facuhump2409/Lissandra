/*
 * memoria.c
 *
 *  Created on: 7 abr. 2019
 *      Author: whyAreYouRunning?
 */

#include <readline/readline.h>
#include <string.h>
#include <stdio.h>
#include <commonsPropias/conexiones.h>
#include <commonsPropias/serializacion.h>
#include "operacionesMemoria.h"

void APIProtocolo(void* buffer, memoria* memoriaPrincipal, configYLogs* configYLog, int socket) {
	operacionProtocolo operacion = empezarDeserializacion(&buffer);

	switch(operacion) {
	case OPERACIONLQL:
		log_info(configYLog->logger, "Recibi una operacionLQL, a ver que es?");
		APIMemoria(deserializarOperacionLQL(buffer), memoriaPrincipal, configYLog, socket);
		break;
	}
}

void APIMemoria(operacionLQL* operacionAParsear, memoria* memoriaPrincipal, configYLogs* configYLog, int socketKernel) {
	if(string_starts_with(operacionAParsear->operacion, "INSERT")) {
		log_info(configYLog->logger, "Recibi un INSERT");
		insertLQL(operacionAParsear, configYLog, memoriaPrincipal);
		selectLQL(operacionAParsear, configYLog, memoriaPrincipal, socketKernel);
	}
	else if (string_starts_with(operacionAParsear->operacion, "SELECT")) {
		log_info(configYLog->logger, "Recibi un SELECT");
		selectLQL(operacionAParsear, configYLog, memoriaPrincipal, socketKernel);
	}
	else if (string_starts_with(operacionAParsear->operacion, "DESCRIBE")) {
		log_info(configYLog->logger, "Recibi un DESCRIBE");
	}
	else if (string_starts_with(operacionAParsear->operacion, "CREATE")) {
		log_info(configYLog->logger, "Recibi un CREATE");
	}
	else if (string_starts_with(operacionAParsear->operacion, "DROP")) {
		log_info(configYLog->logger, "Recibi un DROP");
	}
	else if (string_starts_with(operacionAParsear->operacion, "JOURNAL")) {
		log_info(configYLog->logger, "Recibi un JOURNAL");
		}
	else {
		log_error(configYLog->logger, "No pude entender la operacion");
	}
}
//pthread_mutex_t mutexBufferLFS = 0;

//------------------------------------------------------------------------

void liberarConfigYLogs(configYLogs *archivos) {
	config_destroy(archivos->config);
	log_destroy(archivos->logger);
	free(archivos);
}

datosInicializacion* realizarHandshake(configYLogs* configYLog) {
	socketLissandraFS = crearSocketLFS(configYLog);
	operacionProtocolo operacionHandshake = HANDSHAKE;
	//void* bufferParaEnviarOperacion = serializarHandshake((int) operacionHandshake);
	//enviar(socketLissandraFS, (void*)operacionHandshake, sizeof(int)*2);

	void* bufferHandshake = recibir(socketLissandraFS);

	datosInicializacion* datosImportantes = malloc(sizeof(datosInicializacion));
	datosImportantes->tamanio = deserializarHandshake(bufferHandshake);
	return datosImportantes;
	/*datosInicializacion* datosImportantes = malloc(sizeof(datosInicializacion));
	datosImportantes->tamanio = 2048;
	return datosImportantes;*/
}

void liberarDatosDeInicializacion(datosInicializacion* datos) {
	free(datos);
}

int crearSocketLFS(configYLogs* archivosDeConfigYLog) {
	char* fileSystemIP = config_get_string_value(archivosDeConfigYLog->config, "IPFILESYSTEM");
	char* fileSystemPuerto = config_get_string_value(archivosDeConfigYLog->config, "PUERTOFILESYSTEM");

	int socketClienteLFS = crearSocketCliente(fileSystemIP,fileSystemPuerto);
	return socketClienteLFS;
}

void *servidorMemoria(void* arg, memoria* memoriaPrincipal){
	configYLogs *archivosDeConfigYLog = (configYLogs*) arg;

	int socketServidorMemoria = crearSocketServidor(config_get_string_value(archivosDeConfigYLog->config, "IPMEMORIA"), config_get_string_value(archivosDeConfigYLog->config, "PUERTO"));

	if(socketServidorMemoria == -1) {
		cerrarConexion(socketServidorMemoria);
		log_error(archivosDeConfigYLog->logger, "No se pudo inicializar el servidor de memoria");
		pthread_exit(0);
	}

	log_info(archivosDeConfigYLog->logger, "Servidor Memoria en linea");

	while(1){
		int socketKernel = aceptarCliente(socketServidorMemoria);
		if(socketKernel == -1) {
			log_error(archivosDeConfigYLog->logger, "ERROR: Socket Defectuoso");
			continue;
		}
		//void* buffer = envioDePrueba("hola");
		void* mensaje = recibir(socketKernel); // interface( deserializarOperacion( buffer , 1 ) )
		log_info(archivosDeConfigYLog->logger, "Recibi algo, A parsear!");
		APIProtocolo(mensaje, memoriaPrincipal, archivosDeConfigYLog, socketKernel);

		free(mensaje);
		cerrarConexion(socketKernel);
	}

	cerrarConexion(socketServidorMemoria);

}

void pedirALFS(operacionLQL* operacion){

	//pthread_mutex_lock(&mutexBufferLFS);
		// bufferConLFS = serializarOperacionLQL(operacion);
		//socketLFS(bufferConLFS);
	//pthread_mutex_unlock(&mutexBufferLFS);

}

int main() {
	pthread_t threadServer; //threadCliente, threadTimedJournal, threadTimedGossiping;
	configYLogs *archivosDeConfigYLog = malloc(sizeof(configYLogs));

	archivosDeConfigYLog->config = config_create("memoria.config");
	archivosDeConfigYLog->logger = log_create("memoria.log", "MEMORIA", 1, LOG_LEVEL_INFO);

	datosInicializacion* datosDeInicializacion = realizarHandshake(archivosDeConfigYLog);

	memoria* memoriaPrincipal = inicializarMemoria(datosDeInicializacion, archivosDeConfigYLog);
	liberarDatosDeInicializacion(datosDeInicializacion);

	servidorMemoria((void*) archivosDeConfigYLog, memoriaPrincipal);
	//pthread_create(&threadServer,NULL,servidorMemoria,(void*) archivosDeConfigYLog);
	//pthread_create(&threadCliente, NULL, clienteKernel, archivosDeConfigYLog);
	//pthread_create(&threadTimedJournal, NULL, timedJournal, archivosDeConfigYLog);
	//pthread_create(&threadTimedGossiping, NULL, timedGossip, archivosDeConfigYLog);

	//pthread_join(threadServer, NULL);
	//pthread_detach(threadCliente);
	//pthread_detach(threadTimedJournal);
	//pthread_detach(threadTimedGossiping);

	liberarMemoria(memoriaPrincipal);
	liberarConfigYLogs(archivosDeConfigYLog);
	return 0;
}



