#ifndef COMPORTAMIENTOAUXILIAR_H
#define COMPORTAMIENTOAUXILIAR_H

#include <chrono>
#include <time.h>
#include <thread>
#include <list>

#include "comportamientos/comportamiento.hpp"

struct EstadoA
{
	int f;
	int c;
	int brujula;
	bool zapatillas;

	bool operator==(const EstadoA &st) const
	{
		return f == st.f && c == st.c && brujula == st.brujula && zapatillas == st.zapatillas;
	}
};

struct NodoA
{
	EstadoA estado;
	list<Action> secuencia;

	bool operator==(const NodoA &nodo) const
	{
		return estado == nodo.estado;
	}
};

class ComportamientoAuxiliar : public Comportamiento
{

public:
	ComportamientoAuxiliar(unsigned int size = 0) : Comportamiento(size)
	{
		// Inicializar Variables de Estado Niveles 0,1,4

		last_action = IDLE;
		tiene_zapatillas = false;
		giro45izq = giro180 = 0;
	}
	ComportamientoAuxiliar(std::vector<std::vector<unsigned char>> mapaR, std::vector<std::vector<unsigned char>> mapaC) : Comportamiento(mapaR, mapaC)
	{
		// Inicializar Variables de Estado Niveles 2,3
		hayPlan = false;
	}
	ComportamientoAuxiliar(const ComportamientoAuxiliar &comport) : Comportamiento(comport) {}
	~ComportamientoAuxiliar() {}

	Action think(Sensores sensores);

	int interact(Action accion, int valor);

	int VeoCasillaInteresanteA(char i, char c, char d);

	char ViablePorAlturaA(char casilla, int dif);

	void SituarSensorEnMapaA(vector<vector<unsigned char>> &m, vector<vector<unsigned char>> &a, Sensores sensores);

	//list<Action> AnchuraAuxiliar(const EstadoA &inicio, const EstadoA &final, 
	//	const vector<vector<unsigned char>> &terreno, const vector<vector<unsigned char>> &altura);

	Action ComportamientoAuxiliarNivel_0(Sensores sensores);
	Action ComportamientoAuxiliarNivel_1(Sensores sensores);
	Action ComportamientoAuxiliarNivel_2(Sensores sensores);
	Action ComportamientoAuxiliarNivel_3(Sensores sensores);
	Action ComportamientoAuxiliarNivel_4(Sensores sensores);

	//Action ComportamientoAuxiliarNivel_E(Sensores sensores);

private:
	// Definir Variables de Estado

	// Variables de estado propuestas:

	Action last_action;
	bool tiene_zapatillas;
	int giro45izq;
	int giro180;

	list<Action> plan;
	bool hayPlan;
};

#endif
