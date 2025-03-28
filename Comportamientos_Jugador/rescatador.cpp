#include "../Comportamientos_Jugador/rescatador.hpp"
#include "motorlib/util.h"

Action ComportamientoRescatador::think(Sensores sensores)
{
	Action accion = IDLE;

	switch (sensores.nivel)
	{
	case 0:
		accion = ComportamientoRescatadorNivel_0 (sensores);
		break;
	case 1:
		// accion = ComportamientoRescatadorNivel_1 (sensores);
		break;
	case 2:
		// accion = ComportamientoRescatadorNivel_2 (sensores);
		break;
	case 3:
		// accion = ComportamientoRescatadorNivel_3 (sensores);
		break;
	case 4:
		// accion = ComportamientoRescatadorNivel_4 (sensores);
		break;
	}

	return accion;
}

int ComportamientoRescatador::interact(Action accion, int valor)
{
	return 0;
}

int ComportamientoRescatador::VeoCasillaInteresanteR (char i, char c, char d, bool zap) {
	if(c=='X') return 2;
	else if(i=='X') return 1;
	else if(d=='X') return 3;
	else if(!zap) {
		if(c=='D') return 2;
		else if(i=='D') return 1;
		else if(d=='D') return 3;
	}
	if(c=='C') return 2;
	else if(i=='C') return 1;
	else if(d=='C') return 3;
	else return 0;
}

char ComportamientoRescatador::ViablePorAlturaR(char casilla, int dif, bool zap) {
	if(abs(dif) <= 1 || (zap && abs(dif) <= 2))
		return casilla;
	else
		return 'P';
}

Action ComportamientoRescatador::ComportamientoRescatadorNivel_0(Sensores sensores)
{
	// El comportamiento de seguir un camino hasta encontrar un puesto base.
	/*
	bool transitable[4];
	for(int i=1; i<4; ++i) {
		transitable[i] = (sensores.superficie[i] == 'C' || sensores.superficie[i] == 'X') && 
			abs(sensores.cota[i]-sensores.cota[i]) <= 1;
	} 

	if(transitable[3]) action = TURN_SR;
	else if(transitable[2]) action = WALK;
	else action = TURN_L;

	if(sensores.choque) {
		action = TURN_L;
	}
	*/

	/*
		Fase 1: Observar el entorno

		Obtener la información de los sensores, cambia en cada paso,
		por lo que es necesario utilizar variables de estado.

		Se proponen 3 variables de estado:

		Action last_action;
		bool tiene_zapatillas; 
		int giro45izq; 
	*/

	Action action;

	if(sensores.superficie[0] == 'D') {
		tiene_zapatillas = true;
	}

	/*
		Fase 2: Actuar

		Giro a la izquierda: TURN_L + TURN_SR

		Hago un giro a la izquierda, guardo en la variable de estado
		que estoy girando (la pongo a 1) y giro a la derecha
	*/

	if(sensores.superficie[0] == 'X') {
		action = IDLE;
	}
	else if(giro45izq != 0) {
		action = TURN_SR;
		giro45izq--;
	}
	else 
	{
		char i = ViablePorAlturaR(sensores.superficie[1], sensores.cota[1]-sensores.cota[0], tiene_zapatillas);
		char c = ViablePorAlturaR(sensores.superficie[2], sensores.cota[2]-sensores.cota[0], tiene_zapatillas);
		char d = ViablePorAlturaR(sensores.superficie[3], sensores.cota[3]-sensores.cota[0], tiene_zapatillas);

		int pos = VeoCasillaInteresanteR(i, c, d, tiene_zapatillas);
		switch(pos) {
			case 2: 
				action = WALK;
				break;
			case 1: 
				giro45izq = 1;
				action = TURN_L;
				break;
			case 3: 
				action = TURN_SR;
				break;
			case 0:
				action = TURN_L;
				break;
		}
	}


	/*-------------------------------------------------------------*/

	last_action = action;

	return action;
}

Action ComportamientoRescatador::ComportamientoRescatadorNivel_1(Sensores sensores)
{
}

Action ComportamientoRescatador::ComportamientoRescatadorNivel_2(Sensores sensores)
{
}

Action ComportamientoRescatador::ComportamientoRescatadorNivel_3(Sensores sensores)
{
}

Action ComportamientoRescatador::ComportamientoRescatadorNivel_4(Sensores sensores)
{
}