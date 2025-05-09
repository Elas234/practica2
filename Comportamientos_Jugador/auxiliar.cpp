#include "../Comportamientos_Jugador/auxiliar.hpp"
#include <iostream>
#include <math.h>
#include <set>
#include <iomanip>
#include <queue>
#include "motorlib/util.h"

#define TURN_SR_pendientes 0
#define WALK_pendientes 1

const int INF = 1e6;

const int sf[8] = {-1, -1, 0,  1,  1,  1,  0, -1};
const int sc[8] = { 0,  1, 1,  1,  0, -1, -1, -1};

using min_priority_queue = priority_queue<NodoA, vector<NodoA>, greater<NodoA>>;

Action ComportamientoAuxiliar::think(Sensores sensores)
{
	Action accion = IDLE;

	switch (sensores.nivel)
	{
	case 0:
		accion = ComportamientoAuxiliarNivel_0(sensores);
		break;
	case 1:
		accion = ComportamientoAuxiliarNivel_1 (sensores);
		break;
	case 2:
		// accion = ComportamientoAuxiliarNivel_2 (sensores);
		break;
	case 3:
		accion = ComportamientoAuxiliarNivel_3 (sensores);
		// accion = ComportamientoAuxiliarNivel_E(sensores);
		break;
	case 4:
		// accion = ComportamientoAuxiliarNivel_4 (sensores);
		break;
	}

	return accion;
}

int ComportamientoAuxiliar::interact(Action accion, int valor)
{
	return 0;
}

/*********************************** NIVEL 0 **********************************************/

pair<int, int> ComportamientoAuxiliar::VtoM(int i, Orientacion rumbo, const pair<int, int> & orig)
{
	const int level[16] = {0,1,1,1,2,2,2,2,2,3,3,3,3,3,3,3};
	const int dist[16] = {0,-1,0,1,-2,-1,0,1,2,-3,-2,-1,0,1,2,3};
	int sdistf[8] = {0,  1,  1,  1,  0, -1, -1, -1};
	int sdistc[8] = {1,  1,  0, -1, -1, -1,  0,  1};
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

	int nf = f + sf[rumbo] * level[i] + sdistf[rumbo] * dist[i];
	int nc = c + sc[rumbo] * level[i] + sdistc[rumbo] * dist[i];

	return {nf, nc};
}

void ComportamientoAuxiliar::SituarSensorEnMapa(const Sensores & sensores) {
	
	int f = sensores.posF;
	int c = sensores.posC;
	pair<int,int> orig = {f, c};
	for(int i=0; i<16; ++i) {
		pair<int,int> casilla = VtoM(i, sensores.rumbo, orig);
		int nf = casilla.first;
		int nc = casilla.second;
		if(sensores.superficie[i] == 'X') conozco_bases = true;
		if(sensores.superficie[i] == 'D') conozco_zapatillas = true;
		mapaResultado[nf][nc] = sensores.superficie[i];
		mapaCotas[nf][nc] = sensores.cota[i];
		if(sensores.superficie[i] == 'X' && sensores.agentes[i] == 'r') {
			mapaEntidades[nf][nc] = 'r';
		}
	}
}

bool ComportamientoAuxiliar::TengoTareasPendientes(const Sensores & sensores, Action & action) {
	// action = IDLE;
	// if (acciones_pendientes[TURN_SR_pendientes] != 0)
	// {
	// 	action = TURN_SR;
	// 	acciones_pendientes[TURN_SR_pendientes]--;
	// }
	// // else if (sensores.agentes[2] != '_')
	// // {
	// // 	acciones_pendientes[TURN_SR_pendientes] = 3;
	// // 	// Resetea movimientos peligrosos
	// // 	acciones_pendientes[WALK_pendientes] = 0;
	// // 	action = TURN_SR;
	// // }
	// else if(acciones_pendientes[WALK_pendientes] != 0) {
	// 	action = WALK;
	// 	acciones_pendientes[WALK_pendientes]--;
	// }
	// if(action != IDLE) cout << "No estoy decidiendo" << endl;
	// return action != IDLE;

	const int NUM_ACCIONES = 2;
	const Action ACCIONES[NUM_ACCIONES] = {TURN_SR, WALK};

	for(int i=0; i<NUM_ACCIONES; ++i) {
		if(acciones_pendientes[i] != 0) {
			action = ACCIONES[i];
			acciones_pendientes[i]--;
			return true;
		}
	}
	return false;
}

bool ComportamientoAuxiliar::ViablePorAltura(int dif)
{
	return abs(dif) <= 1;
}

int ComportamientoAuxiliar::SelectCasilla(const Sensores & sensores, const vector<int> & casillas_interesantes, 
	const vector<bool> & is_interesting)
{
	const vector<unsigned char> vision = sensores.superficie;
	const vector<unsigned char> mapaCotas = sensores.cota;
	const vector<unsigned char> agentes = sensores.agentes;

	const int ALCANZABLES = 3;
	const int NUM_RELEVANT = 2;

	const char RELEVANT[NUM_RELEVANT] = {'X', 'C'};
	const double POINTS[NUM_RELEVANT] = {1000.0, 100.0};
	const int DESTINOS[ALCANZABLES] = {1,2,3};

	if(casillas_interesantes.size() == 0) return 0;
	if(casillas_interesantes.size() == 1) return casillas_interesantes[0];

	// Hay más de una, me voy por donde haya más y mejores casillas interesantes

	// Matriz de decisión : fila 0 -> izq, fila 1 -> centro, fila 2 -> derecha

	//	9	10	11	12	13	14	15
	//		4	5	6	7	8
	//			1	2	3
	//				^
	const int CASILLAS_POR_SECCION = 4;
	const int M[ALCANZABLES][CASILLAS_POR_SECCION] = 
	{
		{4,5,6}, // izquierda (ir a 1)
		{5,6,7}, // centro (ir a 2)
		{6,7,8} // derecha (ir a 3)
	}; 

	double puntuacion_destino[3] = {0.0};
	double max_points = 0.0;
	int decision = 0;
	pair<int,int> orig = {sensores.posF, sensores.posC};
	for(int i=0; i<ALCANZABLES; ++i) {
		if(!is_interesting[DESTINOS[i]]) continue;
		for(int j=0; j<CASILLAS_POR_SECCION; ++j) {
			for(int k=0; k<NUM_RELEVANT; ++k) {
				if(vision[M[i][j]] == RELEVANT[k]) {
					// No considero las casillas objetivo con agentes
					if(vision[M[i][j]] == 'X' && agentes[M[i][j]] == 'r') continue;

					// Puntuación inversamente proporcional a la diferencia de mapaCotas
					puntuacion_destino[i] += (POINTS[k]/(1+abs(mapaCotas[M[i][j]]-mapaCotas[DESTINOS[i]])));
					
					// Puntuación inversamente proporcional a la frecuencia de la casilla
					pair<int,int> casilla = VtoM(M[i][j], sensores.rumbo, orig);
					puntuacion_destino[i] /= (1.0+mapaFrecuencias[casilla.first][casilla.second]);
				}
			}
		}
		pair<int,int> casilla_destino = VtoM(DESTINOS[i], sensores.rumbo, orig);
		puntuacion_destino[i] /= (1+mapaFrecuencias[casilla_destino.first][casilla_destino.second]);
		// Actualizar máximo
		if(puntuacion_destino[i] >= max_points) {
			max_points = puntuacion_destino[i];
			decision = DESTINOS[i];
		}
	}

	// cout << "Puntuaciones: " << endl;
	// for(int i=0; i<ALCANZABLES; ++i) {
	// 	cout << DESTINOS[i] << ": " << puntuacion_destino[i] << endl;
	// }
	// cout << "Ganador: " << decision << endl;

	return decision;
}

