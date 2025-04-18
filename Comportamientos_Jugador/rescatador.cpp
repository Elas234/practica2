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

pair<int, int> ComportamientoRescatador::VtoM(int i, Orientacion rumbo, const pair<int, int> & orig)
{
	// Incremento/decremento de las coordenadas de la casilla justo en frente en función de la dirección
	// Norte, noreste, este, sureste, sur, suroeste, oeste,noroeste
	const int sf[8] = {-1, -1, 0,  1,  1,  1,  0, -1};
	const int sc[8] = { 0,  1, 1,  1,  0, -1, -1, -1};

	// Nivel de la casilla 		
	//									Nivel
	//	9	10	11	12	13	14	15		3
	//		4	5	6	7	8			2
	//			1	2	3				1
	//				^					0

	const int level[16] = {0,1,1,1,2,2,2,2,2,3,3,3,3,3,3,3};

	// Desplazamiento respecto de la casilla central del nivel 
	// Las casillas centrales son 2,6,12
	const int dist[16] = {0,-1,0,1,-2,-1,0,1,2,-3,-2,-1,0,1,2,3};

	// Signo del desplazamiento
	int sdistf[8] = {0,  1,  1,  1,  0, -1, -1, -1};
	int sdistc[8] = {1,  1,  0, -1, -1, -1,  0,  1};

	// Las diagonales tienen dos casos
	if(dist[i] < 0) {
		sdistf[1] = sdistf[5] = 0;
		sdistc[3] = sdistc[7] = 0;
	}
	else {
		sdistf[3] = sdistf[7] = 0;
		sdistc[1] = sdistc[5] = 0;
	}
	const int f = orig.first;
	const int c = orig.second;

	// Primero me desplazo hacia adelante (primer sumando) y luego lateralmente (segundo sumando)
	int nf = f + sf[rumbo] * level[i] + sdistf[rumbo] * dist[i];
	int nc = c + sc[rumbo] * level[i] + sdistc[rumbo] * dist[i];

	return {nf, nc};
}

void ComportamientoRescatador::CasillasInteresantes (const Sensores & sensores, const vector<bool> & accesible, const vector<bool> & transitable,
	vector<bool> & is_interesting, vector<int> & casillas_interesantes, bool zap) {

	is_interesting.resize(16, false);
	casillas_interesantes.clear();

	vector<unsigned char> vision = sensores.superficie;
	vector<unsigned char> altura = sensores.cota;
	vector<unsigned char> agentes = sensores.agentes;
	
		
	//? Primero compruebo si puedo ir a algún lado, y solo hago cálculos si puedo ir a más de uno
	
	const int NUM_RELEVANT = 3;
	const int ALCANZABLES = 6;
	
	const char RELEVANT[NUM_RELEVANT] = {'X', 'D', 'C'};
	const int POINTS[NUM_RELEVANT] = {10000, 1000, 100};
	const int DESTINOS[ALCANZABLES] = {1,2,3,4,6,8};
	
	for(char c : RELEVANT) {
		if (c == 'D' && zap) continue;
		for(int i : DESTINOS) {
			if(vision[i] == c && accesible[i]) {
				if(i >= 4 && !transitable[i/2-1]) continue;
				if((c=='X'  || c=='D') && agentes[i] != 'a') {
					casillas_interesantes.push_back(i);
					return;
				}
				is_interesting[i] = true;
				casillas_interesantes.push_back(i);
			}
		}
	}
	return;
}

