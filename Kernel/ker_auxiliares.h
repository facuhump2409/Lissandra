/*
 * ker_auxiliares.h
 *
 *  Created on: 17 jun. 2019
 *      Author: utnso
 */

#ifndef KER_AUXILIARES_H_
#define KER_AUXILIARES_H_
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <commonsPropias/serializacion.h>
#include <commonsPropias/conexiones.h>
#include <stdlib.h>
#include <semaphore.h>
#include <stdbool.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
#include "ker_structs.h"

/******************************DECLARACIONES******************************************/
bool recibidoContiene(char* recibido, char* contiene);
bool instruccion_no_ejecutada(instruccion* instruc);

void kernel_destroy();
void loggearErrorYLiberarParametrosEXEC(char* recibido, operacionLQL *opAux);
void loggearInfoYLiberarParametrosEXEC(char* recibido, operacionLQL *opAux);
void loggearInfoEXEC(char* estado, int threadProcesador, char* operacion);
void loggearErrorEXEC(char* estado, int threadProcesador, char* operacion);
void loggearErrorEXEC(char* estado, int threadProcesador, char* operacion);
void loggearInfoEXEC(char* estado, int threadProcesador, char* operacion);
void agregarALista(t_list* lista, void* elemento, pthread_mutex_t semaphore);
void guardarTablaCreada(char* parametros);
void eliminarTablaCreada(char* parametros);
void enviarJournal(int socket);

int socketMemoriaSolicitada(consistencia criterio);
int encontrarSocketDeMemoria(int numero);
int realizarConexion(memoria* mem);

tabla* encontrarTablaPorNombre(char* nombre);

memoria* encontrarMemoria(int numero);
memoria* encontrarMemoriaStrong();

consistencia encontrarConsistenciaDe(char* nombreTablaBuscada);
int obtenerListaDeConsistencia(consistencia unaConsistencia);
int obtenerSocket(operacionLQL* opAux,int index);
int enviarOperacion(operacionLQL* opAux,int index);

