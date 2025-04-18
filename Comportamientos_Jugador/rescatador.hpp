#ifndef COMPORTAMIENTORESCATADOR_H
#define COMPORTAMIENTORESCATADOR_H

#include <chrono>
#include <time.h>
#include <thread>

#include "comportamientos/comportamiento.hpp"

class ComportamientoRescatador : public Comportamiento
{

public:
	ComportamientoRescatador(unsigned int size = 0) : Comportamiento(size)
	{
		// Inicializar Variables de Estado Niveles 0,1,4 (constructor)

		last_action = IDLE;
		tiene_zapatillas = false;
		corre = avanza = giro45izq = giro180 = 0;
		mapaFrecuencias.resize(size, vector<int>(size, 0));
	}
	ComportamientoRescatador(std::vector<std::vector<unsigned char>> mapaR, std::vector<std::vector<unsigned char>> mapaC) : Comportamiento(mapaR, mapaC)
	{
		// Inicializar Variables de Estado Niveles 2,3
	}
	ComportamientoRescatador(const ComportamientoRescatador &comport) : Comportamiento(comport) {}
	~ComportamientoRescatador() {}

	Action think(Sensores sensores);

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
	 * @brief Obtiene las casillas interesantes
	 * @param sensores Sensores del agente
	 * @param accesible Casillas accesibles
	 * @param transitable Casillas transitables
	 * @param is_interesting Casillas interesantes. Parámetro de salida
	 * @param casillas_interesantes Casillas interesantes. Parámetro de salida
	 * @param zap Si el agente tiene zapatillas
	 */
	void CasillasInteresantes (const Sensores & sensores, const vector<bool> & accesible, const vector<bool> & transitable,
		vector<bool> & is_interesting, vector<int> & casillas_interesantes, bool zap);

	/**
	 * @brief Selecciona la casilla más interesante
	 * @param sensores Sensores del agente
	 * @param casillas_interesantes Casillas interesantes
	 * @param is_interesting Casillas interesantes
	 * @param zap Si el agente tiene zapatillas
	 * @return Casilla seleccionada
	 */
	int SelectCasilla (const Sensores & sensores, const vector<int> & casillas_interesantes, 
			const vector<bool> & is_interesting, bool zap);
	
	/**
	 * @brief Comprueba si la casilla es viable por altura
	 * @param dif Diferencia de altura
	 * @param zap Si el agente tiene zapatillas
	 * @return Si la casilla es viable por altura
	 */
	bool ViablePorAltura(int dif, bool zap);
	
	/**
	 * @brief Coloca el sensor en el mapa
	 * @param m Mapa de la superficie
	 * @param a Mapa de la altura
	 * @param sensores Sensores del agente
	 */
	void SituarSensorEnMapa(vector<vector<unsigned char>> &m, vector<vector<unsigned char>> &a, Sensores sensores);

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
	int giro45izq;
	int giro180;
	int avanza;
	int corre;
	vector<vector<int>> mapaFrecuencias;
};

#endif
