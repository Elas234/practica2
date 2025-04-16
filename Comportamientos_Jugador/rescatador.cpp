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

int ComportamientoRescatador::TomarDecision (const vector<unsigned char> & vision, const vector<unsigned char> & altura,
		const vector<bool> & accesible, const vector<bool> & transitable, bool zap) {

	
	//? Primero compruebo si puedo ir a algún lado, y solo hago cálculos si puedo ir a más de uno

	const int NUM_RELEVANT = 3;
	const int ALCANZABLES = 6;

	const char RELEVANT[NUM_RELEVANT] = {'X', 'D', 'C'};
	const char POINTS[NUM_RELEVANT] = {100, 10, 1};

	bool is_interesting[ALCANZABLES] = {false};
	vector<int> casillas_interesantes;
	for(char c : RELEVANT) {
		if (c == 'D' && zap) continue;
		for(int i=1, inc=1; i<=ALCANZABLES; i+=inc) {
			if(i==4) ++inc;
			if(vision[i] == c && accesible[i]) {
				if(i>=4 && !transitable[i/2-1]) continue;
				if(c=='X' || c=='D') return i;
				is_interesting[i] = true;
				casillas_interesantes.push_back(i);
				break;
			}
		}
	}

	// No hay casillas interesantes o solo hay una
	if(casillas_interesantes.size() == 0) return 0;
	if(casillas_interesantes.size() == 1) return casillas_interesantes[0];

	// Hay más de una, me voy por donde haya más y mejores casillas interesantes

	// Matriz de decisión : fila 0 -> izq, fila 1 -> centro, fila 2 -> derecha

	//	9	10	11	12	13	14	15
	//		4	5	6	7	8
	//			1	2	3
	//				^
	const int CASILLAS_POR_SECCION = 3;
	const int M[ALCANZABLES][CASILLAS_POR_SECCION] = 
	{	{9,10,11}, // fondo izquierda (ir a 4)
		{4,5,6}, // izquierda (ir a 1)
		{5,6,7}, // centro (ir a 2)
		{11,12,13}, // fondo centro (ir a 6)
		{12,13,14}, // derecha (ir a 3)
		{13,14,15} // fondo derecha (ir a 8)
	}; 

	int puntuacion_camino[ALCANZABLES] = {0};
	int max_points = 0;
	int decision = 0;
	for(int i=0; i<ALCANZABLES; ++i) {
		if(!is_interesting[i]) continue;
		for(int j=0; j<CASILLAS_POR_SECCION; ++j) {
			for(int k=0; k<NUM_RELEVANT; ++k) {
				if(vision[M[i][j]] == RELEVANT[k]) {
					// Puntuación inversamente proporcional a la diferencia de altura
					puntuacion_camino[i] += (POINTS[k]/(1+abs(altura[M[i][j]]-altura[0])));
				}
			}
		}
		// Actualizar máximo
		if(puntuacion_camino[i] >= max_points) {
			max_points = puntuacion_camino[i];
			decision = i;
		}
	}
	// Ajuste de índices
	// 0 -> 1, 1 -> 2, 2 -> 3
	// 3 -> 4, 4 -> 6, 5 -> 8
	if(decision<3) ++decision;
	else decision = 2*(decision-1);

	return decision;

	//? Maybe mejorar esto pa extenderlo a toda la visión
	// if(c=='X') return 2;
	// else if(i=='X') return 1;
	// else if(d=='X') return 3;
	// else if(!zap) {
	// 	if(c=='D') return 2;
	// 	else if(i=='D') return 1;
	// 	else if(d=='D') return 3;
	// }
	// if(c=='C') return 2;
	// else if(i=='C') return 1;
	// else if(d=='C') return 3;
	// else return 0;

	//? Es un comportamiento básico, bastante mejorable

	// const char RELEVANT[3] = {'X', 'D', 'C'};
	// const int MAX_LVL = 3;
	// for(char c : RELEVANT) {
	// 	for(int i=2, level=1; level<=MAX_LVL; i+=2*(++level)) {
	// 		cout << i << endl;
	// 		if(vision[i] == c) return i;
	// 		for(int j=1; j<=level; ++j) {
	// 			cout << i-j << endl;
	// 			cout << i+j << endl;
	// 			if(vision[i-j] == c) return i-j;
	// 			if(vision[i+j] == c) return i+j;
	// 		}
	// 	}
	// }
	// return 0;
}

