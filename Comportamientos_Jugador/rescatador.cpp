#include "../Comportamientos_Jugador/rescatador.hpp"
#include "motorlib/util.h"
#include <iostream>
using namespace std;

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

void ComportamientoRescatador::SituarSensorEnMapaR(vector<vector<unsigned char>> &m, vector<vector<unsigned char>> &a, Sensores sensores) {
	
	int x = sensores.posF;
	int y = sensores.posC;
	cout << x << " " << y << endl;
	int cont = 0;
	int mx, my;
	Orientacion o = sensores.rumbo;
	int sgn[2] = {1, -1};
	cout << o << endl;
	int s = sgn[o/4];

	m[x][y] = sensores.superficie[0];

	// if(o&1) {
	// 	for(int i=0; i<4; ++i) {
	// 		for(int j=i;j>0;++j) {
	// 			switch(o) {
	// 				case noreste:
	// 				case suroeste:
	// 					mx = x+s*(i-j);
	// 					my = y+s*i;
	// 					break;
	// 				case este:
	// 				case oeste:
	// 					mx = x+s*i;
	// 					my = y+s*(i-j);
	// 					break;
	// 			}
	// 			m[mx][my] = sensores.superficie[cont];
	// 			a[mx][my] = sensores.cota[cont];
	// 			++cont;
	// 		}


	// 		// Noreste
	// 		// m[x+i-j][y+i] = sensores.superficie[cont];

	// 		// Sureste
	// 		// m[x+i][y+i-j] = sensores.superficie[cont];

	// 		// Suroeste
	// 		// m[x-i+j][y-i] = sensores.superficie[cont];

	// 		// Noroeste
	// 		// m[x-i][y-i+j] = sensores.superficie[cont];

	// 		for(int j=0;j<=i;++j) {
	// 			switch(o) {
	// 				case noreste:
	// 				case suroeste:
	// 					mx = x+s*i;
	// 					my = y+s*(i-j);
	// 					break;
	// 				case este:
	// 				case oeste:
	// 					mx = x+s*(i-j);
	// 					my = y+s*i;
	// 					break;
	// 			}
	// 			m[mx][my] = sensores.superficie[cont];
	// 			a[mx][my] = sensores.cota[cont];
	// 			++cont;
	// 		}

	// 		// Noreste
	// 		// m[x+i][y+i-j] = sensores.superficie[cont];

	// 		// Sureste
	// 		// m[x+i-j][y+i] = sensores.superficie[cont];

	// 		// Suroeste
	// 		// m[x-i][y-i+j] = sensores.superficie[cont];

	// 		// Noroeste
	// 		// m[x-i+j][y-i] = sensores.superficie[cont];
	// 	}
	// }
	// else {

	// 	for(int i=0; i<4; ++i) {
	// 		for(int j=-i;j<=i;++j) {
	// 			switch (o) {
	// 				case norte:
	// 				case sur:
	// 					mx = x + s*j;
	// 					my = y + s*i;
	// 					break;
	// 				case este:
	// 				case oeste:
	// 					mx = x + s*i;
	// 					my = y - s*j;
	// 					break;
	// 				default:
	// 					cout << "error" << endl;
	// 					break;
	// 			}
	// 			// cout << mx-x << " " << my-y << endl;
	// 			m[mx][my] = sensores.superficie[cont];
	// 			a[mx][my] = sensores.cota[cont];
	// 			++cont;
	// 		}
	// 	}
	// }


	// // Norte
	// // m[x+j][y+i] = sensores.superficie[cont];

	// // Sur	
	// // m[x-j][y-i] = sensores.superficie[cont];

	// // Este
	// // m[x+i][y-j] = sensores.superficie[cont];

	// // Oeste
	// // m[x-i][y+j] = sensores.superficie[cont];
	
}

Action ComportamientoRescatador::ComportamientoRescatadorNivel_0(Sensores sensores)
{
	// El comportamiento de seguir un camino hasta encontrar un puesto base.

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

	// SituarSensorEnMapaR(mapaResultado, mapaCotas, sensores);

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
	else if(giro180 != 0) {
		action = TURN_L;
		giro180--;
	}
	else if (sensores.agentes[2] != '_') {
		giro180 = 1;
		action = TURN_L;
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