int ComportamientoAuxiliar::SelectCasillaAllAround_LVL1(const pair<int,int> & orig, const vector<int> & casillas_interesantes,  const vector<bool> & is_interesting, Orientacion rumbo) {
	const int ALCANZABLES = 8;
	const int NUM_RELEVANT = 6;

	const char RELEVANT[NUM_RELEVANT] = {'X', '?', 'D', 'C', 'S'};
	const int POINTS[NUM_RELEVANT] = {10, 10, 10, 10, 10};

	if(casillas_interesantes.size() == 0) return -1;
	if(casillas_interesantes.size() == 1) return casillas_interesantes[0];

	// Hay más de una, me voy por donde haya más y mejores casillas interesantes

	// Matriz de decisión

	// Cada casilla accesible tiene un valor asociado que depende de las 3 casillas que tiene enfrente

	const int CASILLAS_POR_SECCION = 3;
	const pair<int,int> M[ALCANZABLES][CASILLAS_POR_SECCION] = 
	{
		{{-1,-1},{-1, 0},{-1, 1}}, // ir a 0
		{{-1, 0},{-1, 1},{ 0, 1}}, // ir a 1
		{{-1, 1},{ 0, 1},{ 1, 1}}, // ir a 2
		{{ 0, 1},{ 1, 1},{ 1, 0}}, // ir a 3
		{{ 1, 1},{ 1, 0},{ 1,-1}}, // ir a 4
		{{ 1, 0},{ 1,-1},{ 0,-1}}, // ir a 5
		{{ 1,-1},{ 0,-1},{-1,-1}}, // ir a 6
		{{ 0,-1},{-1,-1},{-1, 0}}  // ir a 7
	};

	int peso_mapaCotas = 1;
	int peso_frecuencia_adyacentes = 0;
	int peso_frecuencia = 10;
	int puntuacion_destino[ALCANZABLES] = {0};
	int puntuaciones_iniciales[ALCANZABLES] = // {0};
	{100, 50, 10, 5, 1, 5 , 10, 50};
	// {100, 50, 10, 5, 1, 5 , 10, 50};
	// {50, 100, 50, 30, 10, 5 , 10, 30};

	for(int i=0; i<ALCANZABLES; ++i) {
		if(!is_interesting[(rumbo+i)%8]) continue;
		puntuacion_destino[(rumbo+i)%8] = puntuaciones_iniciales[i];
	}
	int max_points = -INF;
	int decision = 0;
	for(int i=0; i<ALCANZABLES; ++i) {
		if(!is_interesting[i]) continue;
		int f = orig.first + sf[i];
		int c = orig.second + sc[i];
		for(int j=0; j<CASILLAS_POR_SECCION; ++j) {
			for(int k=0; k<NUM_RELEVANT; ++k) {
				int nf = f + M[i][j].first;
				int nc = c + M[i][j].second;
				char x = mapaResultado[nf][nc];
				int h = mapaCotas[nf][nc];
				char a = mapaEntidades[nf][nc];
				if(x == RELEVANT[k]) {
					// No considero las casillas objetivo con agentes
					if(a == 'r') continue;
					
					// Puntuación de casillas adyacentes
					puntuacion_destino[i] += (POINTS[k] - peso_frecuencia_adyacentes*mapaFrecuencias[nf][nc] - peso_mapaCotas*abs(h-mapaCotas[f][c]));
				}
			}
		}
		puntuacion_destino[i] -= (peso_frecuencia * mapaFrecuencias[f][c]);
		// Actualizar máximo
		if(puntuacion_destino[i] >= max_points) {
			max_points = puntuacion_destino[i];
			decision = i;
		}
	}

	// cout << "Puntuaciones: " << endl;
	// for(int i=0; i<ALCANZABLES; ++i) {
	// 	cout << i << ": " << puntuacion_destino[i] << endl;
	// }
	// cout << "Ganador: " << decision << endl;

	return decision;
}

/*
	// int ComportamientoAuxiliar::SelectCasillaAllAround_LVL1(const pair<int,int> & orig, const vector<int> & casillas_interesantes, 
	// 	const vector<bool> & is_interesting, Orientacion rumbo) 
	// {
		// const int ALCANZABLES = 8;
		// const int NUM_RELEVANT = 5;
		// const int LENGTH = 1;
		// const int WIDTH = 3;
		// int sup = WIDTH/2;
		// int inf = -sup;
		// const int K = sqrt(LENGTH*WIDTH);
		
		// const char RELEVANT[NUM_RELEVANT] = {'X', '?', 'D', 'C', 'S'};
		// const int POINTS[NUM_RELEVANT] = {10, 20, 10, 10, 10};

		// if(casillas_interesantes.size() == 0) return -1;
		// if(casillas_interesantes.size() == 1) return casillas_interesantes[0];

		// // Hay más de una, me voy por donde haya más y mejores casillas interesantes

		// int peso_frecuencia_adyacentes = 2;
		// int peso_frecuencia = 10;
		// int puntuacion_destino[ALCANZABLES] = {0};
		// int puntuaciones_iniciales[ALCANZABLES] = // {0};
		// {100, 50, 10, 5, 1, 5 , 10, 50};
		// // {100, 50, 10, 5, 1, 5 , 10, 50};
		// // {50, 100, 50, 30, 10, 5 , 10, 30};

		// for(int i=0; i<ALCANZABLES; ++i) {
		// 	if(!is_interesting[(rumbo+i)%8]) continue;
		// 	puntuacion_destino[(rumbo+i)%8] = K*puntuaciones_iniciales[i];
		// }

		// const int df[ALCANZABLES] = { 0, 1, 1, 0, 0,-1,-1, 0};
		// const int dc[ALCANZABLES] = { 1, 0, 0,-1,-1, 0, 0, 1};

		// const int MAX_F = mapaResultado.size();
		// const int MAX_C = mapaResultado[0].size();

		// int max_points = -INF;
		// int decision = -1;
		// vector<vector<pair<int,int>>> casillas(ALCANZABLES);
		// for(int pos=0; pos<ALCANZABLES; ++pos) {
		// 	if(!is_interesting[pos]) continue;
		// 	int f = orig.first + sf[pos];
		// 	int c = orig.second + sc[pos];

		// 	// View ray
		// 	for(int i=1; i<=LENGTH; ++i) {
		// 		for(int j=inf; j<=sup; ++j) {
		// 			int npos = (j<=0 && (pos&1)) ? pos-1 : pos;
		// 			int nf = f + i*sf[pos] + j*df[npos];
		// 			int nc = c + i*sc[pos] + j*dc[npos];
		// 			if(nf < 0 || nc < 0 || nf >= MAX_F || nc >= MAX_C) continue;
		// 			casillas[pos].push_back({nf, nc});
		// 			// mapaEntidades[nf][nc] = '.';
		// 		}
		// 	}

		// 	// // Imprimir mapa entidades
		// 	// for(int i=0; i<MAX_F; ++i) {
		// 	// 	for(int j=0; j<MAX_C; ++j) {
		// 	// 		cout << mapaEntidades[i][j] << " ";
		// 	// 	}
		// 	// 	cout << endl;
		// 	// }
		// 	// cout << endl;

		// 	for(int i=0; i<casillas[pos].size(); ++i) {
		// 		int nf = casillas[pos][i].first;
		// 		int nc = casillas[pos][i].second;
		// 		for(int j=0; j<NUM_RELEVANT; ++j) {
		// 			char x = mapaResultado[nf][nc];
		// 			if(x == RELEVANT[j]) {

		// 				// Puntuación de casillas adyacentes
		// 				puntuacion_destino[pos] += (POINTS[j] - peso_frecuencia_adyacentes*mapaFrecuencias[nf][nc]);
		// 			}
		// 		}
		// 	}
			
		// 	puntuacion_destino[pos] -= (peso_frecuencia * mapaFrecuencias[f][c]);
		// 	// Actualizar máximo
		// 	if(puntuacion_destino[pos] >= max_points) {
		// 		max_points = puntuacion_destino[pos];
		// 		decision = pos;
		// 	}
		// }

		// // cout << "Puntuaciones: " << endl;
		// // for(int i=0; i<ALCANZABLES; ++i) {
		// // 	cout << i << ": " << puntuacion_destino[i] << endl;
		// // }
		// // cout << "Ganador: " << decision << endl;

		// return decision;
	// }
*/

