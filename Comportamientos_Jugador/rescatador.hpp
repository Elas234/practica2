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
		giro45izq = 0;
	}
	ComportamientoRescatador(std::vector<std::vector<unsigned char>> mapaR, std::vector<std::vector<unsigned char>> mapaC) : Comportamiento(mapaR, mapaC)
	{
		// Inicializar Variables de Estado Niveles 2,3
	}
	ComportamientoRescatador(const ComportamientoRescatador &comport) : Comportamiento(comport) {}
	~ComportamientoRescatador() {}

	Action think(Sensores sensores);

	int interact(Action accion, int valor);

	int VeoCasillaInteresanteR (char i, char c, char d, bool zap);

	char ViablePorAlturaR(char casilla, int dif, bool zap);

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
};

#endif
