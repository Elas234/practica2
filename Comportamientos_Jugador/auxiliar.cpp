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
		accion = ComportamientoAuxiliarNivel_4 (sensores);
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
		if(sensores.superficie[i] == 'X' && !baneadas[nf][nc] && !conozco_bases) {
			veo_base = true;
			pos_base = {nf, nc};
		}
		if(sensores.superficie[i] == 'D' && !baneadas[nf][nc] && !tiene_zapatillas) {
			pos_zapatillas = {nf, nc};
			conozco_zapatillas = true;
		}
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

void GenerateLevelA(int level, vector<int> & filas, vector<int> & columnas) {
	filas.clear();
	columnas.clear();
	int n = -level;
	for(int i=0; i<=level; ++i) {
		filas.push_back(n);
		columnas.push_back(i);
	}
	while(n < level) {
		++n;
		filas.push_back(n);
		columnas.push_back(level);
	}
	for(int i=level-1; i>=-level; --i) {
		filas.push_back(level);
		columnas.push_back(i);
	}
	while(n > -level) {
		--n;
		filas.push_back(n);
		columnas.push_back(-level);
	}
}

void ComportamientoAuxiliar::OndaDeCalor(int f, int c) {

	int calor = 32;
	const int MAX_LEVEL = 5;
	const int LEVELS = 2;
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

	// Imprimir el mapa de frecuencias
	// for (int i = 0; i < mapaFrecuencias.size(); ++i) {
	// 	for (int j = 0; j < mapaFrecuencias[i].size(); ++j) {
	// 		cout << setw(3) << mapaFrecuencias[i][j] << " ";
	// 	}
	// 	cout << endl;
	// }

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
		// OndaDeCalor(sensores.posF, sensores.posC);
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
		plan = A_Estrella(inicio, fin, sensores).secuencia;
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

bool ComportamientoAuxiliar::HayQueReplanificar(const Sensores & sensores, const Action & accion) {
	
	DecideOpcionReplanificacion(sensores);

	// if(opcion_busqueda == NO_PERMISIVA) {
	// 	cout << "Opcion busqueda: NO_PERMISIVA" << endl;
	// }
	// else {
	// 	cout << "Opcion busqueda: PERMISIVA" << endl;
	// }
	// if(opcion_replanificacion == NO_PERMISIVA) {
	// 	cout << "Opcion replanificacion: NO_PERMISIVA" << endl;
	// }
	// else {
	// 	cout << "Opcion replanificacion: PERMISIVA" << endl;
	// }

	EstadoA estado = {sensores.posF, sensores.posC, sensores.rumbo, tiene_zapatillas};
	//if(sensores.superficie[2] == 'X' && buscando_recarga) return true;
	if(sensores.choque)  {	
		return true;
	}
	if(accion == TURN_SR) return false;

	cout << "Casilla accesible ?" << (char)sensores.superficie[2] << endl;

	// cout << (char)(mapaResultado[next.f][next.c]) << endl;
	if(!CasillaAccesibleAuxiliarOpciones(estado, opcion_replanificacion)) {
		return true;
	}

	return false;
}

void ComportamientoAuxiliar::DecideOpcionReplanificacion(const Sensores & sensores)
{
	bool estoy_cerca = max(abs(sensores.posF - sensores.destinoF), abs(sensores.posC - sensores.destinoC)) <= RADIO_INMERSION+4;
	bool busco_base = (veo_base && !conozco_bases);
	bool busco_zapatillas = (conozco_zapatillas && !tiene_zapatillas);
	if ((sensores.superficie[0] == 'A' || sensores.superficie[0] == 'T' || sensores.superficie[0] == 'B') || estoy_cerca || camino_duro || busco_base || busco_zapatillas) {
		opcion_replanificacion = PERMISIVA;
		opcion_busqueda = PERMISIVA;
	}
	else {
		opcion_replanificacion = NO_PERMISIVA;
		opcion_busqueda = NO_PERMISIVA;
	}

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
// Action ComportamientoAuxiliar::ComportamientoAuxiliarNivel_4(Sensores sensores)
// {
// 	Action accion = IDLE;

// 	const int UMBRAL_TIEMPO = 4*mapaResultado.size();
// 	const int UMBRAL_ENERGIA = 20*mapaResultado.size();
// 	const int MAX_ENERGIA = 3000;

// 	SituarSensorEnMapa(sensores);
// 	if(last_action == WALK)
// 		OndaDeCalor(sensores.posF, sensores.posC);

// 	if(sensores.superficie[0] == 'D') {
// 		tiene_zapatillas = true;
// 	}

// 	if(sensores.superficie[0] != 'X') {
// 		recargar = sensores.energia <= UMBRAL_ENERGIA;
// 	}

// 	/**********************************************************************************************
// 	 * ? RECARGAR ENERGÍA 
// 	 **********************************************************************************************/

// 	if(recargar && conozco_bases) {
// 		// Dijkstra a puestos base
// 		// cout << "Dijkstra a puestos base" << endl;
// 		hayPlan = false;
// 		plan.clear();
// 		index = 0;

// 		// cout << sensores.energia << endl;
// 		if(sensores.superficie[0] == 'X') {
// 			// cout << "Recargando..." << endl;
// 			if(sensores.energia == MAX_ENERGIA) {
// 				// cout << "Voy a dejar de recargar..." << endl;
// 				recargar = false;
// 			}
			
// 			return IDLE;
// 		}
		
		
		
// 		// cout << sensores.energia << endl;
		

// 		// cout << "Voy a X" << endl;

// 		EstadoA estado;
// 		estado.f = sensores.posF;
// 		estado.c = sensores.posC;
// 		estado.brujula = sensores.rumbo;
// 		estado.zapatillas = tiene_zapatillas;
// 		if(hayPlanBases && HayQueReplanificar(sensores, planBases[indexBases], estado)) {
// 			// Borrar plan
// 			// cout << "replanifico..." << endl;
// 			hayPlanBases = false;
// 		}

// 		// cout << "No replanifico..." << endl;


// 		if (!hayPlanBases)
// 		{
// 			// Invocar al método de búsqueda
// 			// cout << "Buscando plan a X" << endl;
// 			indexBases = 0;
// 			planBases.clear();
// 			EstadoA inicio;
// 			inicio.f = sensores.posF;
// 			inicio.c = sensores.posC;
// 			inicio.brujula = sensores.rumbo;
// 			inicio.zapatillas = tiene_zapatillas;
// 			planBases = DijkstraPuestosBase(inicio, sensores);
// 			VisualizaPlan(inicio, planBases);
// 			PintaPlan(planBases, tiene_zapatillas);
// 			hayPlanBases = (planBases.size() != 0);
// 			// cout << "Plan a X encontrado" << endl;
// 		}
// 		if (hayPlanBases && indexBases < planBases.size())
// 		{
// 			// cout << "ejecutando plan" << endl;
// 			accion = planBases[indexBases];
// 			++indexBases;
// 		}
// 		if (indexBases == planBases.size())
// 		{
// 			// cout << "Plan a X terminado" << endl;
// 			hayPlanBases = false;
// 		}
// 		return accion;
// 	}

// ? REDO 

void ComportamientoAuxiliar::AnularRecarga() {
	recargando = false;
	buscando_recarga = false;
	contador = 0;
	plan.clear();
	index = 0;
	hayPlan = false;
}

void ComportamientoAuxiliar::AnularPlan() {
	plan.clear();
	index = 0;
	hayPlan = false;
	ejecutando_plan_objetivo = false;
}

Action ComportamientoAuxiliar::PlanRecarga(Sensores sensores) {

	Action accion = IDLE;

	if(ejecutando_plan_objetivo) {
		AnularPlan();
		ejecutando_plan_objetivo = false;
	}


	if(sensores.superficie[0] == 'X') {
		cout << "NO NECESITO PLAN" << endl;
		return IDLE;
	}

	// Invocar al método de búsqueda
	EstadoA estado;
	// NodoA nodo_base;
	estado.f = sensores.posF;
	estado.c = sensores.posC;
	estado.brujula = sensores.rumbo;
	estado.zapatillas = tiene_zapatillas;
	vector<char> casillas_desconocidas = {'T','S','C'};
	for(int i=0; i<2; ++i) {
		for(char c : casillas_desconocidas) {
			cout << "Buscando plan a X" << endl;
			desconocidas_busqueda = c;
			NodoA nodo_base = BuscaCasillas(estado, sensores, {'X'});
			camino_duro = CaminoDuro(estado, {INF,INF}, nodo_base.secuencia);
			if(camino_duro) {
				opcion_replanificacion = PERMISIVA;
				cout << "Camino duro" << endl;
			}
			if(nodo_base.g <= sensores.energia) {
				plan = nodo_base.secuencia;
			}
			// else {
			// 	if(opcion_busqueda == PERMISIVA) {
			// 		cout << "ERROR : PLAN RECARGA IMPOSIBLE" << endl;
			// 	}
			// 	opcion_busqueda = PERMISIVA;
			// 	cout << "NO LLEGO" << endl;
			// }
		
			hayPlan = (plan.size() != 0);
			if(hayPlan) break;
		}
		if(hayPlan) break;
		if(opcion_busqueda == PERMISIVA) break;
		opcion_busqueda = PERMISIVA;
	}
	VisualizaPlan(estado, plan);
	// PintaPlan(plan, tiene_zapatillas);

	return accion;
}

bool ComportamientoAuxiliar::EncontreCasilla(const EstadoA &st, vector<char> casillas_objetivo) {
	for (char c : casillas_objetivo) {
		if (mapaResultado[st.f][st.c] == c) {
			return true;
		}
	}
	return false;
}

NodoA ComportamientoAuxiliar::BuscaCasillas(const EstadoA &inicio, const Sensores & sensores, const vector<char>& casillas_objetivo) {
	NodoA current_node;
	min_priority_queue frontier;
	set<EstadoA> explored;
	NodoA solucion;

	current_node.estado = inicio; // Asigna el estado inicial al nodo actual
	current_node.g = 0;
	// current_node.h = Heuristica(current_node.estado, final);

	frontier.push(current_node);

	bool SolutionFound = EncontreCasilla(current_node.estado, casillas_objetivo);

	while (!SolutionFound && !frontier.empty())
	{
		// cout << "Explorando nodo: " << current_node.estado.f << ", " << current_node.estado.c << endl;
		frontier.pop();

		if (mapaResultado[current_node.estado.f][current_node.estado.c] == 'D')
		{
			current_node.estado.zapatillas = true;
		}
		explored.insert(current_node.estado);


		SolutionFound = EncontreCasilla(current_node.estado, casillas_objetivo);
		if(SolutionFound) break;

		Action acciones[2] = {WALK, TURN_SR};
		for(Action accion : acciones) {
			NodoA child;
			child.estado = current_node.estado;
			
			child.secuencia = current_node.secuencia;

			bool accesible = true;
			child.estado = applyA(accion, current_node.estado, accesible, sensores);
			if(!accesible) continue;
			// if (mapaResultado[child.estado.f][child.estado.c] == '?') continue;
			int coste = CalcularCoste(accion, current_node.estado);
			// Las considero como agua por si acaso
			// if (mapaResultado[child.estado.f][child.estado.c] == '?') {
			// 	switch(accion) {
			// 		case RUN:
			// 			coste = 165;
			// 			break;
			// 		case WALK:
			// 			coste = 110;
			// 			break;
			// 		case TURN_SR:
			// 			coste = 16;
			// 			break;
			// 		case TURN_L:
			// 			coste = 30;
			// 			break;
			// 	}
			// }
			child.g = current_node.g + coste;
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
		SolutionFound = EncontreCasilla(current_node.estado, casillas_objetivo);
		plan_inseguro = mapaResultado[current_node.estado.f][current_node.estado.c] == '?';
	}

	if (SolutionFound) 
		solucion = current_node;

	return solucion;
}

// Simula el plan y comprueba si tiene agua o matorrales
bool ComportamientoAuxiliar::CaminoDuro(const EstadoA & inicio, const EstadoA & fin, const vector<Action> & plan) {
	EstadoA estado = inicio;
	bool duro = false;
	for (Action accion : plan) {
		if (accion == WALK) {
			estado.f += sf[estado.brujula];
			estado.c += sc[estado.brujula];
			char casilla = mapaResultado[estado.f][estado.c];
			// cout << casilla << endl;
			if (casilla == 'A' || casilla == 'T') {
				if (max(abs(estado.f - fin.f), abs(estado.c - fin.c)) > RADIO_INMERSION+3) {
					return true;
				}
			}
		} else if (accion == TURN_SR) {
			estado.brujula = (estado.brujula + 1) % 8;
		}
	}
	// cout << "Camino duro? " << boolalpha << duro << endl;
	return false;
}

bool ComportamientoAuxiliar::PlanCasillas(Sensores sensores, const vector<char> & casillas) {
	AnularPlan();
	NodoA nodo_base;
	int coste = 0;
	EstadoA estado;
	estado.f = sensores.posF;
	estado.c = sensores.posC;
	estado.brujula = sensores.rumbo;
	estado.zapatillas = tiene_zapatillas;
	bool imposible = false;

	vector<char> casillas_desconocidas = {'T', 'S', 'C'};

	for(int i=0; i<2; ++i) {
		for(char c : casillas_desconocidas) {
			desconocidas_busqueda = c;
			nodo_base = BuscaCasillas(estado, sensores, casillas);
			camino_duro = CaminoDuro(estado, {INF,INF}, nodo_base.secuencia);
			plan_inseguro = nodo_base.path_inseguro;
			// Compruebo si llego (si no estoy muy seguro, no me arriesgo)
			coste = nodo_base.g;
			imposible = nodo_base.secuencia.size() == 0;
			cout << "Imposible?" << boolalpha << imposible << endl;
			bool llego = coste < sensores.energia;
			if(!imposible && llego) 
				plan = nodo_base.secuencia;
			// else if(imposible) {
			// 	if(opcion_busqueda == PERMISIVA) {
			// 		cout << "ERROR : PLAN IMPOSIBLE" << endl;
			// 	}
			// 	opcion_busqueda = PERMISIVA;
			// 	cout << "NO LLEGO" << endl;
			// }
			hayPlan = (plan.size() != 0);
			if(hayPlan) break;
		}
		if(hayPlan) break;
		opcion_busqueda = PERMISIVA;
	}
	VisualizaPlan(estado, plan);
	PintaPlan(plan, tiene_zapatillas);
	
	return !imposible;
}

Action ComportamientoAuxiliar::UltimoRecurso(Sensores sensores) {
	Action accion = IDLE;
	EstadoA estado;
	estado.f = sensores.posF;
	estado.c = sensores.posC;
	estado.brujula = sensores.rumbo;
	estado.zapatillas = tiene_zapatillas;

	if(CasillaAccesibleAuxiliarOpciones(estado, 1)) {
		accion = WALK;
	}
	else {
		accion = TURN_SR;
	}
	return accion;
}

Action ComportamientoAuxiliar::OrganizaPlan(Sensores sensores) {
	Action accion = IDLE;
	cout << "Organizando plan..." << endl;
	cout << "Hay plan? " << boolalpha << hayPlan << endl;
	if(hayPlan) {

		if(index == plan.size()) {
			AnularPlan();
		}
		
		else {

			
			if(HayQueReplanificar(sensores, plan[index])) {
				// Borrar plan
				cout << "replanifico..." << endl;
				AnularPlan();
				// if(buscando_recarga) {
				// 	AnularRecarga();
				// }
				
				accion = IDLE;
			}
			else if(!necesito_recargar) {
				// cout << "Estoy siguiendo un plan" << endl;
				accion = plan[index];
				++index;
				return accion;
			}
		}
	}
	for(int i=1; i<16;++i) {
		pair<int,int> pos = VtoM(i,sensores.rumbo,{sensores.posF, sensores.posC});
		mapaEntidades[pos.first][pos.second] = '?';
	}
	return accion;
}

bool EstoyCercaObjetivo(const EstadoA & estado, const EstadoA & objetivo) {
	
	int distancia = max(abs(estado.f - objetivo.f), abs(estado.c - objetivo.c));
	return distancia <= 3;
}

NodoA ComportamientoAuxiliar::AproximacionObjetivo(const EstadoA &inicio, const EstadoA &final, const Sensores & sensores) {
	NodoA current_node;
	min_priority_queue frontier;
	set<EstadoA> explored;
	NodoA solucion;

	int cont = 0;

	current_node.estado = inicio; // Asigna el estado inicial al nodo actual
	current_node.g = 0;
	// current_node.h = Heuristica(current_node.estado, final);

	frontier.push(current_node);

	bool SolutionFound = EstoyCercaObjetivo(current_node.estado, final);

	while (!SolutionFound && !frontier.empty())
	{
		frontier.pop();

		if (mapaResultado[current_node.estado.f][current_node.estado.c] == 'D')
		{
			current_node.estado.zapatillas = true;
		}
		explored.insert(current_node.estado);


		SolutionFound = EstoyCercaObjetivo(current_node.estado, final);
		if(SolutionFound) break;

		Action acciones[2] = {WALK, TURN_SR};
		for(Action accion : acciones) {
			NodoA child;
			child.estado = current_node.estado;
			child.secuencia = current_node.secuencia;

			bool accesible = true;
			child.estado = applyA(accion, current_node.estado, accesible, sensores);
			if(!accesible) continue;
			char casilla = mapaResultado[child.estado.f][child.estado.c];
			if(opcion_busqueda == NO_PERMISIVA && (casilla == 'A' || casilla == 'T')) {
				if(max(abs(child.estado.f - final.f), abs(child.estado.c - final.c)) > RADIO_INMERSION+3) {
					continue;
				}
				// continue;
			}
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
		SolutionFound = EstoyCercaObjetivo(current_node.estado, final);
	}

	if (SolutionFound) {
		solucion = current_node;
	}

	return solucion;
}

void ComportamientoAuxiliar::BuscaObjetivo(Sensores sensores, int f, int c)
{
	if(f == -1) f = sensores.destinoF;
	if(c == -1) c = sensores.destinoC;
	Action accion = IDLE;
	
	NodoA nodo_objetivo;
	NodoA nodo_base;
	NodoA nodo_camino;
	NodoA nodo_camino2;
	pair<vector<Action>,int> path_coste;
	EstadoA inicio, fin;
	inicio.f = sensores.posF;
	inicio.c = sensores.posC;
	inicio.brujula = sensores.rumbo;
	fin.f = f;
	fin.c = c;
	
	inicio.zapatillas = tiene_zapatillas;
	vector<char> desconocidas = {'T', 'S', 'C'};
	for(int i=0; i<2; ++i) {
		for(char casilla_desconocida : desconocidas) {

			desconocidas_busqueda = casilla_desconocida;
			cout << "Trato '?' como " << (char)casilla_desconocida << endl;

			nodo_objetivo = AproximacionObjetivo(inicio, fin, sensores);
			bool imposible = nodo_objetivo.secuencia.size() == 0;
			camino_duro = CaminoDuro(inicio, fin, nodo_objetivo.secuencia);
			if(camino_duro) {
				opcion_replanificacion = PERMISIVA;
				cout << "Camino duro" << endl;
			}
			if(!imposible)
				nodo_base = BuscaCasillas(nodo_objetivo.estado, sensores, {'X'});
			// UnionVectores(nodo_objetivo.secuencia, nodo_base.secuencia, path_coste.first);
			path_coste.second = nodo_objetivo.g + nodo_base.g;
			plan_inseguro = nodo_base.path_inseguro || nodo_objetivo.path_inseguro;
			// Compruebo si llego (si no estoy muy seguro, no me arriesgo mucho)
			// cout << "Plan inseguro? " << boolalpha << plan_inseguro << endl;
			
			// cout << "Imposible?" << boolalpha << imposible << endl;
			bool llego = path_coste.second <= sensores.energia;
			// ! Estoy asumiendo que siempre es posible, pero puede que no llegue (YA NO)
			if(!imposible && llego) {
				plan = nodo_objetivo.secuencia;
				ejecutando_plan_objetivo = true;
				// if(opcion_busqueda == NO_PERMISIVA) {
				// 	// cout << "Plan restrictivo" << endl;
				// 	hay_plan_restrictivo = true;
				// }
			}
			// else if(imposible) {
			// 	if(opcion_busqueda == PERMISIVA) {
			// 		cout << "ERROR : PLAN OBJETIVO IMPOSIBLE" << endl;
			// 	}
			// 	opcion_busqueda = PERMISIVA;
			// 	cout << "NO LLEGO" << endl;

			// }
			else {
				// hay_plan_restrictivo = false;
				necesito_recargar = true;
				// if(opcion_busqueda == PERMISIVA) {
				// 	cout << "ERROR : PLAN OBJETIVO IMPOSIBLE" << endl;
				// }
				// opcion_busqueda = PERMISIVA;
				cout << "NO LLEGO" << endl;
			}
			energia_necesaria = path_coste.second;
			
			// cout << "No es Dijkstra" << endl;
			VisualizaPlan(inicio, plan);
			// PintaPlan(plan, tiene_zapatillas);
			// cout << plan.size() << endl;
			hayPlan = (plan.size() != 0);
			if(hayPlan) break;
		}
		if(hayPlan) break;
		opcion_busqueda = PERMISIVA;
	}
	
	return;
}


Action ComportamientoAuxiliar::ComportamientoAuxiliarNivel_4(Sensores sensores) {
	Action accion = IDLE;

	// Actualizar el mapa con la información de los sensores
	SituarSensorEnMapa(sensores);
	for(int i=1; i<16;++i) {
		if(sensores.agentes[i] != '_') {
			pair<int,int> pos = VtoM(i,sensores.rumbo,{sensores.posF, sensores.posC});
			mapaEntidades[pos.first][pos.second] = sensores.agentes[i];
		}
	}

	if(sensores.superficie[0] == 'D') {
		tiene_zapatillas = true;
	}

	// Si el rescatador llama, ir al objetivo
	if (sensores.venpaca) {
		if(ocioso) {
			AnularPlan();
			recargando = false;
			ocioso = false;
		}
		if (!hayPlan || HayQueReplanificar(sensores, accion)) {
			// Generar un plan para ir al objetivo
			index = 0;
			plan.clear();
			EstadoA inicio, fin;
			inicio.f = sensores.posF;
			inicio.c = sensores.posC;
			inicio.brujula = sensores.rumbo;
			inicio.zapatillas = tiene_zapatillas;
			fin.f = sensores.destinoF;
			fin.c = sensores.destinoC;

			plan = AproximacionObjetivo(inicio, fin, sensores).secuencia;
			VisualizaPlan(inicio, plan);
			hayPlan = !plan.empty();
		}
		if(!hayPlan && !tiene_zapatillas) {
			char objetivo = 'D';
			if(!conozco_zapatillas) objetivo = '?';
			EstadoA inicio;
			inicio.f = sensores.posF;
			inicio.c = sensores.posC;
			inicio.brujula = sensores.rumbo;
			inicio.zapatillas = tiene_zapatillas;

			plan = BuscaCasillas(inicio, sensores, {objetivo}).secuencia;
			VisualizaPlan(inicio, plan);
			hayPlan = !plan.empty();
		}

		// Ejecutar el plan
		if (hayPlan && index < plan.size()) {
			accion = plan[index];
			++index;
		}

		// Si se completó el plan, resetear
		if (index == plan.size()) {
			hayPlan = false;
		}

		

		if(EstoyCercaObjetivo({sensores.posF, sensores.posC, sensores.rumbo, tiene_zapatillas}, {sensores.destinoF, sensores.destinoC})) {
			accion = TURN_SR;
		}

		for(int i=1; i<16;++i) {
			pair<int,int> pos = VtoM(i,sensores.rumbo,{sensores.posF, sensores.posC});
			mapaEntidades[pos.first][pos.second] = '?';
		}

		return accion;
	}

	pair<int,int> pos_en_frente = VtoM(2,sensores.rumbo,{sensores.posF, sensores.posC});
	if(sensores.superficie[2] == 'X' && sensores.agentes[2] == 'r' && !baneadas[pos_en_frente.first][pos_en_frente.second]) {
		if(ViablePorAltura(abs(sensores.cota[0] - sensores.cota[2]))) {
			AnularPlan();
			// recargando = false;
			return WALK;
		}
		else {
			baneadas[pos_en_frente.first][pos_en_frente.second] = true;
			AnularRecarga();
		}
		
	}

	cout << "Recargando? " << boolalpha << recargando << endl;
	ocioso = true;
	// Si no hay llamada del rescatador, ir a recargar
	if(sensores.energia < 2000 || recargando) {
		if (sensores.superficie[0] == 'X') {
			// Si ya está en un puesto base, recargar
			recargando = true;
			conozco_bases = true;
			if(sensores.energia < 3000)
				return IDLE;
			else recargando = false;
		} else {
			if(index < plan.size()) {
				accion = plan[index];
			}
			if (!hayPlan || HayQueReplanificar(sensores, accion)) {
				// Generar un plan para ir al puesto base más cercano

				cout << "Hay que replanificar..." << endl;
				index = 0;
				plan.clear();
				char objetivo = 'X';
				if(!veo_base && !conozco_bases) objetivo = '?';
				EstadoA inicio;
				inicio.f = sensores.posF;
				inicio.c = sensores.posC;
				inicio.brujula = sensores.rumbo;
				inicio.zapatillas = tiene_zapatillas;

				plan = BuscaCasillas(inicio, sensores, {objetivo}).secuencia;
				VisualizaPlan(inicio, plan);
				hayPlan = !plan.empty();
				
			}

			// Ejecutar el plan
			if (hayPlan && index < plan.size()) {
				accion = plan[index];
				++index;
			}

			// Si se completó el plan, resetear
			if (index == plan.size()) {
				hayPlan = false;
			}

			for(int i=1; i<16;++i) {
				pair<int,int> pos = VtoM(i,sensores.rumbo,{sensores.posF, sensores.posC});
				mapaEntidades[pos.first][pos.second] = '?';
			}

			return accion;
		}
	}
	else {
		cout << "Hay plan? " << boolalpha << hayPlan << endl;
		cout << "index = " << index << endl;
		cout << "Tamaño plan = " << plan.size() << endl;
		PintaPlan(plan, tiene_zapatillas);
		if(index < plan.size()) {
			accion = plan[index];
		}
		cout << "Accion = " << accion << endl;
		if (!hayPlan || HayQueReplanificar(sensores, accion)) {
			// Generar un plan para ir al puesto base más cercano

			cout << "Hay que replanificar..." << endl;
			index = 0;
			plan.clear();
			EstadoA inicio;
			inicio.f = sensores.posF;
			inicio.c = sensores.posC;
			inicio.brujula = sensores.rumbo;
			inicio.zapatillas = tiene_zapatillas;

			plan = BuscaCasillas(inicio, sensores, {'?'}).secuencia;
			VisualizaPlan(inicio, plan);
			hayPlan = !plan.empty();
			
		}

		if(!hayPlan) {
			recargando = true;
			for(int i=1; i<16;++i) {
				pair<int,int> pos = VtoM(i,sensores.rumbo,{sensores.posF, sensores.posC});
				mapaEntidades[pos.first][pos.second] = '?';
			}
			return IDLE;
		}

		// Ejecutar el plan
		if (hayPlan && index < plan.size()) {
			accion = plan[index];
			++index;
		}

		// Si se completó el plan, resetear
		if (index == plan.size()) {
			hayPlan = false;
		}

		for(int i=1; i<16;++i) {
			pair<int,int> pos = VtoM(i,sensores.rumbo,{sensores.posF, sensores.posC});
			mapaEntidades[pos.first][pos.second] = '?';
		}

		return accion;
	}

	return accion;
}

// Action ComportamientoAuxiliar::ComportamientoAuxiliarNivel_4(Sensores sensores) {
// 	Action accion = IDLE;
// 	instantes++;
// 	cout << "Tamaño plan = " << plan.size() << endl;

// 	// cout << "Necesito recargar? " << boolalpha << necesito_recargar << endl;
// 	// cout << "Recargando? " << boolalpha << recargando << endl;

// 	int S = mapaResultado.size();
// 	bool mapa_grande = S > 60;

// 	const int UMBRAL_TIEMPO = 1;//0*S;
// 	const int INF_ENERGIA = mapa_grande ? 1000 : 500;
// 	const int SUP_ENERGIA = mapa_grande ? 3000 : 2000;
// 	const int MAX_ENERGIA = 3000;
// 	const int TIEMPO_RECARGA_CASUAL = 0;
// 	const int ACCIONES_EXPLORACION = 500;
// 	// const int UMBRAL_INSTANTES = 1500 + S*sqrt(S);

// 	/**********************************************************************************************
// 	 * ? ACCIONES INICIALES
// 	 **********************************************************************************************/

// 	SituarSensorEnMapa(sensores);

// 	for(int i=1; i<16;++i) {
// 		if(sensores.agentes[i] != '_') {
// 			pair<int,int> pos = VtoM(i,sensores.rumbo,{sensores.posF, sensores.posC});
// 			mapaEntidades[pos.first][pos.second] = sensores.agentes[i];
// 		}
// 	}

// 	// Imprimir el mapa de entidades
// 	// for (int i = 0; i < mapaEntidades.size(); ++i) {
// 	// 	for (int j = 0; j < mapaEntidades[i].size(); ++j) {
// 	// 		cout << mapaEntidades[i][j] << " ";
// 	// 	}
// 	// 	cout << endl;
// 	// }


// 	if(sensores.superficie[0] == 'D') {
// 		tiene_zapatillas = true;
// 		conozco_zapatillas = false;
// 	}
// 	pair<int,int> pos_en_frente = VtoM(2,sensores.rumbo,{sensores.posF, sensores.posC});
// 	if(sensores.superficie[2] == 'X' && buscando_recarga && !ejecutando_plan_objetivo && !baneadas[pos_en_frente.first][pos_en_frente.second]) {
// 		if(ViablePorAltura(abs(sensores.cota[0] - sensores.cota[2]))) {
// 			AnularPlan();
// 			return WALK;
// 		}
// 		else {
// 			baneadas[pos_en_frente.first][pos_en_frente.second] = true;
// 			AnularRecarga();
// 		}
		
// 	}

// 	cout << "Necesito recargar? " << boolalpha << necesito_recargar << endl;
// 	cout << "Buscando recarga? " << boolalpha << buscando_recarga << endl;
// 	cout << "energia necesaria? " << energia_necesaria << endl;

// 	if(recargando && sensores.choque) {
// 		AnularRecarga();
// 	}

// 	if(buscando_recarga) {
// 		// cout << energia_necesaria << endl;
// 		// cout << min(max(SUP_ENERGIA, energia_necesaria), MAX_ENERGIA) << endl;
// 		recargando = sensores.energia < min(max(SUP_ENERGIA, energia_necesaria), MAX_ENERGIA);
// 		// cout << "Recargando = " << boolalpha << recargando << endl;
// 	}
// 	else {
// 		necesito_recargar = sensores.energia < max(INF_ENERGIA, energia_necesaria);
// 	}
// 	// energia_necesaria = 0;


// 	if(sensores.superficie[0] == 'X') {
// 		veo_base = false;
// 		conozco_bases = true;
// 		if((recargando || contador < TIEMPO_RECARGA_CASUAL)) {	
// 			contador++;
// 			return IDLE;
// 		} else {
// 			AnularRecarga();
// 		}
// 	}
// 	else 

// 	/**********************************************************************************************
// 	 * ? RECARGAR ENERGÍA 
// 	 **********************************************************************************************/

// 	if(necesito_recargar) {
// 		buscando_recarga = true;
// 		necesito_recargar = false;
// 		// Dijkstra a puestos base
// 		// cout << "EJECUTANDO PLAN DE RECARGA" << endl;
// 		accion = PlanRecarga(sensores);
// 		if(hayPlan)
// 			return accion;
// 	}

// 	/**********************************************************************************************
// 	 * ? ACUDIR A LA LLAMADA DEL RESCATADOR
// 	 **********************************************************************************************/
// 	cout << "Conozco bases " << boolalpha << conozco_bases << endl;
// 	if(sensores.venpaca && conozco_bases && !ejecutando_plan_objetivo) {
// 		cout << "uwu" << endl;
// 		if(buscando_recarga && sensores.energia >= max(INF_ENERGIA, energia_necesaria)) {
// 			AnularRecarga();
// 		}
// 		if(!hayPlan) {



// 			// Si me choco o me voy a caer, recalculo el plan 
// 			// cout << "Tengo plan? " << boolalpha << hayPlan << endl;
// 			// cout << "Tengo que replanificar? " << boolalpha << HayQueReplanificar(sensores, accion) << endl;
			
// 			BuscaObjetivo(sensores);

// 			bool estoy_cerca_objetivo = EstoyCercaObjetivo({sensores.posF, sensores.posC}, {sensores.destinoF, sensores.destinoC});
// 			if(hayPlan) {
// 				ejecutando_plan_objetivo = true;
// 			}
// 			else if(estoy_cerca_objetivo) {
// 				accion = TURN_SR;
// 				return accion;
// 			}
// 			else {
// 				opcion_busqueda = PERMISIVA;
// 				opcion_replanificacion = PERMISIVA;
// 			}
			
// 		}
		
// 		// cout << "Hay plan restrictivo? " << boolalpha << hay_plan_restrictivo << endl;
		
// 	}
	

// 	/**********************************************************************************************
// 	 * ? EXPLORAR UN POCO DE MAPA AL INICIO AL MENOS HASTA ENCONTRAR UN PUESTO BASE Y ZAPATILLAS
// 	 **********************************************************************************************/
	
// 	 cout << "Conozco bases? " << boolalpha << conozco_bases << endl;
// 	 cout << "Tiene zapatillas? " << boolalpha << tiene_zapatillas << endl;
// 		if(!tiene_zapatillas && instantes < ACCIONES_EXPLORACION) {
// 			// Busca zapatillas
// 			if(conozco_zapatillas) {
// 				cout << "Hay plan == " << boolalpha << hayPlan << endl;
// 				if(!hayPlan) {
// 					PlanCasillas(sensores, {'D'});
// 				}
// 				cout << "Planificacion exitosa == " << boolalpha << hayPlan << endl;
// 				if(!hayPlan) {
// 					baneadas[pos_zapatillas.first][pos_zapatillas.second] = true;
// 					conozco_zapatillas = false;
// 				}
// 				accion = OrganizaPlan(sensores);
// 				return accion;
// 			}
// 			else {
				
// 				if(!hayPlan) {
// 					PlanCasillas(sensores, {'?'});
// 				}
// 				accion = OrganizaPlan(sensores);
// 				return accion;
// 			}
// 		}

// 		else if(!conozco_bases) {
// 			if(veo_base) {
// 				cout << "Hay plan == " << boolalpha << hayPlan << endl;

// 				if(!hayPlan) {
// 					PlanCasillas(sensores, {'X'});
// 				}
// 				if(!hayPlan) {
// 					baneadas[pos_base.first][pos_base.second] = true;
// 					veo_base = false;
// 				}
// 				accion = OrganizaPlan(sensores);
// 				return accion;
// 			}
// 			else {
// 				if(!hayPlan) {
// 					opcion_busqueda = NO_PERMISIVA;
// 					PlanCasillas(sensores, {'?'});
// 				}
// 				if(!hayPlan) {
// 					opcion_busqueda = PERMISIVA;
// 					opcion_replanificacion = PERMISIVA;
// 					PlanCasillas(sensores, {'?'});
// 				}
				
// 				accion = OrganizaPlan(sensores);
// 				return accion;
// 			}
			
// 		}
// 	// else if(instantes < ACCIONES_EXPLORACION) {
// 	// 	return ComportamientoAuxiliarNivel_1(sensores);
// 	// }

// 	else {
// 		// cout << "Ya he llegado" << endl;
// 		if(estoy_cerca_objetivo) {
// 			estoy_cerca_objetivo = EstoyCercaObjetivo({sensores.posF, sensores.posC}, {sensores.destinoF, sensores.destinoC});

// 			opcion_replanificacion = NO_PERMISIVA; // Ya he llegado
// 			opcion_busqueda = NO_PERMISIVA; // Ya he llegado
// 			// cout << "Plan casillas " << endl;
// 			// cout << "Superficie: " << (char)(sensores.superficie[0]) << endl;
// 			// cout << "Vida: " << sensores.vida << endl;
// 			// cout << "objetivo_anterior: " << objetivo_anterior.first << " " << objetivo_anterior.second << endl;
// 			// cout << "objetivo: " << sensores.destinoF << " " << sensores.destinoC << endl;
// 			if((objetivo_anterior.first != sensores.destinoF || objetivo_anterior.second != sensores.destinoC)) {
// 				objetivo_anterior = {sensores.destinoF, sensores.destinoC};
// 				// cout << "Nuevo objetivo" << endl;
				
			
// 			}
// 		}
// 		if(!hayPlan && !buscando_recarga) {
// 			buscando_recarga = true;
// 			PlanRecarga(sensores);
// 			accion = PlanRecarga(sensores);
// 			return accion;
// 		}
		
// 	}

	
	
// 	accion = OrganizaPlan(sensores);

	
	
// 	return accion;
// }


// 	// cout << "Doy vueltas hasta cansarme" << endl;

// 	// return TURN_SR;


// 	cout << "Voy a ver si me llaman" << endl;
// 	if(sensores.venpaca) {
// 		cout << "Me llaman" << endl;
// 		EstadoA estado;
// 		estado.f = sensores.posF;
// 		estado.c = sensores.posC;
// 		estado.brujula = sensores.rumbo;
// 		estado.zapatillas = false;
		
// 		cout << "Voy a ver si replanifico" << endl;
// 		if(hayPlan && HayQueReplanificar(sensores, plan[index], estado)) {
// 			cout << "replanifico..." << endl;
// 			hayPlan = false;
// 		}
		
// 		cout << "Comportamiento Nivel 3" << endl;
// 		accion = ComportamientoAuxiliarNivel_3(sensores);
// 		cout << "Termine con nivel 3" << endl;
// 	}
// 	// Mientras el rescatador no me llame, descubro mapa (similar al nivel 1)
// 	else {
// 		cout << "No me llaman" << endl;
// 		recargar = true;
// 		hayPlan = false;
// 		index = 0;
// 		plan.clear();
// 		if(conozco_bases) {
// 			// Dijkstra a puestos base
// 			// cout << "Dijkstra a puestos base" << endl;
	
// 			// cout << sensores.energia << endl;
// 			if(sensores.superficie[0] == 'X') {
// 				// cout << "Recargando..." << endl;
// 				if(sensores.energia == MAX_ENERGIA) {
// 					// cout << "Voy a dejar de recargar..." << endl;
// 					recargar = false;
// 				}
				
// 				return IDLE;
// 			}
			
			
			
// 			// cout << sensores.energia << endl;
			
	
// 			// cout << "Voy a X" << endl;
	
// 			EstadoA estado;
// 			estado.f = sensores.posF;
// 			estado.c = sensores.posC;
// 			estado.brujula = sensores.rumbo;
// 			estado.zapatillas = tiene_zapatillas;
// 			if(hayPlanBases && HayQueReplanificar(sensores, planBases[indexBases], estado)) {
// 				// Borrar plan
// 				// cout << "replanifico..." << endl;
// 				hayPlanBases = false;
// 			}
	
// 			// cout << "No replanifico..." << endl;
	
	
// 			if (!hayPlanBases)
// 			{
// 				// Invocar al método de búsqueda
// 				// cout << "Buscando plan a X" << endl;
// 				indexBases = 0;
// 				planBases.clear();
// 				EstadoA inicio;
// 				inicio.f = sensores.posF;
// 				inicio.c = sensores.posC;
// 				inicio.brujula = sensores.rumbo;
// 				inicio.zapatillas = tiene_zapatillas;
// 				planBases = DijkstraPuestosBase(inicio, sensores);
// 				VisualizaPlan(inicio, planBases);
// 				PintaPlan(planBases, tiene_zapatillas);
// 				hayPlanBases = (planBases.size() != 0);
// 				// cout << "Plan a X encontrado" << endl;
// 			}
// 			if (hayPlanBases && indexBases < planBases.size())
// 			{
// 				// cout << "ejecutando plan" << endl;
// 				accion = planBases[indexBases];
// 				++indexBases;
// 			}
// 			if (indexBases == planBases.size())
// 			{
// 				// cout << "Plan a X terminado" << endl;
// 				hayPlanBases = false;
// 			}
// 			return accion;
// 		}
// 		else accion = ComportamientoAuxiliarNivel_1(sensores);
	
// 	}
// 	// Cuando llegue o el rescatador aborte misión, repito
// 	last_action = accion;

// 	cout << "Termine" << endl;

// 	return accion;
// }

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

bool ComportamientoAuxiliar::CasillaAccesibleAuxiliarOpciones(const EstadoA &st, int opcion)
{
	EstadoA next = NextCasillaAuxiliar(st);
	
	int dif = abs(mapaCotas[next.f][next.c] - mapaCotas[st.f][st.c]);
	char siguiente = mapaResultado[next.f][next.c];
	char e_siguiente = mapaEntidades[next.f][next.c];
	bool check1 = false, check2 = false, check3 = false, check4 = false;
	check1 = siguiente != 'P' && siguiente != 'M';
	check2 = siguiente != 'B' || (siguiente == 'B' && st.zapatillas);
	check3 = abs(mapaCotas[next.f][next.c] - mapaCotas[st.f][st.c]) <= 1;
	check4 = 3 <= next.f && next.f < mapaResultado.size() - 3 && 3 <= next.c && next.c < mapaResultado[0].size() - 3;
	bool check5 = (e_siguiente == '_' || e_siguiente == '?');

	bool check6 = siguiente != 'A' && siguiente != 'T';

	// cout << "check1: " << check1 << endl;
	// cout << "check2: " << check2 << endl;
	// cout << "check3: " << check3 << endl;
	// cout << "check4: " << check4 << endl;
	// cout << "check5: " << check5 << endl;
	// cout << "check6: " << check6 << endl;

	// cout << "Siguiente: " << (char)siguiente << endl;

	bool check = false;
	switch(opcion) {
		case 0:
			check = check1 && check2 && check3;
			break;
		case 1:
			check = check1 && check2 && (check3 || siguiente == '?' || mapaResultado[st.f][st.c] == '?') && check4 && check5;
			break;
		case 2:
			check = check1 && check2 && (check3 || siguiente == '?' || mapaResultado[st.f][st.c] == '?') && check4 && check5 && check6;
			break;
		default:
			break;
	}
	return check;
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
	int opcion = 0;
	if(sensores.nivel == 4) opcion = 1;
	switch (accion)
	{
	case WALK:
		if (CasillaAccesibleAuxiliarOpciones(st, opcion))
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
	if(casilla == '?') {
		casilla = desconocidas_busqueda;
	}
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

NodoA ComportamientoAuxiliar::A_Estrella(const EstadoA &inicio, const EstadoA &final, const Sensores & sensores) {
	NodoA current_node;
	min_priority_queue frontier;
	set<EstadoA> explored;
	NodoA solucion;

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
		solucion = current_node;

	return solucion;
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