int ComportamientoAuxiliar::SelectCasillaAllAround(const pair<int,int> & orig, const vector<int> & casillas_interesantes, const vector<bool> & is_interesting, Orientacion rumbo) {

	const int ALCANZABLES = 8;
	const int NUM_RELEVANT = 4;
	
	const char RELEVANT[NUM_RELEVANT] = {'X', '?', 'D', 'C'};
	const int POINTS[NUM_RELEVANT] = {100, 10, 10, 10};

	if(casillas_interesantes.size() == 0) return -1;
	if(casillas_interesantes.size() == 1) return casillas_interesantes[0];

	// Hay más de una, me voy por donde haya más y mejores casillas interesantes

	// Matriz de decisión

	// Cada casilla accesible tiene un valor asociado que depende de las 3 casillas que tiene enfrente

	const int CASILLAS_POR_SECCION = 3;
	const pair<int,int> M[ALCANZABLES][CASILLAS_POR_SECCION] = 
	{
		{{-1,-1},{-1, 0},{-1, 1}}, // ir a 0
		{{-1, 0},{-1, 1},{ 0, 1}}, // ir a 1
		{{-1, 1},{ 0, 1},{ 1, 1}}, // ir a 2
		{{ 0, 1},{ 1, 1},{ 1, 0}}, // ir a 3
		{{ 1, 1},{ 1, 0},{ 1,-1}}, // ir a 4
		{{ 1, 0},{ 1,-1},{ 0,-1}}, // ir a 5
		{{ 1,-1},{ 0,-1},{-1,-1}}, // ir a 6
		{{ 0,-1},{-1,-1},{-1, 0}}  // ir a 7
	};

	int peso_mapaCotas = 1;
	int peso_frecuencia_adyacentes = 5;
	int peso_frecuencia = 10;
	int puntuacion_destino[ALCANZABLES] = {0};
	int puntuaciones_iniciales[ALCANZABLES] = // {0};
	{100, 50, 10, 5, 1, 5 , 10, 50};
	// {100, 50, 10, 5, 1, 5 , 10, 50};
	// {50, 100, 50, 30, 10, 5 , 10, 30};

	for(int i=0; i<ALCANZABLES; ++i) {
		if(!is_interesting[(rumbo+i)%8]) continue;
		puntuacion_destino[(rumbo+i)%8] = puntuaciones_iniciales[i];
	}
	int max_points = -INF;
	int decision = 0;
	for(int i=0; i<ALCANZABLES; ++i) {
		if(!is_interesting[i]) continue;
		int f = orig.first + sf[i];
		int c = orig.second + sc[i];
		for(int j=0; j<CASILLAS_POR_SECCION; ++j) {
			for(int k=0; k<NUM_RELEVANT; ++k) {
				int nf = f + M[i][j].first;
				int nc = c + M[i][j].second;
				char x = mapaResultado[nf][nc];
				int h = mapaCotas[nf][nc];
				char a = mapaEntidades[nf][nc];
				if(x == RELEVANT[k]) {
					// No considero las casillas objetivo con agentes
					if(a == 'r') continue;
					
					// Puntuación de casillas adyacentes
					puntuacion_destino[i] += (POINTS[k] - peso_frecuencia_adyacentes*mapaFrecuencias[nf][nc] - peso_mapaCotas*abs(h-mapaCotas[f][c]));
				}
			}
		}
		puntuacion_destino[i] -= (peso_frecuencia * mapaFrecuencias[f][c]);
		// Actualizar máximo
		if(puntuacion_destino[i] >= max_points) {
			max_points = puntuacion_destino[i];
			decision = i;
		}
	}

	// cout << "Puntuaciones: " << endl;
	// for(int i=0; i<ALCANZABLES; ++i) {
	// 	cout << i << ": " << puntuacion_destino[i] << endl;
	// }
	// cout << "Ganador: " << decision << endl;

	return decision;

}