bool ComportamientoRescatador::ViablePorAltura(char casilla, int dif, bool zap) {
	return (abs(dif) <= 1 || (zap && abs(dif) <= 2));
}

// char ComportamientoRescatador::ViablePorSprint(char origen, char inter, int dif, bool zap) {
// 	bool inter_transitable = true;
// 	if(inter == 'P' || inter == 'M' || (inter == 'B' && !zap) ) {
// 		return 'P';
// 	}
// 	else return ViablePorAltura(origen, dif, zap);
// }

void ComportamientoRescatador::SituarSensorEnMapa(vector<vector<unsigned char>> &m, vector<vector<unsigned char>> &a, Sensores sensores) {
	
	int y = sensores.posF;
	int x = sensores.posC;
	// cout << x << " " << y << endl;
	int cont = 0;
	int mx, my;
	Orientacion o = sensores.rumbo;
	int sgn[2] = {1, -1};
	// cout << o << endl;
	int s = sgn[o/4];

	if(o&1) {
		for(int i=0; i<4; ++i) {
			for(int j=i;j>0;--j) {
				switch(o) {
					case noreste:
					case suroeste:
						my = y-s*i;
						mx = x+s*(i-j);
						break;
					case sureste:
					case noroeste:
						my = y+s*(i-j);
						mx = x+s*i;
						break;
				}
				// int p = i, q = i-j;
				// if(o == sureste || o == noroeste) {
				// 	p = i-j;
				// 	q = i;
				// }
				// my = y-s*p;
				// mx = x+s*q;
				m[my][mx] = sensores.superficie[cont];
				a[my][mx] = sensores.cota[cont];
				++cont;
			}


			// Noreste (s=1)
			//? m[y-i][x+i-j] = sensores.superficie[cont];

			// Sureste (s=1)
			//? m[y+i-j][x+i] = sensores.superficie[cont];

			// Suroeste (s=-1)
			//? m[y+i][x-i+j] = sensores.superficie[cont];

			// Noroeste (s=-1)
			//? m[y-i+j][x-i] = sensores.superficie[cont];

			for(int j=0;j<=i;++j) {
				switch(o) {
					case noreste:
					case suroeste:
						my = y-s*(i-j);
						mx = x+s*i;
						break;
					case sureste:
					case noroeste:
						my = y+s*i;
						mx = x+s*(i-j);
						break;
				}
				// int p = i, q = i-j;
				// if(o == sureste || o == noroeste) {
				// 	p = i-j;
				// 	q = i;
				// }
				// my = y-s*p;
				// mx = x+s*q;
				m[my][mx] = sensores.superficie[cont];
				a[my][mx] = sensores.cota[cont];
				++cont;
			}

			// Noreste (s=1)
			//? m[y-i+j][x+i] = sensores.superficie[cont];

			// Sureste (s=1)
			//? m[y+i][x+i-j] = sensores.superficie[cont];
			
			// Suroeste (s=-1)
			//? m[y+i-j][x-i] = sensores.superficie[cont];

			// Noroeste (s=-1)
			//? m[y-i][x-i+j] = sensores.superficie[cont];
		}
	}
	else {

		for(int i=0; i<4; ++i) {
			for(int j=-i;j<=i;++j) {
				switch (o) {
					case norte:
					case sur:
						mx = x + s*j;
						my = y - s*i;
						break;
					case este:
					case oeste:
						mx = x + s*i;
						my = y + s*j;
						break;
				}
				// cout << mx-x << " " << my-y << endl;
				m[my][mx] = sensores.superficie[cont];
				a[my][mx] = sensores.cota[cont];
				++cont;
			}
		}
	}


	// Norte
	//? m[x+j][y-i] = sensores.superficie[cont];

	// Sur	
	//? m[x-j][y+i] = sensores.superficie[cont];

	// Este
	//? m[x+i][y+j] = sensores.superficie[cont];

	// Oeste
	//? m[x-i][y-j] = sensores.superficie[cont];
	
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
			transitable[i] = (sensores.superficie[i] != 'P' && sensores.superficie[i] != 'M' && (sensores.superficie[i] != 'B' || tiene_zapatillas) );
			accesible[i] = transitable[i] && ViablePorAltura(sensores.superficie[i], sensores.superficie[i]-sensores.superficie[0], tiene_zapatillas);
		}

		int pos = TomarDecision(sensores.superficie, sensores.cota, accesible, transitable, tiene_zapatillas);
		switch(pos) {
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