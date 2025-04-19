#ifndef COMPORTAMIENTORESCATADOR_H
#define COMPORTAMIENTORESCATADOR_H

#include <chrono>
#include <time.h>
#include <thread>

#include "comportamientos/comportamiento.hpp"

class ComportamientoRescatador : public Comportamiento
{

public:
	/**
	 * @brief Constructor por defecto para los niveles 0,1,4
	 * @param size Tamaño del mapa
	 */
	ComportamientoRescatador(unsigned int size = 0) : Comportamiento(size)
	{
		// Inicializar Variables de Estado Niveles 0,1,4 (constructor)

		last_action = IDLE;
		tiene_zapatillas = false;
		acciones_pendientes.resize(4, 0);
		num_acciones = 0;
		decision = -1;
		contador = 0;
		baneados.resize(16, false);
		mapaFrecuencias.resize(size, vector<int>(size, 0));
	}

	/**
	 * @brief Constructor para los niveles 2,3
	 * @param mapaR Mapa de terreno
	 * @param mapaC Mapa de alturas
	 */
	ComportamientoRescatador(std::vector<std::vector<unsigned char>> mapaR, std::vector<std::vector<unsigned char>> mapaC) : Comportamiento(mapaR, mapaC)
	{
		// Inicializar Variables de Estado Niveles 2,3
	}

	/**
	 * @brief Constructor de copia
	 * @param comport Comportamiento a copiar
	 */
	ComportamientoRescatador(const ComportamientoRescatador &comport) : Comportamiento(comport) {}

	/**
	 * @brief Destructor
	 */
	~ComportamientoRescatador() {}

	/**
	 * @brief Función principal del comportamiento
	 * @param sensores Sensores del agente
	 * @return Acción a realizar
	 */
	Action think(Sensores sensores);

	/**
	 * @brief Interactúa con el entorno
	 * @param accion Acción a realizar
	 * @param valor Valor de la acción
	 * @return Resultado de la interacción
	 */
	int interact(Action accion, int valor);

	/**
	 * @brief Convierte la posición de la casilla en el mapa a la posición de la casilla en el mapa
	 * @param i Posición de la casilla en el mapa
	 * @param rumbo Dirección del sensor
	 * @param orig Posición del agente
	 * @return Posición de la casilla en el mapa
	 */
	pair<int,int> VtoM(int i, Orientacion rumbo, const pair<int, int> & orig);

	/**
	 * @brief Coloca el sensor en el mapa (lo "pinta")
	 * @param sensores Sensores del agente
	 */
	void SituarSensorEnMapa(const Sensores & sensores);

	/**
	 * @brief Comprueba si tengo tareas pendientes
	 * @param sensores Sensores del agente
	 * @param action Acción a realizar
	 * @return Si tengo tareas pendientes
	 */
	bool TengoTareasPendientes(const Sensores & sensores, Action & action);

	/**
	 * @brief Comprueba si la casilla es viable por altura
	 * @param dif Diferencia de altura
	 * @param zap Si el agente tiene zapatillas
	 * @return Si la casilla es viable por altura
	 */
	bool ViablePorAltura(int dif, bool zap);
	
	/**
	 * @brief Obtiene las casillas interesantes visibles por el agente
	 * @param sensores Sensores del agente
	 * @param accesible Casillas accesibles
	 * @param transitable Casillas transitables
	 * @param is_interesting Casillas interesantes. Parámetro de salida
	 * @param casillas_interesantes Casillas interesantes. Parámetro de salida
	 * @param zap Si el agente tiene zapatillas
	 * 
	 * @note Las casillas seleccionadas son enteros relativos a la posición del agente:
	 * 
	 * 		9	10	11	12	13	14	15
	 * 			4	5	6	7	8
	 * 				1	2	3
	 * 					^		
	 */
	void CasillasInteresantes (const Sensores & sensores, const vector<bool> & accesible, const vector<bool> & transitable,
		vector<bool> & is_interesting, vector<int> & casillas_interesantes, bool zap);

	/**
	 * @brief Obtiene las casillas interesantes alrededor del agente
	 * @param orig Posición del agente
	 * @param accesible Casillas accesibles
	 * @param transitable Casillas transitables
	 * @param is_interesting Casillas interesantes. Parámetro de salida
	 * @param casillas_interesantes Casillas interesantes. Parámetro de salida
	 * @param zap Si el agente tiene zapatillas
	 * 
	 * @note Las casillas seleccionadas son enteros relativos a la posición del agente:
	 * 
	 * 		15		8		9
	 * 			7	0	1	
	 * 		14	6	^	2	10
	 * 			5	4	3	
	 * 		13		12		11
	 */
	void CasillasInteresantesAllAround(const pair<int,int> & orig, const vector<bool> & accesible, 
		const vector<bool> & transitable, vector<bool> & is_interesting, vector<int> & casillas_interesantes, bool zap);
	/**
	 * @brief Selecciona la casilla más interesante
	 * @param sensores Sensores del agente
	 * @param casillas_interesantes Casillas interesantes
	 * @param is_interesting Casillas interesantes
	 * @param zap Si el agente tiene zapatillas
	 * @return Casilla seleccionada
	 * 
	 * @note La casilla seleccionada es un entero relativo a la posición del agente:
	 * 
	 * 		9	10	11	12	13	14	15
	 * 			4	5	6	7	8
	 * 				1	2	3
	 * 					^	
	 */
	int SelectCasilla (const Sensores & sensores, const vector<int> & casillas_interesantes, 
			const vector<bool> & is_interesting);
	
	/**
	 * @brief Selecciona la casilla más interesante de entre las que puede ir con un giro y un avance
	 * @param orig Posición del agente
	 * @param casillas_interesantes Casillas interesantes
	 * @param is_interesting Casillas interesantes
	 * @param rumbo Dirección en la que está mirando el agente
	 * @return Casilla seleccionada
	 * 
	 * @note La casilla seleccionada es un entero relativo a la posición del agente:
	 * 
	 * 		15		8		9
	 * 			7	0	1	
	 * 		14	6	^	2	10
	 * 			5	4	3	
	 * 		13		12		11
	 * 		
	 */
	int SelectCasillaAllAround (const pair<int,int> & orig, const vector<int> & casillas_interesantes, 
		const vector<bool> & is_interesting, Orientacion rumbo, bool zap);
	
	/**
	 * @brief Selecciona la acción a realizar
	 * @param decision Casilla seleccionada
	 * @param rumbo Dirección del sensor
	 * @return Acción a realizar
	 * 
	 * @note La casilla seleccionada @p decision es un entero relativo a la posición del agente:
	 * 
	 * 		15		8		9
	 * 			7	0	1	
	 * 		14	6	^	2	10
	 * 			5	4	3	
	 * 		13		12		11
	 */
	Action SelectAction(int decision, Orientacion rumbo);

	Action ComportamientoRescatadorNivel_0(Sensores sensores);
	Action ComportamientoRescatadorNivel_1(Sensores sensores);
	Action ComportamientoRescatadorNivel_2(Sensores sensores);
	Action ComportamientoRescatadorNivel_3(Sensores sensores);
	Action ComportamientoRescatadorNivel_4(Sensores sensores);

private:
	// Variables de Estado

	// Variables de estado propuestas:

	Action last_action;
	bool tiene_zapatillas;
	int decision;
	vector<int> acciones_pendientes; // [TURN_L, TURN_SR, WALK, RUN]
	int num_acciones;
	int contador;
	vector<bool> baneados;
	vector<vector<int>> mapaFrecuencias;
};

#endif