void ComportamientoAuxiliar::CasillasInteresantes (const Sensores & sensores, const vector<bool> & accesible, vector<bool> & is_interesting, vector<int> & casillas_interesantes) {

	is_interesting.resize(16, false);
	casillas_interesantes.clear();

	vector<unsigned char> vision = sensores.superficie;
	vector<unsigned char> mapaCotas = sensores.cota;
	vector<unsigned char> agentes = sensores.agentes;

	const int ALCANZABLES = 3;
	const int NUM_RELEVANT = 2;

	const char RELEVANT[NUM_RELEVANT] = {'X', 'C'};
	const int DESTINOS[ALCANZABLES] = {1,2,3};

	for(char c : RELEVANT) {
		for(int i : DESTINOS) {
			if(vision[i] == c && accesible[i]) {
				if(c=='X' && agentes[i] != 'r') {
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

void ComportamientoAuxiliar::CasillasInteresantesAllAround(const pair<int,int> & orig, const vector<bool> & accesible, vector<bool> & is_interesting, vector<int> & casillas_interesantes) {

	int f = orig.first;
	int c = orig.second;

	const int ALCANZABLES = 8;
	const int NUM_RELEVANT = 3;
	const char RELEVANT[NUM_RELEVANT] = {'X', 'D', 'C'};

	is_interesting.resize(ALCANZABLES, false);
	casillas_interesantes.clear();

	for(char x : RELEVANT) {
		for(int i=0; i<ALCANZABLES; ++i) {
			if(baneados[i]) continue;
			int nf = f + sf[i];
			int nc = c + sc[i];

			if(mapaResultado[nf][nc] == x && accesible[i]) {
				if(x=='X') {
					if(mapaEntidades[nf][nc] == 'r') continue;
					casillas_interesantes.clear();
					casillas_interesantes.push_back(i);
					return;
				}
				is_interesting[i] = true;
				casillas_interesantes.push_back(i);
			}
		}
	}
}

void ComportamientoAuxiliar::CasillasInteresantesAllAround_LVL1 (const pair<int,int> & orig, const vector<bool> & accesible, vector<bool> & is_interesting, vector<int> & casillas_interesantes) {

	int f = orig.first;
	int c = orig.second;

	const int ALCANZABLES = 8;
	const int NUM_RELEVANT = 4;
	const char RELEVANT[NUM_RELEVANT] = {'X', 'D', 'C', 'S'};

	is_interesting.resize(ALCANZABLES, false);
	casillas_interesantes.clear();

	for(char x : RELEVANT) {
		for(int i=0; i<ALCANZABLES; ++i) {
			if(baneados[i]) continue;
			int nf = f + sf[i];
			int nc = c + sc[i];

			if(mapaResultado[nf][nc] == x && accesible[i]) {
				is_interesting[i] = true;
				casillas_interesantes.push_back(i);
			}
		}
	}
}

Action ComportamientoAuxiliar::SelectAction(int decision, Orientacion rumbo)
{
	Action action;
	if(decision == -1) {
		action = TURN_SR;
	}
	else{
		decision = (decision + 8 - rumbo) % 8;

		if(decision==0) action = WALK;
		else if(decision>0) {
			acciones_pendientes[TURN_SR_pendientes] = decision-1;
			acciones_pendientes[WALK_pendientes] = 1;
			action = TURN_SR;
		}
		else {
			action = TURN_SR;
		}
	}
	return action;
}

void GenerateLevel(int level, vector<int> & filas, vector<int> & columnas) {
	filas.clear();
	columnas.clear();
	int n = -level;
	for(int i=0; i<=level; ++i) {
		filas.push_back(n);
	}
	while(n < level) {
		++n;
		filas.push_back(n);
	}
	for(int i=0; i<2*level+1; ++i) {
		filas.push_back(level);
	}

}

void ComportamientoAuxiliar::OndaDeCalor(int f, int c) {

	int calor = 1;
	const int MAX_LEVEL = 5;
	const int LEVELS = 1;
	vector<int> m[MAX_LEVEL+1][2];

	m[0][0] = m[0][1] = {0};

	m[1][0] = {-1, -1,  0,  1,  1,  1,  0, -1};
	m[1][1] = { 0,  1,  1,  1,  0, -1, -1, -1};

	m[2][0] = {-2, -2, -2, -1, 0, 1, 2, 2, 2, 2, 2, 1, 0, -1, -2, -2};
	m[2][1] = { 0,  1, 2, 2, 2, 2, 2, 1, 0, -1, -2, -2, -2, -2, -2, -1};

	m[3][0] = {-3,-3,-3,-3,-2,-1,0,1,2,3,3,3,3,3,3,3,2,1,0,-1,-2,-3,-3,-3};
	m[3][1] = {0,1,2,3,3,3,3,3,3,3,2,1,0,-1,-2,-3,-3,-3,-3,-3,-3,-3,-2,-1};

	m[4][0] = {-4,-4,-4,-4,-4,-3,-2,-1,0,1,2,3,4,4,4,4,4,4,4,4,4,3,2,1,0,-1,-2,-3,-4,-4,-4,-4};
	m[4][1] = {0,1,2,3,4,4,4,4,4,4,4,4,4,3,2,1,0,-1,-2,-3,-4,-4,-4,-4,-4,-4,-4,-4,-4,-3,-2,-1};

	m[5][0] = {-5,-5,-5,-5,-5,-5,-4,-3,-2,-1,0,1,2,3,4,5,5,5,5,5,5,5,5,5,5,5,4,3,2,1,0,-1,-2,-3,-4,-5,-5,-5,-5,-5};
	m[5][1] = {0,1,2,3,4,5,5,5,5,5,5,5,5,5,5,5,4,3,2,1,0,-1,-2,-3,-4,-5,-5,-5,-5,-5,-5,-5,-5,-5,-5,-5,-4,-3,-2,-1};

	// mapaFrecuencias[f][c] += calor;
	for(int level=0; level<LEVELS; ++level) {
		for(int j=0; j<m[level][0].size(); ++j) {
			int nf = f + m[level][0][j];
			int nc = c + m[level][1][j];
			if(nf < 0 || nc < 0 || nf >= mapaFrecuencias.size() || nc >= mapaFrecuencias[0].size()) continue;
			mapaFrecuencias[nf][nc] += calor;
		}
		calor /= 2;
		if(calor <= 0) break;
	}
	
}

/*********************************** NIVEL 0 **********************************************/
Action ComportamientoAuxiliar::ComportamientoAuxiliarNivel_0(Sensores sensores)
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

	// Actualización de variables de estado

	// Si está atascado, resetea el mapa de frecuencias
	// const int NUM_ACCIONES_ATASCO = 1000;
	// if(num_acciones == NUM_ACCIONES_ATASCO) {
	// 	for(int i=0; i<mapaFrecuencias.size(); ++i) {
	// 		fill(mapaFrecuencias[i].begin(), mapaFrecuencias[i].end(), 0);
	// 	}
	// }

	SituarSensorEnMapa(sensores);
	if(last_action == WALK)
		// mapaFrecuencias[sensores.posF][sensores.posC]++;
		OndaDeCalor(sensores.posF, sensores.posC);

	if(sensores.superficie[0] == 'D') {
		tiene_zapatillas = true;
	}

	/*
		Fase 2: Actuar

		Giro a la izquierda: TURN_L + TURN_SR

		Hago un giro a la izquierda, guardo en la variable de estado
		que estoy girando (la pongo a 1) y giro a la derecha
	*/

	int f = sensores.posF;
	int c = sensores.posC;

	if (sensores.superficie[0] == 'X') {
		action = IDLE;
	}
	else if(!TengoTareasPendientes(sensores, action)) {
		// Casillas que puedo atravesar corriendo
		vector<bool> transitable(8);

		// Casillas a las que puedo acceder (accesible ==> transitable pero no al revés)
		vector<bool> accesible(8);

		for(int i=0; i<8; ++i) {
			int nf = f + sf[i];
			int nc = c + sc[i];
			char s = mapaResultado[nf][nc];
			int h = mapaCotas[nf][nc];
			char e = mapaEntidades[nf][nc];

			transitable[i] = (s != 'P' && s != 'M' && (s != 'B' || tiene_zapatillas) && s != '?' && e != 'a' && !baneados[i]);
			accesible[i] = transitable[i] && ViablePorAltura(h-mapaCotas[f][c]);
		}

		vector<int> casillas_interesantes;
		vector<bool> is_interesting(8, false);

		CasillasInteresantesAllAround({f,c}, accesible, is_interesting, casillas_interesantes);

		decision = SelectCasillaAllAround({f,c}, casillas_interesantes, is_interesting, sensores.rumbo);
		action = SelectAction(decision, sensores.rumbo);

	}

	if(num_acciones - contador == 10) {
		fill(baneados.begin(), baneados.end(), false);
	}

	if((action == WALK && sensores.agentes[2] != '_')) {
		// Resetea movimientos peligrosos
		fill(acciones_pendientes.begin(), acciones_pendientes.end(), 0);
		acciones_pendientes[TURN_SR_pendientes] = 3;
		action = TURN_SR;
		baneados[decision] = true;
		contador = num_acciones;
	}

	// Resetea mapa de entidades
	for(int i=0; i<16; ++i) {
		pair<int,int> casilla = VtoM(i, sensores.rumbo, {f,c});
		int nf = casilla.first;
		int nc = casilla.second;
		if(mapaResultado[nf][nc] != 'X' || mapaEntidades[nf][nc] != 'r') {
			mapaEntidades[nf][nc] = '?';
		}
	}

	last_action = action;
	++num_acciones;

	return action;
}

/*********************************** NIVEL 1 **********************************************/
Action ComportamientoAuxiliar::ComportamientoAuxiliarNivel_1(Sensores sensores)
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

	// Actualización de variables de estado

	// Si está atascado, resetea el mapa de frecuencias
	// const int NUM_ACCIONES_ATASCO = 1000;
	// if(num_acciones == NUM_ACCIONES_ATASCO) {
	// 	for(int i=0; i<mapaFrecuencias.size(); ++i) {
	// 		fill(mapaFrecuencias[i].begin(), mapaFrecuencias[i].end(), 0);
	// 	}
	// }
	if(sensores.energia <= 10) {
		// Low battery
		return IDLE;
	}

	SituarSensorEnMapa(sensores);
	if(last_action == WALK)
		OndaDeCalor(sensores.posF, sensores.posC);
		// mapaFrecuencias[sensores.posF][sensores.posC]++;

	if(sensores.superficie[0] == 'D') {
		tiene_zapatillas = true;
	}

	/*
		Fase 2: Actuar

		Giro a la izquierda: TURN_L + TURN_SR

		Hago un giro a la izquierda, guardo en la variable de estado
		que estoy girando (la pongo a 1) y giro a la derecha
	*/

	int f = sensores.posF;
	int c = sensores.posC;

	if(!TengoTareasPendientes(sensores, action)) {
		// Casillas que puedo atravesar corriendo
		vector<bool> transitable(8);

		// Casillas a las que puedo acceder (accesible ==> transitable pero no al revés)
		vector<bool> accesible(8);

		for(int i=0; i<8; ++i) {
			int nf = f + sf[i];
			int nc = c + sc[i];
			char s = mapaResultado[nf][nc];
			int h = mapaCotas[nf][nc];
			char e = mapaEntidades[nf][nc];

			transitable[i] = (s != 'P' && s != 'M' && (s != 'B' || tiene_zapatillas) && s != '?' && e != 'a' && !baneados[i]);
			accesible[i] = transitable[i] && ViablePorAltura(h-mapaCotas[f][c]);
		}

		vector<int> casillas_interesantes;
		vector<bool> is_interesting(8, false);

		CasillasInteresantesAllAround_LVL1({f,c}, accesible, is_interesting, casillas_interesantes);

		decision = SelectCasillaAllAround_LVL1({f,c}, casillas_interesantes, is_interesting, sensores.rumbo);
		action = SelectAction(decision, sensores.rumbo);

	}

	if(num_acciones - contador == 10) {
		fill(baneados.begin(), baneados.end(), false);
	}

	if((action == WALK && sensores.agentes[2] != '_')) {
		// Resetea movimientos peligrosos
		fill(acciones_pendientes.begin(), acciones_pendientes.end(), 0);
		acciones_pendientes[TURN_SR_pendientes] = 3;
		action = TURN_SR;
		baneados[decision] = true;
		contador = num_acciones;
	}

	// Resetea mapa de entidades
	// for(int i=0; i<16; ++i) {
	// 	pair<int,int> casilla = VtoM(i, sensores.rumbo, {f,c});
	// 	int nf = casilla.first;
	// 	int nc = casilla.second;
	// 	mapaEntidades[nf][nc] = '?';
	// }

	for(int i=0; i<mapaEntidades.size(); ++i) {
		for(int j=0; j<mapaEntidades[i].size(); ++j) {
			mapaEntidades[i][j] = '?';
		}
	}

	last_action = action;
	++num_acciones;

	return action;
}

/*********************************** NIVEL 2 **********************************************/
Action ComportamientoAuxiliar::ComportamientoAuxiliarNivel_2(Sensores sensores)
{
}

/*********************************** NIVEL 3 **********************************************/
Action ComportamientoAuxiliar::ComportamientoAuxiliarNivel_3(Sensores sensores)
{
	Action accion = IDLE;
	if (!hayPlan)
	{
		index = 0;
		plan.clear();
		EstadoA inicio, fin;
		inicio.f = sensores.posF;
		inicio.c = sensores.posC;
		inicio.brujula = sensores.rumbo;
		fin.f = sensores.destinoF;
		fin.c = sensores.destinoC;

		inicio.zapatillas = false;
		plan = A_Estrella(inicio, fin, sensores);
		VisualizaPlan(inicio, plan);
		PintaPlan(plan, tiene_zapatillas);
		hayPlan = (plan.size() != 0);
	}
	if (hayPlan && index < plan.size())
	{
		accion = plan[index];
		++index;
	}
	if (index == plan.size())
	{
		hayPlan = false;
	}
	return accion;
}

bool ComportamientoAuxiliar::HayQueReplanificar(const Sensores & sensores, const Action & accion, const EstadoA & estado) {
	if(sensores.choque) return true;
	if(accion == WALK && !CasillaAccesibleAuxiliar(estado, 2)) return true;

	return false;
}

vector<Action> ComportamientoAuxiliar::DijkstraPuestosBase(const EstadoA &inicio, const Sensores & sensores) {
	NodoA current_node;
	min_priority_queue frontier;
	set<EstadoA> explored;
	vector<Action> path;

	int cont = 0;

	current_node.estado = inicio; // Asigna el estado inicial al nodo actual
	current_node.g = 0;
	// current_node.h = Heuristica(current_node.estado, final);

	frontier.push(current_node);

	bool SolutionFound = mapaResultado[current_node.estado.f][current_node.estado.c] == 'X';

	while (!SolutionFound && !frontier.empty())
	{
		frontier.pop();

		if (mapaResultado[current_node.estado.f][current_node.estado.c] == 'D')
		{
			current_node.estado.zapatillas = true;
		}
		explored.insert(current_node.estado);


		SolutionFound = mapaResultado[current_node.estado.f][current_node.estado.c] == 'X';
		if(SolutionFound) break;

		Action acciones[2] = {WALK, TURN_SR};
		for(Action accion : acciones) {
			NodoA child;
			child.estado = current_node.estado;
			child.secuencia = current_node.secuencia;

			bool accesible = true;
			child.estado = applyA(accion, current_node.estado, accesible, sensores);
			if(!accesible) continue;
			child.g = current_node.g + CalcularCoste(accion, current_node.estado);
			// child.h = Heuristica(child.estado, final);
			
			if (explored.find(child.estado) == explored.end())
			{
				child.secuencia.push_back(accion);
				frontier.push(child);
			}
		}

		if (!SolutionFound && !frontier.empty())
		{
			current_node = frontier.top();
			while(explored.find(current_node.estado) != explored.end() && !frontier.empty())
			{
				frontier.pop();
				current_node = frontier.top();
			}
		}
		SolutionFound = mapaResultado[current_node.estado.f][current_node.estado.c] == 'X';
	}

	if (SolutionFound)
		path = current_node.secuencia;

	return path;
}

/*********************************** NIVEL 4 **********************************************/
Action ComportamientoAuxiliar::ComportamientoAuxiliarNivel_4(Sensores sensores)
{
	Action accion = IDLE;

	const int UMBRAL_TIEMPO = 4*mapaResultado.size();
	const int UMBRAL_ENERGIA = 20*mapaResultado.size();
	const int MAX_ENERGIA = 3000;

	SituarSensorEnMapa(sensores);
	if(last_action == WALK)
		OndaDeCalor(sensores.posF, sensores.posC);

	if(sensores.superficie[0] == 'D') {
		tiene_zapatillas = true;
	}

	if(sensores.superficie[0] != 'X') {
		recargar = sensores.energia <= UMBRAL_ENERGIA;
	}

	/**********************************************************************************************
	 * ? RECARGAR ENERGÍA 
	 **********************************************************************************************/

	if(recargar && conozco_bases) {
		// Dijkstra a puestos base
		// cout << "Dijkstra a puestos base" << endl;
		hayPlan = false;
		plan.clear();
		index = 0;

		// cout << sensores.energia << endl;
		if(sensores.superficie[0] == 'X') {
			// cout << "Recargando..." << endl;
			if(sensores.energia == MAX_ENERGIA) {
				// cout << "Voy a dejar de recargar..." << endl;
				recargar = false;
			}
			
			return IDLE;
		}
		
		
		
		// cout << sensores.energia << endl;
		

		// cout << "Voy a X" << endl;

		EstadoA estado;
		estado.f = sensores.posF;
		estado.c = sensores.posC;
		estado.brujula = sensores.rumbo;
		estado.zapatillas = tiene_zapatillas;
		if(hayPlanBases && HayQueReplanificar(sensores, planBases[indexBases], estado)) {
			// Borrar plan
			// cout << "replanifico..." << endl;
			hayPlanBases = false;
		}

		// cout << "No replanifico..." << endl;


		if (!hayPlanBases)
		{
			// Invocar al método de búsqueda
			// cout << "Buscando plan a X" << endl;
			indexBases = 0;
			planBases.clear();
			EstadoA inicio;
			inicio.f = sensores.posF;
			inicio.c = sensores.posC;
			inicio.brujula = sensores.rumbo;
			inicio.zapatillas = tiene_zapatillas;
			planBases = DijkstraPuestosBase(inicio, sensores);
			VisualizaPlan(inicio, planBases);
			PintaPlan(planBases, tiene_zapatillas);
			hayPlanBases = (planBases.size() != 0);
			// cout << "Plan a X encontrado" << endl;
		}
		if (hayPlanBases && indexBases < planBases.size())
		{
			// cout << "ejecutando plan" << endl;
			accion = planBases[indexBases];
			++indexBases;
		}
		if (indexBases == planBases.size())
		{
			// cout << "Plan a X terminado" << endl;
			hayPlanBases = false;
		}
		return accion;
	}

	// cout << "Doy vueltas hasta cansarme" << endl;

	// return TURN_SR;


	cout << "Voy a ver si me llaman" << endl;
	if(sensores.venpaca) {
		cout << "Me llaman" << endl;
		EstadoA estado;
		estado.f = sensores.posF;
		estado.c = sensores.posC;
		estado.brujula = sensores.rumbo;
		estado.zapatillas = false;
		
		cout << "Voy a ver si replanifico" << endl;
		if(hayPlan && HayQueReplanificar(sensores, plan[index], estado)) {
			cout << "replanifico..." << endl;
			hayPlan = false;
		}
		
		cout << "Comportamiento Nivel 3" << endl;
		accion = ComportamientoAuxiliarNivel_3(sensores);
		cout << "Termine con nivel 3" << endl;
	}
	// Mientras el rescatador no me llame, descubro mapa (similar al nivel 1)
	else {
		cout << "No me llaman" << endl;
		recargar = true;
		hayPlan = false;
		index = 0;
		plan.clear();
		if(conozco_bases) {
			// Dijkstra a puestos base
			// cout << "Dijkstra a puestos base" << endl;
	
			// cout << sensores.energia << endl;
			if(sensores.superficie[0] == 'X') {
				// cout << "Recargando..." << endl;
				if(sensores.energia == MAX_ENERGIA) {
					// cout << "Voy a dejar de recargar..." << endl;
					recargar = false;
				}
				
				return IDLE;
			}
			
			
			
			// cout << sensores.energia << endl;
			
	
			// cout << "Voy a X" << endl;
	
			EstadoA estado;
			estado.f = sensores.posF;
			estado.c = sensores.posC;
			estado.brujula = sensores.rumbo;
			estado.zapatillas = tiene_zapatillas;
			if(hayPlanBases && HayQueReplanificar(sensores, planBases[indexBases], estado)) {
				// Borrar plan
				// cout << "replanifico..." << endl;
				hayPlanBases = false;
			}
	
			// cout << "No replanifico..." << endl;
	
	
			if (!hayPlanBases)
			{
				// Invocar al método de búsqueda
				// cout << "Buscando plan a X" << endl;
				indexBases = 0;
				planBases.clear();
				EstadoA inicio;
				inicio.f = sensores.posF;
				inicio.c = sensores.posC;
				inicio.brujula = sensores.rumbo;
				inicio.zapatillas = tiene_zapatillas;
				planBases = DijkstraPuestosBase(inicio, sensores);
				VisualizaPlan(inicio, planBases);
				PintaPlan(planBases, tiene_zapatillas);
				hayPlanBases = (planBases.size() != 0);
				// cout << "Plan a X encontrado" << endl;
			}
			if (hayPlanBases && indexBases < planBases.size())
			{
				// cout << "ejecutando plan" << endl;
				accion = planBases[indexBases];
				++indexBases;
			}
			if (indexBases == planBases.size())
			{
				// cout << "Plan a X terminado" << endl;
				hayPlanBases = false;
			}
			return accion;
		}
		else accion = ComportamientoAuxiliarNivel_1(sensores);
	
	}
	// Cuando llegue o el rescatador aborte misión, repito
	last_action = accion;

	cout << "Termine" << endl;

	return accion;
}

/*********************************** NIVEL E **********************************************/
list<Action> AvanzaASaltosDeCaballo()
{
	list<Action> secuencia;
	secuencia.push_back(WALK);
	secuencia.push_back(WALK);
	secuencia.push_back(TURN_SR);
	secuencia.push_back(TURN_SR);
	secuencia.push_back(WALK);
	return secuencia;
}

vector<Action> ComportamientoAuxiliar::AnchuraAuxiliar(const EstadoA &inicio, const EstadoA &final, const Sensores & sensores)
{
	NodoA current_node;
	list<NodoA> frontier;
	set<NodoA> explored;
	vector<Action> path;

	current_node.estado = inicio; // Asigna el estado inicial al nodo actual
	frontier.push_back(current_node);
	bool SolutionFound = IsSolution(current_node.estado, final);

	while (!SolutionFound && !frontier.empty())
	{
		frontier.pop_front();
		explored.insert(current_node);

		// for each action applicable to current_node do
		// begin
		// 	child ← problem.apply(action, current_node)
		// 	if (problem.Is_Solution(child) then current_node = child
		// 	else if child.state() is not in explored or frontier then
		// 		frontier.insert(child)
		// end
		// if (!problem.Is_Solution(current_node) then
		// 	current_node ← frontier.next()

		if (mapaResultado[current_node.estado.f][current_node.estado.c] == 'D')
		{
			current_node.estado.zapatillas = true;
		}

		Action acciones[2] = {WALK, TURN_SR};
		for (Action accion : acciones)
		{
			NodoA child = current_node;
			bool accesible = true;
			child.estado = applyA(accion, current_node.estado, accesible, sensores);
			if (IsSolution(child.estado, final))
			{
				child.secuencia.push_back(accion);
				current_node = child;
				SolutionFound = true;
			}
			else if (explored.find(child) == explored.end())
			{
				child.secuencia.push_back(accion);
				frontier.push_back(child);
			}
		}

		if (!SolutionFound && !frontier.empty())
		{
			current_node = frontier.front();
			// SolutionFound = IsSolution(current_node.estado, final);
			while(explored.find(current_node) != explored.end() && !frontier.empty())
			{
				frontier.pop_front();
				current_node = frontier.front();
			}
		}
	}

	if (SolutionFound)
		path = current_node.secuencia;

	return path;
}

bool ComportamientoAuxiliar::Find(const NodoA &st, const list<NodoA> &lista)
{
	auto it = lista.begin();
	while (it != lista.end() and !((*it) == st))
	{
		it++;
	}
	return (it != lista.end());
}

bool ComportamientoAuxiliar::IsSolution(const EstadoA &estado, const EstadoA &final)
{
	return estado.f == final.f && estado.c == final.c;
}

bool ComportamientoAuxiliar::CasillaAccesibleAuxiliar(const EstadoA &st, int nivel)
{
	EstadoA next = NextCasillaAuxiliar(st);
	bool check1 = false, check2 = false, check3 = false, check4 = false;
	check1 = mapaResultado[next.f][next.c] != 'P' && mapaResultado[next.f][next.c] != 'M';
	check2 = mapaResultado[next.f][next.c] != 'B' || (mapaResultado[next.f][next.c] == 'B' && st.zapatillas);
	check3 = abs(mapaCotas[next.f][next.c] - mapaCotas[st.f][st.c]) <= 1;
	check4 = 3 <= next.f && next.f < mapaResultado.size() - 3 && 3 <= next.c && next.c < mapaResultado[0].size() - 3;
	if(nivel == 2)
		return check1 && check2 && check3;
	else 
		return check1 && check2 && (check3 || mapaResultado[next.f][next.c] == '?' || mapaResultado[st.f][st.c] == '?') && check4;
}

EstadoA ComportamientoAuxiliar::NextCasillaAuxiliar(const EstadoA &st)
{
	EstadoA next = st;
	next.f += sf[st.brujula];
	next.c += sc[st.brujula];
	return next;

	// EstadoA siguiente = st;
	// switch (st.brujula)
	// {
	// case norte:
	// 	siguiente.f = st.f - 1;
	// 	break;
	// case noreste:
	// 	siguiente.f = st.f - 1;
	// 	siguiente.c = st.c + 1;
	// 	break;
	// case este:
	// 	siguiente.c = st.c + 1;
	// 	break;
	// case sureste:
	// 	siguiente.f = st.f + 1;
	// 	siguiente.c = st.c + 1;
	// 	break;
	// case sur:
	// 	siguiente.f = st.f + 1;
	// 	break;
	// case suroeste:
	// 	siguiente.f = st.f + 1;
	// 	siguiente.c = st.c - 1;
	// 	break;
	// case oeste:
	// 	siguiente.c = st.c - 1;
	// 	break;
	// case noroeste:
	// 	siguiente.f = st.f - 1;
	// 	siguiente.c = st.c - 1;
	// }
	// return siguiente;
}

void ComportamientoAuxiliar::AnularMatrizA(vector<vector<unsigned char>> &m)
{
	for (unsigned int i = 0; i < m.size(); i++)
	{
		for (unsigned int j = 0; j < m[i].size(); j++)
		{
			m[i][j] = 0;
		}
	}
}

void ComportamientoAuxiliar::PintaPlan(const vector<Action> &plan, bool zap)
{
	auto it = plan.begin();
	while (it != plan.end())
	{
		if (*it == WALK)
		{
			cout << "W ";
		}
		else if (*it == RUN)
		{
			cout << "R ";
		}
		else if (*it == TURN_SR)
		{
			cout << "r ";
		}
		else if (*it == TURN_L)
		{
			cout << "L ";
		}
		else if (*it == CALL_ON)
		{
			cout << "C ";
		}
		else if (*it == CALL_OFF)
		{
			cout << "c ";
		}
		else if (*it == IDLE)
		{
			cout << "I ";
		}
		else
		{
			cout << "-_ ";
		}
		it++;
	}
	cout << "( longitud " << plan.size();
	if (zap)
		cout << "[Z]";
	cout << ")\n";
}

void ComportamientoAuxiliar::VisualizaPlan(const EstadoA &st, const vector<Action> &plan)
{
	AnularMatrizA(mapaConPlan);
	EstadoA cst = st;
	auto it = plan.begin();
	while (it != plan.end())
	{
		switch (*it)
		{
		case WALK:
			switch (cst.brujula)
			{
			case 0:
				cst.f--;
				break;
			case 1:
				cst.f--;
				cst.c++;
				break;
			case 2:
				cst.c++;
				break;
			case 3:
				cst.f++;
				cst.c++;
				break;
			case 4:
				cst.f++;
				break;
			case 5:
				cst.f++;
				cst.c--;
				break;
			case 6:
				cst.c--;
				break;
			case 7:
				cst.f--;
				cst.c--;
				break;
			}
			mapaConPlan[cst.f][cst.c] = 2;
			break;
		case TURN_SR:
			cst.brujula = (cst.brujula + 1) % 8;
			break;
		}
		it++;
	}
}

EstadoA ComportamientoAuxiliar::applyA(Action accion, const EstadoA &st, bool &accesible, const Sensores & sensores)
{
	EstadoA next = st;
	switch (accion)
	{
	case WALK:
		if (CasillaAccesibleAuxiliar(st, sensores.nivel))
		{
			next = NextCasillaAuxiliar(st);
		}
		else
		{
			accesible = false;
		}
		break;
	case TURN_SR:
		next.brujula = (next.brujula + 1) % 8;
		break;
	}
	return next;
}

// Calcular el mínimo número de giros necesarios para llegar al objetivo
int RefinamientoAuxiliar(const EstadoA & st, const EstadoA & final) {
	// Pendiente
	double m;
	int plus1 = 0;
	if(st.c==final.c) m = (st.f>final.f) ? -INF : INF;
	else if(st.f==final.f) m=0;
	else if((st.f-final.f)==(st.c-final.c)) m=1;
	else if((st.f-final.f)==-(st.c-final.c)) m=-1;
	else {
		m = (double)(st.f-final.f)/(st.c-final.c);
		plus1 = 1;
	}

	// Octante
	int octante;	

	if(final.c>st.c) {
		if(-INF < m && m < -1) octante = 0;
		else if(-1 <= m && m < 0) octante = 1;
		else if(0 <= m && m < 1) octante = 2;
		else octante = 3;
	}
	else if(final.c<st.c) {
		if(-INF < m && m < -1) octante = 4;
		else if(-1 <= m && m < 0) octante = 5;
		else if(0 <= m && m < 1) octante = 6;
		else octante = 7;
	}
	else {
		if(st.f>final.f) octante = 0;
		else octante = 4;
	}
	// Incremento de la heurística
	int inc = ((octante - st.brujula + 8) % 8);

	return inc+plus1;
}

int ComportamientoAuxiliar::Heuristica(const EstadoA &st, const EstadoA &final)
{
	if(st.f == final.f && st.c == final.c)
		return 0;
	// Heurística del máximo
	int h = max(abs(st.f - final.f), abs(st.c - final.c));

	// Refinamiento
	int ref = RefinamientoAuxiliar(st, final);
	
	return h+ref;
}

int ComportamientoAuxiliar::CalcularCoste(Action accion, const EstadoA &st)
{
	int coste = 0;
	char casilla = mapaResultado[st.f][st.c];
	int h = mapaCotas[st.f][st.c];
	int rumbo = st.brujula;
	bool zap = st.zapatillas;
	int h1 = mapaCotas[st.f+sf[rumbo]][st.c+sc[rumbo]];
	//! Cuidado Rescatador
	int dif = h1 - h;
	int inc = 0;
	switch (accion)
	{
		case WALK:
			switch(casilla)
			{
				case 'A':
					coste = 100;
					inc = 10;
					break;
				case 'T':
					coste = 20;
					inc = 5;
					break;
				case 'S':
					coste = 2;
					inc = 1;
					break;
				default:
					coste = 1;
					inc = 0;
					break;
			}
			coste += (dif*inc);
		break;
		case TURN_SR:
			switch(casilla)
			{
				case 'A':
					coste = 16;
					break;
				case 'T':
					coste = 3;
					break;
				default:
					coste = 1;
					break;
			}
		break;
	}

	return coste;
}

vector<Action> ComportamientoAuxiliar::A_Estrella(const EstadoA &inicio, const EstadoA &final, const Sensores & sensores) {
	NodoA current_node;
	min_priority_queue frontier;
	set<EstadoA> explored;
	vector<Action> path;

	int cont = 0;

	current_node.estado = inicio; // Asigna el estado inicial al nodo actual
	current_node.g = 0;
	current_node.h = Heuristica(current_node.estado, final);

	frontier.push(current_node);

	bool SolutionFound = IsSolution(current_node.estado, final);

	while (!SolutionFound && !frontier.empty())
	{
		frontier.pop();

		if (mapaResultado[current_node.estado.f][current_node.estado.c] == 'D')
		{
			current_node.estado.zapatillas = true;
		}
		explored.insert(current_node.estado);


		SolutionFound = IsSolution(current_node.estado, final);
		if(SolutionFound) break;

		Action acciones[2] = {WALK, TURN_SR};
		for(Action accion : acciones) {
			NodoA child;
			child.estado = current_node.estado;
			child.secuencia = current_node.secuencia;

			bool accesible = true;
			child.estado = applyA(accion, current_node.estado, accesible, sensores);
			if(!accesible) continue;
			child.g = current_node.g + CalcularCoste(accion, current_node.estado);
			child.h = Heuristica(child.estado, final);
			
			if (explored.find(child.estado) == explored.end())
			{
				child.secuencia.push_back(accion);
				frontier.push(child);
			}
		}

		if (!SolutionFound && !frontier.empty())
		{
			current_node = frontier.top();
			while(explored.find(current_node.estado) != explored.end() && !frontier.empty())
			{
				frontier.pop();
				current_node = frontier.top();
			}
		}
		SolutionFound = IsSolution(current_node.estado, final);
	}

	if (SolutionFound)
		path = current_node.secuencia;

	return path;
}

Action ComportamientoAuxiliar::ComportamientoAuxiliarNivel_E(Sensores sensores)
{
	Action accion = IDLE;
	if (!hayPlan)
	{
		index = 0;
		plan.clear();
		// Invocar al método de búsqueda
		EstadoA inicio, fin;
		inicio.f = sensores.posF;
		inicio.c = sensores.posC;
		inicio.brujula = sensores.rumbo;
		inicio.zapatillas = false;
		fin.f = sensores.destinoF;
		fin.c = sensores.destinoC;
		plan = AnchuraAuxiliar(inicio, fin, sensores);
		VisualizaPlan(inicio, plan);
		hayPlan = plan.size() != 0;
	}
	if (hayPlan && index < plan.size())
	{
		accion = plan[index];
		++index;
	}
	if (index == plan.size())
	{
		hayPlan = false;
	}
	return accion;
}