/******************************IMPLEMENTACIONES******************************************/
int enviarOperacion(operacionLQL* opAux,int index){
	int socket = obtenerSocket(opAux,index);
	if(socket != -1){
		serializarYEnviarOperacionLQL(socket, opAux);
		char* recibido = (char*) recibir(socket);
		if(recibidoContiene(recibido, "ERROR")){
			loggearErrorYLiberarParametrosEXEC(recibido,opAux);
			cerrarConexion(socket);
			return -1;
		}
		else{
			while(recibidoContiene(recibido, "FULL")){
				enviarJournal(socket);
				serializarYEnviarOperacionLQL(socket, opAux);
				recibido = (char*) recibir(socket);
			}
			loggearInfoYLiberarParametrosEXEC(recibido,opAux);
			cerrarConexion(socket);
			return 1;
		}
	}
	else{
		pthread_mutex_lock(&mLog);
		log_info(kernel_configYLog->log, "ERROR: No hay memorias para enviar la request %s %s", opAux->operacion, opAux->parametros);
		pthread_mutex_unlock(&mLog);
		return -1;
	}
}
int obtenerSocket(operacionLQL* opAux,int index){
	int socket = -1;
	bool pudeConectarYEnviar(memoria* mem){
		if((socket = crearSocketCliente(mem->ip,mem->puerto))){
			serializarYEnviarOperacionLQL(socket, opAux);
			pthread_mutex_lock(&mLog);
			log_info(kernel_configYLog->log, "ENVIADO: %s %s", opAux->operacion, opAux->parametros);
			pthread_mutex_unlock(&mLog);
			return true;
		}
		else{
			bool memoriaASacar(memoria* mem2){
				return mem2->numero == mem->numero;
			}
			void freeMemoria(memoria* mem3){
				free(mem3->ip);
				free(mem3->puerto);
				free(mem3);
			}
		 	list_remove_and_destroy_by_condition(criterios[index].memorias,(void*)memoriaASacar, (void*)freeMemoria);
			return false;
		}
	}
	list_find(criterios[index].memorias,(void*)pudeConectarYEnviar);
	return socket;
}
int obtenerListaDeConsistencia(consistencia unaConsistencia){
	if(unaConsistencia == SC){
		return STRONG;
	}
	else if(unaConsistencia == EC){
		return EVENTUAL;
	}
	else if(unaConsistencia == SH){
		return HASH;
	}
	return -1;
}
//------ OPERACIONES LQL ---------
void enviarJournal(int socket){
	operacionLQL* opAux=splitear_operacion("JOURNAL");
	serializarYEnviarOperacionLQL(socket, opAux);
	pthread_mutex_lock(&mLog);
	log_info(kernel_configYLog->log, "ENVIADO: JOURNAL");
	pthread_mutex_unlock(&mLog);
	char* recibido = (char*) recibir(socket);
	loggearInfoYLiberarParametrosEXEC(recibido,opAux);
	free(recibido);
	liberarOperacionLQL(opAux);
}
//------ INSTRUCCIONES DE PCB ---------
bool instruccion_no_ejecutada(instruccion* instruc){
	return instruc->ejecutado==0;
}
//------ TABLAS ---------
void guardarTablaCreada(char* parametros){
	char** opAux =string_n_split(parametros,3," ");
	tabla* tablaAux = malloc(sizeof(tabla));
	tablaAux->nombreDeTabla= *opAux;
	if(string_equals_ignore_case(*(opAux+1),"SC")){
		tablaAux->consistenciaDeTabla = SC;
	}
	else if(string_equals_ignore_case(*(opAux+1),"SH")){
		tablaAux->consistenciaDeTabla = SH;
	}
	else if(string_equals_ignore_case(*(opAux+1),"EC")){
		tablaAux->consistenciaDeTabla = EC;
	}
	list_add(tablas,tablaAux);
}
void eliminarTablaCreada(char* parametros){
	tabla* tablaAux = malloc(sizeof(tabla));
	bool tablaDeNombre(tabla* t){
			return t->nombreDeTabla == parametros;
		}
	tablaAux = list_remove_by_condition(tablas, (void*)tablaDeNombre);
	free(tablaAux->nombreDeTabla);
	free(tablaAux);
}
tabla* encontrarTablaPorNombre(char* nombre){
	bool tablaDeNombre(tabla* t){
			return t->nombreDeTabla == nombre;
		}
	return list_find(tablas,(void* ) tablaDeNombre);
}
//------ MEMORIAS ---------
memoria* encontrarMemoria(int numero){
	bool memoriaEsNumero(memoria* mem) {
		return mem->numero == numero;
	}
	memoria * memory = malloc(sizeof(memoria));
	memory = (memoria*) list_find(conexionesMemoria, (void*)memoriaEsNumero);
	return memory;
}
memoria* encontrarMemoriaStrong(){
	return list_get(criterios[STRONG].memorias, 0);
}
//------ CRITERIOS ---------
consistencia encontrarConsistenciaDe(char* nombreTablaBuscada){
	bool encontrarTabla(tabla t){
		return t.nombreDeTabla == nombreTablaBuscada;
	}
	tabla retorno =*(tabla*) list_find(tablas,(void*)encontrarTabla);
	return retorno.consistenciaDeTabla;
}
//------ CONEXION ---------
int realizarConexion(memoria* mem){
	return crearSocketCliente(mem->ip,mem->puerto);
}
//int encontrarSocketDeMemoria(int numero){
//	bool encontrarSocket(memoria* unaConex){
//		return unaConex->numero == numero;
//	}
//	memoria* mem = list_find(conexionesMemoria,(void*) encontrarSocket);
//	return mem->socket;
//}
//
//int socketMemoriaSolicitada(consistencia criterio){
//	memoria* mem = NULL;
//	switch (criterio){
//
//		case SC:
//			mem = encontrarMemoriaStrong();
//			break;
//		case SH:
//
//			break;
//		case EC:
//			break;
//	}
//
//	return encontrarSocketDeMemoria(mem->numero);
//}
//------ ERRORES ---------
bool recibidoContiene(char* recibido, char* contiene){
	string_to_upper(recibido);
	return string_contains(recibido, contiene);
}
//------ CERRAR ---------
void kernel_destroy(){
	destroy = 1;
}
//----------------- LOGS -----------------------------
void loggearErrorYLiberarParametrosEXEC(char* recibido, operacionLQL *opAux){
	pthread_mutex_lock(&mLog);
	log_error(kernel_configYLog->log, "RECIBIDO: %s", recibido);
	pthread_mutex_unlock(&mLog);
	free(recibido);
	liberarOperacionLQL(opAux);
}
void loggearInfoYLiberarParametrosEXEC(char* recibido, operacionLQL *opAux){
	pthread_mutex_lock(&mLog);
	log_info(kernel_configYLog->log, "RECIBIDO: %s", recibido);
	pthread_mutex_unlock(&mLog);
	free(recibido);
	liberarOperacionLQL(opAux);
}
void loggearErrorEXEC(char* estado, int threadProcesador, char* operacion){
	pthread_mutex_lock(&mLog);
	log_error(kernel_configYLog->log,"%s[%d]: %s",estado,threadProcesador, operacion);
	pthread_mutex_unlock(&mLog);
}
void loggearInfoEXEC(char* estado, int threadProcesador, char* operacion){
	pthread_mutex_lock(&mLog);
	log_info(kernel_configYLog->log,"%s[%d]: %s",estado,threadProcesador, operacion);
	pthread_mutex_unlock(&mLog);
}
//------ LISTAS ---------
void agregarALista(t_list* lista, void* elemento, pthread_mutex_t semaphore){
	pthread_mutex_lock(&semaphore);
	list_add(lista,elemento);
	pthread_mutex_unlock(&semaphore);
}

#endif /* KER_AUXILIARES_H_ */