int ComportamientoRescatador::SelectCasilla (const Sensores & sensores, const vector<int> & casillas_interesantes, 
	const vector<bool> & is_interesting, bool zap) {

	vector<unsigned char> vision = sensores.superficie;
	vector<unsigned char> altura = sensores.cota;
	vector<unsigned char> agentes = sensores.agentes;

	
	//? Primero compruebo si puedo ir a algún lado, y solo hago cálculos si puedo ir a más de uno

	const int NUM_RELEVANT = 3;
	const int ALCANZABLES = 6;

	const char RELEVANT[NUM_RELEVANT] = {'X', 'D', 'C'};
	const double POINTS[NUM_RELEVANT] = {10000.0, 1000.0, 100.0};
	const int DESTINOS[ALCANZABLES] = {1,2,3,4,6,8};

	// No hay casillas interesantes o solo hay una
	if(casillas_interesantes.size() == 0) {
		return 0;
	}
	if(casillas_interesantes.size() == 1) {
		return casillas_interesantes[0];
	}

	

	// Hay más de una, me voy por donde haya más y mejores casillas interesantes

	// Matriz de decisión : fila 0 -> izq, fila 1 -> centro, fila 2 -> derecha

	//	9	10	11	12	13	14	15
	//		4	5	6	7	8
	//			1	2	3
	//				^
	const int CASILLAS_POR_SECCION = 3;
	const int M[ALCANZABLES][CASILLAS_POR_SECCION] = 
	{	
		{4,5,6}, // izquierda (ir a 1)
		{5,6,7}, // centro (ir a 2)
		{6,7,8}, // derecha (ir a 3)
		{9,10,11}, // fondo izquierda (ir a 4)
		{11,12,13}, // fondo centro (ir a 6)
		{13,14,15} // fondo derecha (ir a 8)
	}; 

	double puntuacion_destino[ALCANZABLES] = {0.0};
	double max_points = 0.0;
	int decision = 0;
	pair<int,int> orig = {sensores.posF, sensores.posC};
	for(int i=0; i<ALCANZABLES; ++i) {
		if(!is_interesting[DESTINOS[i]]) continue;
		for(int j=0; j<CASILLAS_POR_SECCION; ++j) {
			for(int k=0; k<NUM_RELEVANT; ++k) {
				if(vision[M[i][j]] == RELEVANT[k]) {
					// No considero las casillas objetivo con agentes
					if(vision[M[i][j]] == 'X' && agentes[M[i][j]] == 'a') continue;

					// Puntuación inversamente proporcional a la diferencia de altura
					puntuacion_destino[i] += (POINTS[k]/(1+abs(altura[M[i][j]]-altura[DESTINOS[i]])));
					
					// Puntuación inversamente proporcional a la frecuencia de la casilla
					pair<int,int> casilla = VtoM(M[i][j], sensores.rumbo, orig);
					puntuacion_destino[i] /= (1.0+mapaFrecuencias[casilla.first][casilla.second]);

				}
			}
		}

		pair<int,int> casilla_destino = VtoM(DESTINOS[i], sensores.rumbo, orig);
		puntuacion_destino[i] /= (1.0+mapaFrecuencias[casilla_destino.first][casilla_destino.second]);
		// Actualizar máximo
		if(puntuacion_destino[i] >= max_points) {
			max_points = puntuacion_destino[i];
			decision = DESTINOS[i];
		}
	}

	cout << "Puntuaciones: " << endl;
	for(int i=0; i<ALCANZABLES; ++i) {
		cout << DESTINOS[i] << ": " << puntuacion_destino[i] << endl;
	}
	cout << "Ganador: " << decision << endl;

	return decision;
}

bool ComportamientoRescatador::ViablePorAltura(int dif, bool zap) {
	return (abs(dif) <= 1 || (zap && abs(dif) <= 2));
}

void ComportamientoRescatador::SituarSensorEnMapa(vector<vector<unsigned char>> &m, vector<vector<unsigned char>> &a, Sensores sensores) {
	
	int f = sensores.posF;
	int c = sensores.posC;
	pair<int,int> orig = {f, c};
	for(int i=0; i<16; ++i) {
		// Convierte la posición del sensor en la posición del mapa
		pair<int,int> casilla = VtoM(i, sensores.rumbo, orig);
		int nf = casilla.first;
		int nc = casilla.second;
		m[nf][nc] = sensores.superficie[i];
		a[nf][nc] = sensores.cota[i];
	}	
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

	SituarSensorEnMapa(mapaResultado, mapaCotas, sensores);
	mapaFrecuencias[sensores.posF][sensores.posC]++;

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
		// Resetea movimientos peligrosos
		corre = avanza = giro45izq = 0;
		action = TURN_L;
	}
	else if(avanza != 0) {
		action = WALK;
		avanza--;
	}
	else if(corre != 0) {
		action = RUN;
		corre--;
	}
	else 
	{
		// Casillas que puedo atravesar corriendo
		vector<bool> transitable(16);

		// Casillas a las que puedo acceder (accesible ==> transitable pero no al revés)
		vector<bool> accesible(16);

		// Casilla genérica
		for(int i=1; i<16; ++i) {
			transitable[i] = (sensores.superficie[i] != 'P' && sensores.superficie[i] != 'M' && sensores.superficie[i] != 'B');
			accesible[i] = transitable[i] && ViablePorAltura(sensores.cota[i]-sensores.cota[0], tiene_zapatillas);
		}

		vector<int> casillas_interesantes;
		vector<bool> is_interesting(16, false);
		CasillasInteresantes(sensores, accesible, transitable, is_interesting, casillas_interesantes, tiene_zapatillas);

		int decision = SelectCasilla(sensores, casillas_interesantes, is_interesting, tiene_zapatillas);
		// cout << "Go to " << decision << endl;
		switch(decision) {
			case 1: 
				// Casilla interesante a la izquierda
				giro45izq = 1;
				avanza = 1;
				action = TURN_L;
				break;
			case 2: 
				// Casilla interesante en frente
				action = WALK;
				break;
			case 3: 
				// Casilla interesante a la derecha
				avanza = 1;
				action = TURN_SR;
				break;
			case 4:
				corre = 1;
				giro45izq = 1;
				action = TURN_L;
				break;
			case 6:
				action = RUN;
				break;
			case 8:
				corre = 1;
				action = TURN_SR;
				break;
			case 0:
				// No hay casilla interesante (doy vueltas)
				action = TURN_SR;
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