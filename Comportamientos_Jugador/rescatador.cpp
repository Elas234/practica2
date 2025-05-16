#include "../Comportamientos_Jugador/rescatador.hpp"
#include "motorlib/util.h"
#include <iostream>
#include <assert.h>
#include <math.h>
using namespace std;

#define TURN_L_pendientes 0
#define TURN_SR_pendientes 1
#define WALK_pendientes 2
#define RUN_pendientes 3



const int INF = 1e6;

// Incremento/decremento de las coordenadas de la casilla justo en frente en función de la dirección
// Norte, noreste, este, sureste, sur, suroeste, oeste,noroeste
const int sf[8] = {-1, -1, 0,  1,  1,  1,  0, -1};
const int sc[8] = { 0,  1, 1,  1,  0, -1, -1, -1};

using min_priority_queue = priority_queue<NodoR, vector<NodoR>, greater<NodoR>>;


Action ComportamientoRescatador::think(Sensores sensores)
{
	Action accion = IDLE;

	switch (sensores.nivel)
	{
	case 0:
		accion = ComportamientoRescatadorNivel_0 (sensores);
		break;
	case 1:
		accion = ComportamientoRescatadorNivel_1 (sensores);
		break;
	case 2:
		accion = ComportamientoRescatadorNivel_2 (sensores);
		break;
	case 3:
		// accion = ComportamientoRescatadorNivel_3 (sensores);
		break;
	case 4:
		accion = ComportamientoRescatadorNivel_4 (sensores);
		break;
	}

	return accion;
}

int ComportamientoRescatador::interact(Action accion, int valor)
{
	return 0;
}

/*********************************** NIVEL 0 **********************************************/
pair<int, int> ComportamientoRescatador::VtoM(int i, Orientacion rumbo, const pair<int, int> & orig)
{
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

void ComportamientoRescatador::SituarSensorEnMapa(const Sensores & sensores) {
	
	int f = sensores.posF;
	int c = sensores.posC;
	pair<int,int> orig = {f, c};
	for(int i=0; i<16; ++i) {
		// Convierte la posición del sensor en la posición del mapa
		if(sensores.superficie[i] == 'X') veo_base = true;
		if(sensores.superficie[i] == 'D') conozco_zapatillas = true;
		pair<int,int> casilla = VtoM(i, sensores.rumbo, orig);
		int nf = casilla.first;
		int nc = casilla.second;
		mapaResultado[nf][nc] = sensores.superficie[i];
		mapaCotas[nf][nc] = sensores.cota[i];

		if(sensores.agentes[i] == 'a') {
			mapaEntidades[nf][nc] = 'a';
		}
	}	
}

bool ComportamientoRescatador::TengoTareasPendientes(const Sensores & sensores, Action & action) {

	const int NUM_ACCIONES = 4;
	const Action ACCIONES[NUM_ACCIONES] = {TURN_L, TURN_SR, WALK, RUN};

	for(int i=0; i<NUM_ACCIONES; ++i) {
		if(acciones_pendientes[i] != 0) {
			action = ACCIONES[i];
			acciones_pendientes[i]--;
			return true;
		}
	}
	return false;
}

bool ComportamientoRescatador::ViablePorAltura(int dif, bool zap) {
	return (abs(dif) <= 1 || (zap && abs(dif) <= 2));
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

void ComportamientoRescatador::CasillasInteresantesAllAround(const pair<int,int> & orig, const vector<bool> & accesible, 
	const vector<bool> & transitable, vector<bool> & is_interesting, vector<int> & casillas_interesantes, bool zap) {
	
	int f = orig.first;
	int c = orig.second;
	
	const int ALCANZABLES = 16;
	const int NUM_RELEVANT = 3;
	const char RELEVANT[NUM_RELEVANT] = {'X', 'D', 'C'};

	is_interesting.resize(ALCANZABLES, false);
	casillas_interesantes.clear();
	
	for(char x : RELEVANT) {
		for(int i=0; i<ALCANZABLES; ++i) {
			if(baneados[i]) continue;
			int level = 1+(i/8);
			int pos = i&7; // i%8
			int nf = f + level * sf[pos];
			int nc = c + level * sc[pos];
			if(mapaResultado[nf][nc] == x && accesible[i]) {
				if(level==2 && !transitable[pos]) continue;
				if(x=='X' || (x=='D' && !zap)) {
					if(x=='X' && mapaEntidades[nf][nc] == 'a') continue;
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

void ComportamientoRescatador::CasillasInteresantesAllAround_LVL1(const pair<int,int> & orig, const vector<bool> & accesible, 
	const vector<bool> & transitable, vector<bool> & is_interesting, vector<int> & casillas_interesantes, bool zap) {
	
	int f = orig.first;
	int c = orig.second;
	
	const int ALCANZABLES = 16;
	const int NUM_RELEVANT = 4;
	const char RELEVANT[NUM_RELEVANT] = {'X', 'D', 'C', 'S'};

	is_interesting.resize(ALCANZABLES, false);
	casillas_interesantes.clear();
	
	for(char x : RELEVANT) {
		for(int i=0; i<ALCANZABLES; ++i) {
			if(baneados[i]) continue;
			int level = 1+(i/8);
			int pos = i&7; // i%8
			int nf = f + level * sf[pos];
			int nc = c + level * sc[pos];
			if(mapaResultado[nf][nc] == x && accesible[i]) {
				if(level==2 && !transitable[pos]) continue;
				if(x=='D' && !zap) {
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


int ComportamientoRescatador::SelectCasilla (const Sensores & sensores, const vector<int> & casillas_interesantes, 
	const vector<bool> & is_interesting) {

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

	// cout << "Puntuaciones: " << endl;
	// for(int i=0; i<ALCANZABLES; ++i) {
	// 	cout << DESTINOS[i] << ": " << puntuacion_destino[i] << endl;
	// }
	// cout << "Ganador: " << decision << endl;

	return decision;
}

int ComportamientoRescatador::SelectCasillaAllAround (const pair<int,int> & orig, const vector<int> & casillas_interesantes, 
	const vector<bool> & is_interesting, Orientacion rumbo, bool zap) {

	const int ALCANZABLES = 16;
	const int NUM_RELEVANT = 4;
	int puntos_zapatillas = zap ? 50 : 10;

	const char RELEVANT[NUM_RELEVANT] = {'X', 'D', '?', 'C'};
	const int POINTS[NUM_RELEVANT] = {100, puntos_zapatillas, 10, 10};

	if(casillas_interesantes.size() == 0) return -1;
	if(casillas_interesantes.size() == 1) return casillas_interesantes[0];

	// Hay más de una, me voy por donde haya más y mejores casillas interesantes

	// Matriz de decisión

	// Cada casilla accesible tiene un valor asociado que depende de las 3 casillas que tiene enfrente

	const int CASILLAS_POR_SECCION = 3;
	const pair<int,int> M[ALCANZABLES/2][CASILLAS_POR_SECCION] = 
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

	int peso_altura = 1;
	int peso_frecuencia = 10;
	int peso_frecuencia_adyacentes = 5;

	int puntuacion_destino[ALCANZABLES] = {0};
	int puntuaciones_iniciales[ALCANZABLES] = 
	{100, 50, 10, 5, 1, 5 , 10, 50,
	 100, 50, 10, 5, 1, 5 , 10, 50};

	for(int i=0; i<ALCANZABLES; ++i) {
		if(!is_interesting[8*(i/8) + (rumbo+i)%8]) continue;
		puntuacion_destino[8*(i/8) + (rumbo+i)%8] = puntuaciones_iniciales[i];
	}

	int max_points = -INF;
	int decision = 0;
	for(int i=0; i<ALCANZABLES; ++i) {
		if(!is_interesting[i]) continue;
		int level = 1+(i/8);
		int pos = i&7; // i%8
		int f = orig.first + level * sf[pos];
		int c = orig.second + level * sc[pos];
		for(int j=0; j<CASILLAS_POR_SECCION; ++j) {
			int nf = f + M[pos][j].first;
			int nc = c + M[pos][j].second;
			char x = mapaResultado[nf][nc];
			int h = mapaCotas[nf][nc];
			char a = mapaEntidades[nf][nc];
			for(int k=0; k<NUM_RELEVANT; ++k) {
				if(x == RELEVANT[k]) {
					// No considero las casillas objetivo con agentes
					if(a == 'a') continue;
					// Puntuación inversamente proporcional a la diferencia de altura
					puntuacion_destino[i] += (POINTS[k] - peso_frecuencia_adyacentes*mapaFrecuencias[nf][nc] - peso_altura*abs(h-mapaCotas[f][c]));
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

int ComportamientoRescatador::SelectCasillaAllAround_LVL1(const pair<int,int> & orig, const vector<int> & casillas_interesantes, 
	const vector<bool> & is_interesting, Orientacion rumbo, bool zap) 
{
	// const int ALCANZABLES = 16;
	// const int NUM_RELEVANT = 5;
	// int puntos_zapatillas = zap ? 50 : 10;
	// const int LENGTH = 1;
	// const int WIDTH = 3;
	// int sup = WIDTH/2;
	// int inf = -sup;
	// const int K = sqrt(LENGTH*WIDTH);
	
	// const char RELEVANT[NUM_RELEVANT] = {'X', '?', 'D', 'C', 'S'};
	// const int POINTS[NUM_RELEVANT] = {10, 20, puntos_zapatillas, 10, 10};

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

	// for(int i=0; i<8; ++i) {
	// 	if(!is_interesting[(rumbo+i)%8]) continue;
	// 	puntuacion_destino[(rumbo+i)%8] = K*puntuaciones_iniciales[i];
	// }

	// const int df[8] = { 0, 1, 1, 0, 0,-1,-1, 0};
	// const int dc[8] = { 1, 0, 0,-1,-1, 0, 0, 1};

	// const int MAX_F = mapaResultado.size();
	// const int MAX_C = mapaResultado[0].size();

	// int max_points = -INF;
	// int decision = -1;
	// vector<vector<pair<int,int>>> casillas(8);
	// for(int pos=0; pos<8; ++pos) {
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
	// 		}
	// 	}

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
	// if(is_interesting[decision+8]) decision += 8;


	// cout << "Puntuaciones: " << endl;
	// for(int i=0; i<8; ++i) {
	// 	cout << i << ": " << puntuacion_destino[i] << endl;
	// }
	// cout << "Ganador: " << decision << endl;

	// return decision;

	const int ALCANZABLES = 16;
	const int NUM_RELEVANT = 5;
	int puntos_zapatillas = zap ? 50 : 10;

	const char RELEVANT[NUM_RELEVANT] = {'X', 'D', '?', 'C', 'S'};
	const int POINTS[NUM_RELEVANT] = {10, puntos_zapatillas, 10, 10, 10};

	if(casillas_interesantes.size() == 0) return -1;
	if(casillas_interesantes.size() == 1) return casillas_interesantes[0];

	// Hay más de una, me voy por donde haya más y mejores casillas interesantes

	// Matriz de decisión

	// Cada casilla accesible tiene un valor asociado que depende de las 3 casillas que tiene enfrente

	const int CASILLAS_POR_SECCION = 3;
	const pair<int,int> M[ALCANZABLES/2][CASILLAS_POR_SECCION] = 
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

	int peso_altura = 1;
	int peso_frecuencia = 10;
	int peso_frecuencia_adyacentes = 0;

	int puntuacion_destino[ALCANZABLES] = {0};
	int puntuaciones_iniciales[ALCANZABLES] = 
	{100, 50, 10, 5, 1, 5 , 10, 50,
	 100, 50, 10, 5, 1, 5 , 10, 50};

	for(int i=0; i<ALCANZABLES; ++i) {
		if(!is_interesting[8*(i/8) + (rumbo+i)%8]) continue;
		puntuacion_destino[8*(i/8) + (rumbo+i)%8] = puntuaciones_iniciales[i];
	}

	int max_points = -INF;
	int decision = 0;
	for(int i=0; i<ALCANZABLES; ++i) {
		if(!is_interesting[i]) continue;
		int level = 1+(i/8);
		int pos = i&7; // i%8
		int f = orig.first + level * sf[pos];
		int c = orig.second + level * sc[pos];
		for(int j=0; j<CASILLAS_POR_SECCION; ++j) {
			int nf = f + M[pos][j].first;
			int nc = c + M[pos][j].second;
			char x = mapaResultado[nf][nc];
			int h = mapaCotas[nf][nc];
			char a = mapaEntidades[nf][nc];
			for(int k=0; k<NUM_RELEVANT; ++k) {
				if(x == RELEVANT[k]) {
					// No considero las casillas objetivo con agentes
					if(a == 'a') continue;
					// Puntuación inversamente proporcional a la diferencia de altura
					puntuacion_destino[i] += (POINTS[k] - peso_frecuencia_adyacentes*mapaFrecuencias[nf][nc] - peso_altura*abs(h-mapaCotas[f][c]));
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

Action ComportamientoRescatador::SelectAction(int decision, Orientacion rumbo)
{
	// Decido la acción a realizar en función de la casilla a la que quiero ir
	Action action;
	if(decision == -1) {
		action = TURN_SR;
	}
	else {
		int level = 1+(decision/8);
		decision = (decision + 8 - rumbo) % 8;
		if(level==2) decision += 8;
		int pos = decision&7; // decision%8
		if(pos==0) {
			action = (level==1) ? WALK : RUN;
		}
		else {
			if(level == 1) acciones_pendientes[WALK_pendientes] = 1;
			else acciones_pendientes[RUN_pendientes] = 1;
			switch(pos) {
				case 1: 
				case 2: 
				case 3: 
					acciones_pendientes[TURN_SR_pendientes] = pos-1;
					action = TURN_SR;
					break;
				case 4: // 2 * TURN_L
				case 5: // 2 * TURN_L + TURN_SR
				case 6: // TURN_L
				case 7: // TURN_L + TURN_SR
					acciones_pendientes[TURN_L_pendientes] = (pos<6) ? 1 : 0;
					acciones_pendientes[TURN_SR_pendientes] = pos&1;
					action = TURN_L;
					break;
			}
		}
	}

	return action;
}

void GenerateLevelR(int level, vector<int> & filas, vector<int> & columnas) {
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

void ComportamientoRescatador::OndaDeCalor(int f, int c) {

	int calor = 128;
	const int MAX_LEVEL = 5;
	const int LEVELS = 4;
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

	// Si está atascado, resetea el mapa de frecuencias
	// const int NUM_ACCIONES_ATASCO = 2000;
	// if(num_acciones == NUM_ACCIONES_ATASCO) {
	// 	for(int i=0; i<mapaFrecuencias.size(); ++i) {
	// 		fill(mapaFrecuencias[i].begin(), mapaFrecuencias[i].end(), 0);
	// 	}
	// }

	int f = sensores.posF;
	int c = sensores.posC;

	SituarSensorEnMapa(sensores);
	if(last_action == WALK || last_action == RUN)
		OndaDeCalor(f,c);

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
	else if(!TengoTareasPendientes(sensores, action)) {
		// Casillas que puedo atravesar corriendo
		vector<bool> transitable(16);

		// Casillas a las que puedo acceder (accesible ==> transitable pero no al revés)
		vector<bool> accesible(16);

		for(int i=0; i<16; ++i) {

			int level = 1+(i/8);
			int pos = i&7; // i%8
			int nf = f + level * sf[pos];
			int nc = c + level * sc[pos];
			char s = mapaResultado[nf][nc];
			int h = mapaCotas[nf][nc];
			char e = mapaEntidades[nf][nc];

			transitable[i] = (s != 'P' && s != 'M' && s != 'B' && s != '?' && e != 'a' && !baneados[i]);
			accesible[i] = transitable[i] && ViablePorAltura(h-mapaCotas[f][c], tiene_zapatillas);
		}

		vector<int> casillas_interesantes;
		vector<bool> is_interesting(16, false);

		// CasillasInteresantes(sensores, accesible, transitable, is_interesting, casillas_interesantes, tiene_zapatillas);
		CasillasInteresantesAllAround({f,c}, accesible, transitable, is_interesting, casillas_interesantes, tiene_zapatillas);
		// int decision = SelectCasilla(sensores, casillas_interesantes, is_interesting);
		decision = SelectCasillaAllAround({f,c}, casillas_interesantes, is_interesting, sensores.rumbo, tiene_zapatillas);
		action = SelectAction(decision, sensores.rumbo);
	}

	if(num_acciones - contador == 10) {
		fill(baneados.begin(), baneados.end(), false);
	}

	/*-------------------------------------------------------------*/
	// Si me voy a chocar, doy media vuelta
	if((action == WALK && sensores.agentes[2] != '_') || (action == RUN && (sensores.agentes[6] != '_' || sensores.agentes[2] != '_'))) {
		// Resetea movimientos peligrosos
		// cout << "Rescatador: me voy a chocar, doy la vuelta" << endl;
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
		if(mapaResultado[nf][nc] != 'X' || mapaEntidades[nf][nc] != 'a') {
			mapaEntidades[nf][nc] = '?';
		}
	}



	last_action = action;
	++num_acciones;

	return action;
}


/**************************************** NIVEL 1 ***************************************************/
Action ComportamientoRescatador::ComportamientoRescatadorNivel_1(Sensores sensores)
{
	Action action;

	int f = sensores.posF;
	int c = sensores.posC;

	if(sensores.energia <= 10) {
		// Low battery
		return IDLE;
	}

	SituarSensorEnMapa(sensores);
	if(last_action == WALK || last_action == RUN) {
		OndaDeCalor(f,c);
	}

	if(sensores.superficie[0] == 'D') {
		tiene_zapatillas = true;
	}

	if(!TengoTareasPendientes(sensores, action)) {
		// Casillas que puedo atravesar corriendo
		vector<bool> transitable(16);

		// Casillas a las que puedo acceder (accesible ==> transitable pero no al revés)
		vector<bool> accesible(16);

		for(int i=0; i<16; ++i) {

			int level = 1+(i/8);
			int pos = i&7; // i%8
			int nf = f + level * sf[pos];
			int nc = c + level * sc[pos];
			char s = mapaResultado[nf][nc];
			int h = mapaCotas[nf][nc];
			char e = mapaEntidades[nf][nc];

			transitable[i] = (s != 'P' && s != 'M' && s != 'B' && s != '?' && e != 'a' && !baneados[i]);
			accesible[i] = transitable[i] && ViablePorAltura(h-mapaCotas[f][c], tiene_zapatillas);
		}
		vector<int> casillas_interesantes;
		vector<bool> is_interesting(16, false);
		CasillasInteresantesAllAround_LVL1({f,c}, accesible, transitable, is_interesting, casillas_interesantes, tiene_zapatillas);
		// int decision = SelectCasilla(sensores, casillas_interesantes, is_interesting);

		// cout << "Casillas interesantes: " << endl;
		// for(int i=0; i<casillas_interesantes.size(); ++i) {
		// 	cout << casillas_interesantes[i] << " ";
		// }
		// cout << endl;
		decision = SelectCasillaAllAround_LVL1({f,c}, casillas_interesantes, is_interesting, sensores.rumbo, tiene_zapatillas);
		action = SelectAction(decision, sensores.rumbo);
	}
	if(num_acciones - contador == 10) {
		fill(baneados.begin(), baneados.end(), false);
	}
	// Si me voy a chocar, doy media vuelta
	if((action == WALK && sensores.agentes[2] != '_') || (action == RUN && (sensores.agentes[6] != '_' || sensores.agentes[2] != '_'))) {
		// Resetea movimientos peligrosos
		// cout << "Rescatador: me voy a chocar, doy la vuelta" << endl;
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
		mapaEntidades[nf][nc] = '?';
	}
	last_action = action;
	++num_acciones;

	return action;
}

/**************************************** NIVEL 2 ***************************************************/

bool ComportamientoRescatador::IsSolution(const EstadoR &estado, const EstadoR &final)
{
	return estado.f == final.f && estado.c == final.c;
}

bool ComportamientoRescatador::CasillaAccesibleRescatador(const EstadoR &st, int impulso, int nivel)
{
	EstadoR next = NextCasillaRescatador(st, impulso);
	int dif = abs(mapaCotas[next.f][next.c] - mapaCotas[st.f][st.c]);
	char inter = mapaResultado[st.f+sf[st.brujula]][st.c+sc[st.brujula]];
	char siguiente = mapaResultado[next.f][next.c];
	bool check1 = false, check2 = false, check3 = false, check4 = false, check5 = false;
	check1 = siguiente != 'P' && siguiente != 'M' && siguiente != 'B';
	check2 = inter != 'P' && inter != 'M' && inter != 'B';
	check3 = (dif <= 1) || (st.zapatillas && (dif <= 2));
	check4 = 3 <= next.f && next.f < mapaResultado.size()-3 && 3 <= next.c && next.c < mapaResultado[0].size()-3;
	check5 = mapaEntidades[next.f][next.c] != 'a' && mapaEntidades[st.f+sf[st.brujula]][st.c+sc[st.brujula]] != 'a';
	if(nivel == 2)
		return check1 && check2 && check3;
	else {
		return check1 && check2 && (check3 || siguiente == '?' || mapaResultado[st.f][st.c] == '?') && check4 && check5;
	}
}

// Opciones:
// 0: Usado para Dijkstra en nivel 2, considera si puede ir a la casilla
// 1: Usado para A* en nivel 4, considera que puede pasar por casillas desconocidas
// 2: Usado para replanificar, no considera que puede pasar por casillas desconocidas ni por agua (evitar que pase por agua)
bool ComportamientoRescatador::CasillaAccesibleRescatadorOpciones(const EstadoR &st, int impulso, int opcion)
{
	EstadoR next = NextCasillaRescatador(st, impulso);
	
	int dif = abs(mapaCotas[next.f][next.c] - mapaCotas[st.f][st.c]);
	char inter = mapaResultado[st.f+sf[st.brujula]][st.c+sc[st.brujula]];
	char siguiente = mapaResultado[next.f][next.c];
	char e_siguiente = mapaEntidades[next.f][next.c];
	char e_inter = mapaEntidades[st.f+sf[st.brujula]][st.c+sc[st.brujula]];
	bool check1 = siguiente != 'P' && siguiente != 'M' && siguiente != 'B';
	bool check2 = inter != 'P' && inter != 'M' && inter != 'B';
	bool check3 = (dif <= 1) || (st.zapatillas && (dif <= 2));
	bool check4 = 3 <= next.f && next.f < mapaResultado.size()-3 && 3 <= next.c && next.c < mapaResultado[0].size()-3;
	bool check5 = (e_siguiente == '_' || e_siguiente == '?') && (e_inter == '_' || e_inter == '?');

	bool check6 = siguiente != 'A' && siguiente != 'T';

	// cout << "check1: " << check1 << endl;
	// cout << "check2: " << check2 << endl;
	// cout << "check3: " << check3 << endl;
	// cout << "check4: " << check4 << endl;
	// cout << "check5: " << check5 << endl;
	// cout << "check6: " << check6 << endl;

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


EstadoR ComportamientoRescatador::NextCasillaRescatador(const EstadoR &st, int impulso)
{
	EstadoR next = st;
	next.f += impulso*sf[st.brujula];
	next.c += impulso*sc[st.brujula];
	return next;
}

void ComportamientoRescatador::AnularMatrizA(vector<vector<unsigned char>> &m)
{
	for (unsigned int i = 0; i < m.size(); i++)
	{
		for (unsigned int j = 0; j < m[i].size(); j++)
		{
			m[i][j] = 0;
		}
	}
}

void ComportamientoRescatador::PintaPlan(const vector<Action> &plan, bool zap)
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

void ComportamientoRescatador::VisualizaPlan(const EstadoR &st, const vector<Action> &plan)
{
	AnularMatrizA(mapaConPlan);
	EstadoR cst = st;
	auto it = plan.begin();
	while (it != plan.end())
	{
		int impulso = 1;
		switch (*it)
		{
		case RUN:
			impulso = 2;
		case WALK:
			cst.f += impulso*sf[cst.brujula];
			cst.c += impulso*sc[cst.brujula];
			mapaConPlan[cst.f][cst.c] = 1;
			break;
		case TURN_SR:
			cst.brujula = (cst.brujula + 1) % 8;
			break;
		case TURN_L:
			cst.brujula = (cst.brujula + 6) % 8;
			break;
		}
		it++;
	}
}

EstadoR ComportamientoRescatador::applyR(Action accion, const EstadoR &st, bool &accesible, const Sensores & sensores)
{
	EstadoR next = st;
	int opcion = 0;
	if(sensores.nivel == 4) opcion = 1;
	int impulso = 1;
	switch (accion)
	{
	case RUN:
		impulso = 2;
	case WALK:
		if (CasillaAccesibleRescatadorOpciones(st, impulso, opcion)) {
			next = NextCasillaRescatador(st, impulso);
		}
		else {
			accesible = false;
		}
		break;
	case TURN_SR:
		next.brujula = (next.brujula + 1) % 8;
		break;
	case TURN_L:
		next.brujula = (next.brujula + 6) % 8;
		break;
	}
	return next;
}

// Calcular el mínimo número de giros necesarios para llegar al objetivo
int RefinamientoRescatador(const EstadoR & st, const EstadoR & final) {
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
	int inc = 0;
	int v_inc[8] = {0,1,2,3,2,3,1,2};
	
	inc = v_inc[(octante-st.brujula+8)%8];

	return inc+plus1;
}

int ComportamientoRescatador::Heuristica(const EstadoR &st, const EstadoR &final)
{
	if(st.f == final.f && st.c == final.c)
		return 0;
	// Heurística del máximo (divido entre 2 por RUN y tomo techo)
	int h = (max(abs(st.f - final.f), abs(st.c - final.c))+1)/2;

	// Mínimo número de giros necesarios
	int ref = RefinamientoRescatador(st, final);

	return h+ref;
}

int ComportamientoRescatador::CalcularCoste(Action accion, const EstadoR &st)
{
	int coste = 0;
	int impulso = 1;
	if(accion == RUN) impulso = 2;
	char casilla = mapaResultado[st.f][st.c];
	int h = mapaCotas[st.f][st.c];
	int rumbo = st.brujula;
	bool zap = st.zapatillas;
	int h1 = mapaCotas[st.f+impulso*sf[rumbo]][st.c+impulso*sc[rumbo]];
	int dif = 0;
	if(h1-h<0) dif =-1;
	if(h1-h>0) dif = 1;
	int inc = 0;
	switch (accion)
	{
		case WALK:
			switch(casilla)
			{
				// case '?':
				case 'A':
					coste = 100;
					inc = 10;
					break;
				case '?':
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
				// case '?':
				case 'A':
					coste = 16;
					break;
				case '?':
				case 'T':
					coste = 3;
					break;
				default:
					coste = 1;
					break;
			}
		break;
		case RUN:
			switch(casilla)
			{
				// case '?':
				case 'A':
					coste = 150;
					inc = 15;
					break;
				case 'T':
					coste = 35;
					inc = 5;
					break;
				case '?':
				case 'S':
					coste = 3;
					inc = 2;
					break;
				// case '?':
				// 	coste = 10;
				// 	inc = 3;
				default:
					coste = 1;
					inc = 0;
					break;
			}
			coste += (dif*inc);
		break;
		case TURN_L:
			switch(casilla)
			{
				// case '?':
				case 'A':
					coste = 30;
					break;
				case '?':
				case 'T':
					coste = 5;
					break;
				default:
					coste = 1;
					break;
			}
		break;
	}

	return coste;
}

vector<Action> ComportamientoRescatador::Dijkstra(const EstadoR &inicio, const EstadoR &final, const Sensores & sensores) {
	NodoR current_node;
	min_priority_queue frontier;
	set<EstadoR> explored;
	vector<Action> path;

	current_node.estado = inicio; // Asigna el estado inicial al nodo actual
	current_node.g = 0;
	// current_node.h = Heuristica(current_node.estado, final);

	frontier.push(current_node);

	bool SolutionFound = IsSolution(current_node.estado, final);

	while (!SolutionFound && !frontier.empty())
	{
		// cout << "Explorando nodo: " << current_node.estado.f << ", " << current_node.estado.c << endl;
		frontier.pop();

		if (mapaResultado[current_node.estado.f][current_node.estado.c] == 'D')
		{
			current_node.estado.zapatillas = true;
		}
		explored.insert(current_node.estado);


		SolutionFound = IsSolution(current_node.estado, final);
		if(SolutionFound) break;

		Action acciones[4] = {RUN, WALK, TURN_L, TURN_SR};
		for(Action accion : acciones) {
			NodoR child;
			child.estado = current_node.estado;
			child.secuencia = current_node.secuencia;

			bool accesible = true;
			child.estado = applyR(accion, current_node.estado, accesible, sensores);
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
		SolutionFound = IsSolution(current_node.estado, final);
	}

	if (SolutionFound)
		path = current_node.secuencia;

	return path;
}

bool ComportamientoRescatador::EncontreCasilla(const EstadoR &st, vector<char> casillas_objetivo) {
	for (char c : casillas_objetivo) {
		if (mapaResultado[st.f][st.c] == c) {
			return true;
		}
	}
	return false;
}

NodoR ComportamientoRescatador::BuscaCasillas(const EstadoR &inicio, const Sensores & sensores, const vector<char>& casillas_objetivo) {
	NodoR current_node;
	min_priority_queue frontier;
	set<EstadoR> explored;
	NodoR solucion;

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

		Action acciones[4] = {RUN, WALK, TURN_L, TURN_SR};
		for(Action accion : acciones) {
			NodoR child;
			child.estado = current_node.estado;
			
			child.secuencia = current_node.secuencia;

			bool accesible = true;
			child.estado = applyR(accion, current_node.estado, accesible, sensores);
			if(!accesible) continue;
			// if (mapaResultado[child.estado.f][child.estado.c] == '?') continue;
			int coste = CalcularCoste(accion, current_node.estado);
			// Las considero como agua por si acaso
			if (mapaResultado[child.estado.f][child.estado.c] == '?') {
				switch(accion) {
					case RUN:
						coste = 165;
						break;
					case WALK:
						coste = 110;
						break;
					case TURN_SR:
						coste = 16;
						break;
					case TURN_L:
						coste = 30;
						break;
				}
			}
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

NodoR ComportamientoRescatador::A_estrella(const EstadoR &inicio, const EstadoR &final, const Sensores & sensores) {
	NodoR current_node;
	min_priority_queue frontier;
	set<EstadoR> explored;
	NodoR solucion;

	current_node.estado = inicio; // Asigna el estado inicial al nodo actual
	current_node.g = 0;
	current_node.h = Heuristica(current_node.estado, final);

	frontier.push(current_node);

	bool SolutionFound = IsSolution(current_node.estado, final);

	while (!SolutionFound && !frontier.empty())
	{
		// cout << "Explorando nodo: " << current_node.estado.f << ", " << current_node.estado.c << endl;
		frontier.pop();

		if (mapaResultado[current_node.estado.f][current_node.estado.c] == 'D')
		{
			current_node.estado.zapatillas = true;
		}
		explored.insert(current_node.estado);


		SolutionFound = IsSolution(current_node.estado, final);
		if(SolutionFound) break;

		Action acciones[4] = {RUN, WALK, TURN_L, TURN_SR};
		for(Action accion : acciones) {
			NodoR child;
			child.estado = current_node.estado;
			child.secuencia = current_node.secuencia;

			bool accesible = true;
			child.estado = applyR(accion, current_node.estado, accesible, sensores);
			if(!accesible) continue;
			int coste = CalcularCoste(accion, current_node.estado);
			char casilla = mapaResultado[child.estado.f][child.estado.c];
			// cout << (char)casilla << endl;
			if(opcion_busqueda == NO_PERMISIVA && (casilla == 'A' || casilla == 'T')) {
				if(max(abs(child.estado.f - final.f), abs(child.estado.c - final.c)) > RADIO_INMERSION) {
					continue;
				}
				// continue;
			}
			child.g = current_node.g + coste;
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
				// cout << frontier.size() << endl;
				frontier.pop();
				current_node = frontier.top();
			}
		}
		SolutionFound = IsSolution(current_node.estado, final);
		plan_inseguro = mapaResultado[current_node.estado.f][current_node.estado.c] == '?';
	}

	if (SolutionFound)
		solucion = current_node;

	return solucion;
}

Action ComportamientoRescatador::ComportamientoRescatadorNivel_2(Sensores sensores)
{
	Action accion = IDLE;
	if (!hayPlan)
	{
		// Invocar al método de búsqueda
		index = 0;
		plan.clear();
		
		EstadoR inicio, fin;
		inicio.f = sensores.posF;
		inicio.c = sensores.posC;
		inicio.brujula = sensores.rumbo;
		fin.f = sensores.destinoF;
		fin.c = sensores.destinoC;
		
		inicio.zapatillas = false;
		plan = Dijkstra(inicio, fin, sensores);

		VisualizaPlan(inicio, plan);
		PintaPlan(plan, tiene_zapatillas);
		// cout << plan.size() << endl;
		hayPlan = (plan.size() != 0);
	}
	if (hayPlan && index < plan.size())
	{
		accion = plan[index];
		++index;
	}
	if (index == plan.size())
	{
		index = 0;
		hayPlan = false;
	}
	return accion;
}

void UnionVectores(const vector<Action> &v1, const vector<Action> &v2, vector<Action> &res)
{
	res.clear();
	res.insert(res.end(), v1.begin(), v1.end());
	res.insert(res.end(), v2.begin(), v2.end());
}

pair<int,int> ComportamientoRescatador::CasillaMasFavorable(int f, int c)
{
	pair<int,int> casilla_mas_cercana = {-1,-1};

	const int MAX_LEVEL = max(mapaResultado.size(), mapaResultado[0].size());
	for (int level = 0; level <= MAX_LEVEL; ++level) {
		vector<int> filas, columnas;
		GenerateLevelR(level, filas, columnas);

		for (int i = 0; i < filas.size(); ++i) {
			int nf = f + filas[i];
			int nc = c + columnas[i];

			if (nf >= 0 && nf < mapaResultado.size() && nc >= 0 && nc < mapaResultado[0].size()) {
				if (mapaResultado[nf][nc] == 'C' && !baneadas[nf][nc]) {
					casilla_mas_cercana = {nf, nc};
				}
			}
		}

		// Si ya encontramos una casilla favorable, podemos detener la búsqueda
		if (casilla_mas_cercana.first != -1 && casilla_mas_cercana.second != -1) {
			break;
		}
	}

	return casilla_mas_cercana;
}

void ComportamientoRescatador::DecideOpcionReplanificacion(const Sensores & sensores)
{
	
	bool estoy_cerca = max(abs(sensores.posF - sensores.destinoF), abs(sensores.posC - sensores.destinoC)) <= RADIO_INMERSION+2;

	// cout << "Estoy cerca: " << boolalpha << estoy_cerca << endl;
	// cout << "Camino duro: " << boolalpha << camino_duro << endl;
	
	if ((sensores.superficie[0] == 'A' || sensores.superficie[0] == 'T') || estoy_cerca || camino_duro) {
		opcion_replanificacion = PERMISIVA;
		opcion_busqueda = PERMISIVA;
	}
	// else if(opcion_busqueda == PERMISIVA) {
	// 	opcion_replanificacion = PERMISIVA;
	// }
	else {
		opcion_replanificacion = NO_PERMISIVA;
		opcion_busqueda = NO_PERMISIVA;
	}


}

Action ComportamientoRescatador::ComportamientoRescatadorNivel_3(Sensores sensores)
{
	// Action accion = IDLE;
	// if (!hayPlan)
	// {
	// 	EstadoR inicio, fin;
	// 	inicio.f = sensores.posF;
	// 	inicio.c = sensores.posC;
	// 	inicio.brujula = sensores.rumbo;
	// 	fin.f = sensores.destinoF;
	// 	fin.c = sensores.destinoC;

	// 	inicio.zapatillas = false;
	// 	plan = A_Estrella(inicio, fin);
	// 	VisualizaPlan(inicio, plan);
	// 	PintaPlan(plan, tiene_zapatillas);
	// 	hayPlan = (plan.size() != 0);
	// }
	// if (hayPlan && plan.size() > 0)
	// {
	// 	accion = plan.front();
	// 	plan.pop_front();
	// }
	// if (plan.size() == 0)
	// {
	// 	hayPlan = false;
	// }
	// return accion;
}

bool ComportamientoRescatador::HayQueReplanificar(const Sensores & sensores, const Action & accion) {
	
	DecideOpcionReplanificacion(sensores);

	if(opcion_busqueda == NO_PERMISIVA) {
		cout << "Opcion busqueda: NO_PERMISIVA" << endl;
	}
	else {
		cout << "Opcion busqueda: PERMISIVA" << endl;
	}
	if(opcion_replanificacion == NO_PERMISIVA) {
		cout << "Opcion replanificacion: NO_PERMISIVA" << endl;
	}
	else {
		cout << "Opcion replanificacion: PERMISIVA" << endl;
	}

	EstadoR estado = {sensores.posF, sensores.posC, sensores.rumbo, tiene_zapatillas};
	if(sensores.choque) return true;
	if(accion != WALK && accion != RUN) return false;
	int impulso = (accion == RUN) ? 2 : 1;

	// cout << (char)(mapaResultado[next.f][next.c]) << endl;
	if(!CasillaAccesibleRescatadorOpciones(estado, impulso, opcion_replanificacion)) {
		return true;
	}

	return false;
}


int ComportamientoRescatador::SelectCasillaAllAround_LVL4(Sensores sensores, const vector<int> & casillas_interesantes, 
	const vector<bool> & is_interesting) 
{
	bool zap = tiene_zapatillas;
	int rumbo = sensores.rumbo;
	pair<int,int> orig = {sensores.posF, sensores.posC};
	pair<int,int> dest = {sensores.destinoF, sensores.destinoC};

	const int ALCANZABLES = 16;
	const int NUM_RELEVANT = 5;
	int puntos_zapatillas = zap ? 50 : 10;

	const int PESO_DESTINO = 100;

	const char RELEVANT[NUM_RELEVANT] = {'X', 'D', '?', 'C', 'S'};
	const int POINTS[NUM_RELEVANT] = {10, puntos_zapatillas, 10, 10, 10};

	if(casillas_interesantes.size() == 0) return -1;
	if(casillas_interesantes.size() == 1) return casillas_interesantes[0];

	// Hay más de una, me voy por donde haya más y mejores casillas interesantes

	// Matriz de decisión

	// Cada casilla accesible tiene un valor asociado que depende de las 3 casillas que tiene enfrente

	const int CASILLAS_POR_SECCION = 3;
	const pair<int,int> M[ALCANZABLES/2][CASILLAS_POR_SECCION] = 
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

	int peso_altura = 1;
	int peso_frecuencia = 10;
	int peso_frecuencia_adyacentes = 0;

	int puntuacion_destino[ALCANZABLES] = {0};
	int puntuaciones_iniciales[ALCANZABLES] = 
	{100, 50, 10, 5, 1, 5 , 10, 50,
	 100, 50, 10, 5, 1, 5 , 10, 50};

	for(int i=0; i<ALCANZABLES; ++i) {
		if(!is_interesting[8*(i/8) + (rumbo+i)%8]) continue;
		puntuacion_destino[8*(i/8) + (rumbo+i)%8] = puntuaciones_iniciales[i];
	}

	int max_points = -INF;
	int decision = 0;
	for(int i=0; i<ALCANZABLES; ++i) {
		if(!is_interesting[i]) continue;
		int level = 1+(i/8);
		int pos = i&7; // i%8
		int f = orig.first + level * sf[pos];
		int c = orig.second + level * sc[pos];
		for(int j=0; j<CASILLAS_POR_SECCION; ++j) {
			int nf = f + M[pos][j].first;
			int nc = c + M[pos][j].second;
			char x = mapaResultado[nf][nc];
			int h = mapaCotas[nf][nc];
			char a = mapaEntidades[nf][nc];
			for(int k=0; k<NUM_RELEVANT; ++k) {
				if(x == RELEVANT[k]) {
					// No considero las casillas objetivo con agentes
					if(a == 'a') continue;
					// Puntuación inversamente proporcional a la diferencia de altura
					puntuacion_destino[i] += (POINTS[k] - peso_frecuencia_adyacentes*mapaFrecuencias[nf][nc] - peso_altura*abs(h-mapaCotas[f][c]));
					puntuacion_destino[i] -= PESO_DESTINO*max(abs(dest.first - nf), abs(dest.second - nc));
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

Action ComportamientoRescatador::Exploracion(Sensores sensores) {
	Action action;

	int f = sensores.posF;
	int c = sensores.posC;

	if(sensores.energia <= 10) {
		// Low battery
		return IDLE;
	}

	SituarSensorEnMapa(sensores);
	if(last_action == WALK || last_action == RUN) {
		OndaDeCalor(f,c);
	}

	if(sensores.superficie[0] == 'D') {
		tiene_zapatillas = true;
	}

	if(!TengoTareasPendientes(sensores, action)) {
		// Casillas que puedo atravesar corriendo
		vector<bool> transitable(16);

		// Casillas a las que puedo acceder (accesible ==> transitable pero no al revés)
		vector<bool> accesible(16);

		for(int i=0; i<16; ++i) {

			int level = 1+(i/8);
			int pos = i&7; // i%8
			int nf = f + level * sf[pos];
			int nc = c + level * sc[pos];
			char s = mapaResultado[nf][nc];
			int h = mapaCotas[nf][nc];
			char e = mapaEntidades[nf][nc];

			transitable[i] = (s != 'P' && s != 'M' && s != 'B' && s != '?' && e != 'a' && !baneados[i]);
			accesible[i] = transitable[i] && ViablePorAltura(h-mapaCotas[f][c], tiene_zapatillas);
		}
		vector<int> casillas_interesantes;
		vector<bool> is_interesting(16, false);
		CasillasInteresantesAllAround_LVL1({f,c}, accesible, transitable, is_interesting, casillas_interesantes, tiene_zapatillas);
		// int decision = SelectCasilla(sensores, casillas_interesantes, is_interesting);

		// cout << "Casillas interesantes: " << endl;
		// for(int i=0; i<casillas_interesantes.size(); ++i) {
		// 	cout << casillas_interesantes[i] << " ";
		// }
		// cout << endl;
		decision = SelectCasillaAllAround_LVL4(sensores, casillas_interesantes, is_interesting);
		action = SelectAction(decision, sensores.rumbo);
	}
	if(num_acciones - contador == 10) {
		fill(baneados.begin(), baneados.end(), false);
	}
	// Si me voy a chocar, doy media vuelta
	if((action == WALK && sensores.agentes[2] != '_') || (action == RUN && (sensores.agentes[6] != '_' || sensores.agentes[2] != '_'))) {
		// Resetea movimientos peligrosos
		// cout << "Rescatador: me voy a chocar, doy la vuelta" << endl;
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
		mapaEntidades[nf][nc] = '?';
	}
	last_action = action;
	++num_acciones;

	return action;
}

//TODO : A* para rescatador en lugar de Dijkstra DONE
//TODO : A* para puestos base (multidirigido) en lugar de reactivo DONE
//TODO : mejorar comportamiento nivel 1 para auxiliar DONE
//TODO : maybe cambiar condición de redirección para evitar cruzar mucha agua DONE
//TODO : recargar energía al máximo DONE
//TODO : intentar crear onda de calor solo una vez al desplazarse NOT DONE

//TODO : devolver coste en A* DONE
//TODO : comprobar si puedo llegar a objetivo y después ir a puesto base, si no, recargar DONE
//!Problema: puede ser que no pueda ni con el máximo de energía, revisar eso más tarde, habría que ponerse en modo exploración


//! Errores:
//! Core dump al replanificar después de recargar (solo da core el rescatador) SOLVED (?)
//! Hay un test case oculto en el que se muere el rescatador y otro en el que muere el auxiliar

//? Teorías: 
//? En el A* hay un momento en el que se puede acceder a una cola vacía, esto solo ocurre
//? si el A* no puede alcanzar el destino, ya que se acaban los nodos por explorar

//? Por tanto lo más probable es que la razón del core sea que el A* no puede llegar al objetivo después
//? de recargar, tan solo habría que ver por qué no puede. Antes dio core porque consideraba las '?' como
//? no transitables y no podía llegar al objetivo, así que habría que imprimir que nodos se exploran para ver cuántos
//? se exploran y ver cuál es el último.

//! El auxiliar salta al vacío y no sé por qué

//? Unknown

//! Si hay puestos bases pero no son accesibles, el rescatador se queda parado pensando

//? Comprobar si hay plan en la primera iteración y si no hay, comportamiento reactivo

// RESUMEN COMPORTAMIENTO RESCATADOR:

// 1. Si tengo poca energía, busco un puesto base, ya sea de forma reactiva o deliberativa
// 2. Si tengo energía para ir a un objetivo y después a un puesto base, busco un objetivo y solo voy si estoy casi seguro de que puedo llegar y volver
// 3. Si no estoy seguro, tengo que explorar los alrededores del objetivo, como podría ser una zona costosa, voy al camino más cercano
// 4. Si llego desde ahí, perfecto, si no, tengo que explorar más, debería hacer un comportamiento nivel 0 pero con cierta preferencia por el objetivo
// 5. A la vez que exploro, voy comprobando si puedo ir al objetivo y volver, tengo que buscar un equilibrio, pues comprobarlo en cada iteración es costoso
//   	y no debería volver a ir al camino más cercano, pues la probabilidad de haberlo explorado es alta

/*
Action ComportamientoRescatador::ComportamientoRescatadorNivel_4(Sensores sensores)
{
	Action accion = IDLE;

	const int UMBRAL_TIEMPO = 0*mapaResultado.size();
	const int UMBRAL_ENERGIA = 10*mapaResultado.size();
	const int MAX_ENERGIA = 3000;
	const int ACCIONES_EXPLORACION = 100;


	// TODO: Esto se hace varias veces si se llama a nivel 0 o 1 (optimizar)
	if(last_action == RUN || last_action == WALK) {
		OndaDeCalor(sensores.posF, sensores.posC);
	}

	SituarSensorEnMapa(sensores);

	if(sensores.superficie[0] == 'D') {
		tiene_zapatillas = true;
	}

	if(sensores.superficie[0] != 'X') {
		recargar = sensores.energia <= UMBRAL_ENERGIA;
	}

	
	cout << "Recargar: " << boolalpha << recargar << endl;
	/ **********************************************************************************************
	 * ? RECARGAR ENERGÍA 
	/********************************************************************************************* /

	if(recargar) {
		// Dijkstra a puestos base
		hayPlan = false;
		plan.clear();
		plan_inseguro = false;
		index = 0;

		cout << sensores.energia << endl;
		if(sensores.superficie[0] == 'X') {
			cout << "Recargando..." << endl;
			if(sensores.energia == MAX_ENERGIA) {
				cout << "Voy a dejar de recargar..." << endl;
				recargar = false;
			}
			
			return IDLE;
		}
		
		
		
		cout << sensores.energia << endl;
		

		cout << "Voy a X" << endl;

		EstadoR estado;
		estado.f = sensores.posF;
		estado.c = sensores.posC;
		estado.brujula = sensores.rumbo;
		estado.zapatillas = tiene_zapatillas;
		if(hayPlan && HayQueReplanificar(sensores, plan[index], estado)) {
			// Borrar plan
			cout << "replanifico..." << endl;
			hayPlan = false;
		}

		cout << "No replanifico..." << endl;

		if (!hayPlan)
		{
			// Invocar al método de búsqueda
			cout << "Buscando plan a X" << endl;
			index = 0;
			plan.clear();
			plan_inseguro = false;
			NodoR nodo_base;
			pair<vector<Action>,int> path_coste;
			EstadoR inicio;
			inicio.f = sensores.posF;
			inicio.c = sensores.posC;
			inicio.brujula = sensores.rumbo;
			inicio.zapatillas = tiene_zapatillas;
			cout << "Conozco bases? " << boolalpha << conozco_bases << endl;
			if(conozco_bases) {
				nodo_base = BuscaCasillas(inicio, sensores, {'X'});
				plan_inseguro = nodo_base.path_inseguro;
				// Compruebo si llego (si no estoy muy seguro, no me arriesgo)
				path_coste = {nodo_base.secuencia, nodo_base.g};
				bool imposible = nodo_base.secuencia.size() == 0;
				bool llego = (plan_inseguro && path_coste.second < 0.7*sensores.energia) || path_coste.second < sensores.energia;
				if(!imposible && llego) 
					plan = path_coste.first;
				else 
					conozco_bases = false;
				VisualizaPlan(inicio, plan);
				PintaPlan(plan, tiene_zapatillas);
				hayPlan = (plan.size() != 0);
			}		
			if(!hayPlan) {
				cout << "No hay plan a X" << endl;
				if(sensores.superficie[0] == 'C' || sensores.superficie[0] == 'D') {
					return ComportamientoRescatadorNivel_0(sensores);
				}
				else {
					nodo_base = BuscaCasillas(inicio, sensores, {'C', 'D'});
					plan_inseguro = nodo_base.path_inseguro;
					// Compruebo si llego (si no estoy muy seguro, no me arriesgo)
					path_coste = {nodo_base.secuencia, nodo_base.g};
					bool imposible = nodo_base.secuencia.size() == 0;
					bool llego = (plan_inseguro && path_coste.second < 0.7*sensores.energia) || path_coste.second < sensores.energia;
					if(!imposible && llego) 
						plan = path_coste.first;
					PintaPlan(plan, tiene_zapatillas);
					hayPlan = (plan.size() != 0);
					if(!hayPlan) {
						// No hay caminos visibles, último recurso: dar vueltas
						if(CasillaAccesibleRescatadorOpciones(inicio, 1, 1)) {
							accion = WALK;
						}
						else if(CasillaAccesibleRescatadorOpciones(inicio, 2, 1)) {
							accion = RUN;
						}
						else {
							accion = TURN_SR;
						}
						return accion;
					}
				}
			}
			cout << "Plan a X encontrado" << endl;
		}
		if (hayPlan && index < plan.size())
		{
			cout << "ejecutando plan" << endl;
			accion = plan[index];
			++index;
		}
		if (index == plan.size())
		{
			cout << "Plan a X terminado" << endl;
			hayPlan = false;
		}
		return accion;
	}

	/ **********************************************************************************************
	 * ? EXPLORAR UN POCO DE MAPA AL INICIO
	 ********************************************************************************************** /

	 if(num_acciones < ACCIONES_EXPLORACION) {
		if(sensores.superficie[0] == 'C' || sensores.superficie[0] == 'S' || sensores.superficie[0] == 'D' || sensores.superficie[0] == 'X') 
			return Exploracion(sensores);
		else {
			index = 0;
			plan.clear();
			plan_inseguro = false;
			NodoR nodo_base;
			pair<vector<Action>,int> path_coste;
			EstadoR inicio;
			inicio.f = sensores.posF;
			inicio.c = sensores.posC;
			inicio.brujula = sensores.rumbo;
			inicio.zapatillas = tiene_zapatillas;
			if(conozco_bases) {
				nodo_base = BuscaCasillas(inicio, sensores, {'C','D','X','S'});
				plan_inseguro = nodo_base.path_inseguro;
				// Compruebo si llego (si no estoy muy seguro, no me arriesgo)
				path_coste = {nodo_base.secuencia, nodo_base.g};
				bool imposible = nodo_base.secuencia.size() == 0;
				bool llego = (plan_inseguro && path_coste.second < 0.7*sensores.energia) || path_coste.second < sensores.energia;
				if(!imposible && llego) 
					plan = path_coste.first;
				VisualizaPlan(inicio, plan);
				PintaPlan(plan, tiene_zapatillas);
				hayPlan = (plan.size() != 0);
				if(imposible) conozco_bases = false;
			}
			if(!hayPlan) {
				// No hay caminos visibles, último recurso: dar vueltas
				if(CasillaAccesibleRescatadorOpciones(inicio, 1, 1)) {
					accion = WALK;
				}
				else if(CasillaAccesibleRescatadorOpciones(inicio, 2, 1)) {
					accion = RUN;
				}
				else {
					accion = TURN_SR;
				}
				return accion;
			}

		}
	}

	// cout << "Doy vueltas hasta cansarme" << endl;

	// return TURN_SR;

	
	/ **********************************************************************************************
	 * ? LLAMAR A AUXILIAR
	 ********************************************************************************************** /

	if(sensores.posF == sensores.destinoF && sensores.posC == sensores.destinoC) {
		// Llamar al auxiliar
		// cout << "Tiempo de espera: " << tiempo_espera << endl;
		// cout << "Gravedad: " << sensores.gravedad << endl;
		if(tiempo_espera == 0 && sensores.gravedad == 1) {
			// Acabo de llegar
			// cout << "Llamando al auxiliar" << endl;
			
			accion = CALL_ON;
			tiempo_espera++;
		}
		
		// Si tarda mucho, abortar misión
		else if(tiempo_espera > UMBRAL_TIEMPO || sensores.gravedad == 0) {
			// cout << "Abortando misión" << endl;
			accion = CALL_OFF;
			tiempo_espera = 0;
		}
		// Esperar
		else {
			accion = IDLE;
			tiempo_espera++;
		}
		if(objetivo_anterior.first != sensores.destinoF || objetivo_anterior.second != sensores.destinoC) {
			tiempo_espera = 0;
			objetivo_anterior = {sensores.destinoF, sensores.destinoC};
		}
	}
	
	/ **********************************************************************************************
	 * ? IR A OBJETIVO
	 ********************************************************************************************** /

	else {
		EstadoR estado;
		estado.f = sensores.posF;
		estado.c = sensores.posC;
		estado.brujula = sensores.rumbo;
		estado.zapatillas = tiene_zapatillas;

		// Si me choco o me voy a caer, recalculo el plan 
		cout << "Tengo plan? " << boolalpha << hayPlan << endl;
		cout << "Tengo que replanificar? " << boolalpha << HayQueReplanificar(sensores, accion, estado) << endl;
		if(hayPlan && HayQueReplanificar(sensores, plan[index], estado)) {
			hayPlan = false;
			index = 0;
			plan.clear();
			plan_inseguro = false;
		}
		cout << "Busco objetivo" << endl;
		accion = BuscaObjetivo(sensores);
		cout << "Encontré objetivo?" << boolalpha << hayPlan << endl;
		//! ComportamientoReactivo

		//? Esto podría ser un problema, ya que podría alejarse del objetivo
		//? Solución: A* a la casilla conocida más cercana y si desde ahí no se puede, reactivo

		//! Esto está cogido de los pelos, pero funciona bien y podría usarse como comportamiento principal
		//? Si no tengo plan y no puedo llegar al objetivo, busco la casilla conocida más cercana
		if(!hayPlan) {
			pair<int,int> casilla_mas_cercana = CasillaMasFavorable(sensores.destinoF, sensores.destinoC);
			int fil = casilla_mas_cercana.first;
			int col = casilla_mas_cercana.second;
			EstadoR fin;
			fin.f = fil;
			fin.c = col;
			NodoR node = A_estrella(estado, fin, sensores);
			plan_inseguro = node.path_inseguro;
			// Compruebo si llego (si no estoy muy seguro, no me arriesgo)
			bool imposible = node.secuencia.size() == 0;
			bool llego = (plan_inseguro && node.g < 0.7*sensores.energia) || node.g < sensores.energia;
			if(!imposible && llego) {
				plan = node.secuencia;

			}
			VisualizaPlan(estado, plan);
			PintaPlan(plan, tiene_zapatillas);
			hayPlan = (plan.size() != 0);
			if(!hayPlan) {
				// Exploración durante MAX_EXPLORACIÓN INSTANTES
				num_acciones = 0;
				baneadas[fil][col] = true;
			}
		}
	}
	cout << "uwu3" << endl;

	// Repeat
	// if(accion == CALL_ON) {
	// 	cout << "TE ESTOY LLAMANDO 100% !!!" << endl;
	// }

	return accion;
}
*/

// pre : hayPlan == false || ejecutando_plan_objetivo
Action ComportamientoRescatador::PlanRecarga(Sensores sensores) {

	Action accion = IDLE;

	if(ejecutando_plan_objetivo) {
		AnularPlan();
		ejecutando_plan_objetivo = false;
	}

	// cout << sensores.energia << endl;
	// if(sensores.superficie[0] == 'X') {
	// 	cout << "Recargando..." << endl;
	// 	// if(sensores.energia == UMBRAL_ENERGIA) {
	// 	// 	cout << "Voy a dejar de recargar..." << endl;
	// 	// 	necesito_recargar = recargando = false;
	// 	// }
	// 	recargando = false;
		
	// 	return IDLE;
	// }
	
	
	
	// cout << sensores.energia << endl;
	
	// if(hayPlan && HayQueReplanificar(sensores, plan[index])) {
	// // Borrar plan
	// 	cout << "replanifico..." << endl;
	// 	hayPlan = false;
	// }
	// cout << "No replanifico..." << endl;

	// Invocar al método de búsqueda
	EstadoR estado;
	NodoR nodo_base;
	estado.f = sensores.posF;
	estado.c = sensores.posC;
	estado.brujula = sensores.rumbo;
	estado.zapatillas = tiene_zapatillas;
	vector<char> desconocidas = {'T', 'S', 'C'};
	for(int i=0; i<2; ++i) {
		for(char casilla_desconocida : desconocidas) {

			desconocidas_busqueda = casilla_desconocida;
			nodo_base = BuscaCasillas(estado, sensores, {'X'});
			camino_duro = CaminoDuro(estado, {INF,INF}, nodo_base.secuencia);
			if(camino_duro) {
				opcion_replanificacion = PERMISIVA;
				// cout << "Camino duro" << endl;
			}
			if(nodo_base.g <= sensores.energia) {
				plan = nodo_base.secuencia;
			}
			else {
				cout << "ERROR" << endl;
			}
			VisualizaPlan(estado, plan);
			// PintaPlan(plan, tiene_zapatillas);
			hayPlan = (plan.size() != 0);
		}
	}
	
	// Debería de haber un plan
	// if(!hayPlan) {
	// 	conozco_bases = false;
	// 	cout << "No hay plan a X" << endl;
	// 	if(sensores.superficie[0] == 'C' || sensores.superficie[0] == 'D') {
	// 		return ComportamientoRescatadorNivel_0(sensores);
	// 	}
	// 	else {
	// 		nodo_base = BuscaCasillas(estado, sensores, {'C','D','S'});
	// 		if(nodo_base.g <= sensores.energia)
	// 			plan = nodo_base.secuencia;
	// 		VisualizaPlan(estado, plan);
	// 		PintaPlan(plan, tiene_zapatillas);
	// 		hayPlan = (plan.size() != 0);
	// 		if(!hayPlan) {
	// 			// No hay caminos visibles, último recurso: dar vueltas
	// 			accion = UltimoRecurso(sensores);
	// 			return accion;
	// 		}
	// 	}
	// }
	// cout << "Plan a X encontrado" << endl;

	// if (hayPlan && index < plan.size())
	// {
	// 	cout << "ejecutando plan" << endl;
	// 	accion = plan[index];
	// 	++index;
	// }
	// if (index == plan.size())
	// {
	// 	cout << "Plan a X terminado" << endl;
	// 	hayPlan = false;
	// }
	return accion;
}

// Simula el plan y comprueba si tiene agua o matorrales
bool ComportamientoRescatador::CaminoDuro(const EstadoR & inicio, const EstadoR & fin, const vector<Action> & plan) {
	EstadoR estado = inicio;
	bool duro = false;
	for (Action accion : plan) {
		int impulso = (accion == RUN) ? 2 : 1;
		if (accion == WALK || accion == RUN) {
			estado.f += impulso * sf[estado.brujula];
			estado.c += impulso * sc[estado.brujula];
			char casilla = mapaResultado[estado.f][estado.c];
			// cout << casilla << endl;
			if (casilla == 'A' || casilla == 'T') {
				if (max(abs(estado.f - fin.f), abs(estado.c - fin.c)) > RADIO_INMERSION) {
					return true;
				}
			}
		} else if (accion == TURN_SR) {
			estado.brujula = (estado.brujula + 1) % 8;
		} else if (accion == TURN_L) {
			estado.brujula = (estado.brujula + 6) % 8;
		}
	}
	// cout << "Camino duro? " << boolalpha << duro << endl;
	return false;
}

// pre : no hayPlan
void ComportamientoRescatador::BuscaObjetivo(Sensores sensores, int f, int c)
{
	if(f == -1) f = sensores.destinoF;
	if(c == -1) c = sensores.destinoC;
	Action accion = IDLE;
	
	NodoR nodo_objetivo;
	NodoR nodo_base;
	NodoR nodo_camino;
	NodoR nodo_camino2;
	pair<vector<Action>,int> path_coste;
	EstadoR inicio, fin;
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
			nodo_objetivo = A_estrella(inicio, fin, sensores);
			bool imposible = nodo_objetivo.secuencia.size() == 0;
			camino_duro = CaminoDuro(inicio, fin, nodo_objetivo.secuencia);
			if(camino_duro) {
				// opcion_replanificacion = PERMISIVA;
				// cout << "Camino duro" << endl;
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
			// 	if(opcion_busqueda == NO_PERMISIVA) {
			// 		hay_plan_restrictivo = false;
			// 	}
			// 	else
			// 		cout << "ERROR" << endl;
			// }
			else {
				// hay_plan_restrictivo = false;
				necesito_recargar = true;
			}
			energia_necesaria = path_coste.second;
			
			// cout << "No es Dijkstra" << endl;
			VisualizaPlan(inicio, plan);
			// PintaPlan(plan, tiene_zapatillas);
			// cout << plan.size() << endl;
			hayPlan = (plan.size() != 0);
			if(!hayPlan) break;
		}
		if(!hayPlan) break;
		opcion_busqueda = PERMISIVA;
	}
	
	return;
}

void ComportamientoRescatador::AnularPlan() {
	plan.clear();
	hayPlan = false;
	index = 0;
	plan_inseguro = false;
	// buscando_recarga = false;
	camino_duro = false;
	// hay_plan_restrictivo = true;

	opcion_replanificacion = NO_PERMISIVA;
	opcion_busqueda = NO_PERMISIVA;
	// energia_necesaria = 0;

}

Action ComportamientoRescatador::UltimoRecurso(Sensores sensores) {
	Action accion = IDLE;
	EstadoR estado;
	estado.f = sensores.posF;
	estado.c = sensores.posC;
	estado.brujula = sensores.rumbo;
	estado.zapatillas = tiene_zapatillas;

	if(CasillaAccesibleRescatadorOpciones(estado, 1, 1)) {
		accion = WALK;
	}
	else if(CasillaAccesibleRescatadorOpciones(estado, 2, 1)) {
		accion = RUN;
	}
	else {
		accion = TURN_SR;
	}
	return accion;
}

bool ComportamientoRescatador::PlanCasillas(Sensores sensores, const vector<char> & casillas) {
	AnularPlan();
	NodoR nodo_base;
	int coste = 0;
	EstadoR estado;
	estado.f = sensores.posF;
	estado.c = sensores.posC;
	estado.brujula = sensores.rumbo;
	estado.zapatillas = tiene_zapatillas;
	vector<char> desconocidas = {'T', 'S', 'C'};
	bool imposible = false;
	for(int i=0; i<2; ++i) {
		for(char casilla_desconocida : desconocidas) {

			desconocidas_busqueda = casilla_desconocida;
			nodo_base = BuscaCasillas(estado, sensores, casillas);
			plan_inseguro = nodo_base.path_inseguro;
			// Compruebo si llego (si no estoy muy seguro, no me arriesgo)
			coste = nodo_base.g;
			imposible = nodo_base.secuencia.size() == 0;
			// cout << "Imposible?" << boolalpha << imposible << endl;
			bool llego = coste < sensores.energia;
			if(!imposible && llego) 
				plan = nodo_base.secuencia;
			VisualizaPlan(estado, plan);
			PintaPlan(plan, tiene_zapatillas);
			hayPlan = (plan.size() != 0);
		}
	}
	return !imposible;
}

Action ComportamientoRescatador::LlamarAuxiliar(Sensores sensores) {
	Action accion = IDLE;
	// Llamar al auxiliar
	// cout << "Tiempo de espera: " << tiempo_espera << endl;
	// cout << "Gravedad: " << sensores.gravedad << endl;
	if(tiempo_espera == 0) {
		// Acabo de llegar
		// cout << "Llamando al auxiliar" << endl;
		
		accion = CALL_ON;
		tiempo_espera++;
	}
	
	// Si tarda mucho, abortar misión
	else if(tiempo_espera > UMBRAL_TIEMPO) {
		// cout << "Abortando misión" << endl;
		accion = CALL_OFF;
		tiempo_espera = 0;
	}
	// Esperar
	else {
		accion = IDLE;
		tiempo_espera++;
	}
	
	

	return accion;
}

Action ComportamientoRescatador::OrganizaPlan(Sensores sensores) {
	Action accion = IDLE;

	
	if(hayPlan) {

		if(index == plan.size()) {
			AnularPlan();
		}
		
		else {

			
			if(HayQueReplanificar(sensores, plan[index])) {
				// Borrar plan
				// cout << "replanifico..." << endl;
				AnularPlan();
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
	return accion;
}

void ComportamientoRescatador::AnularRecarga() {
	necesito_recargar = false;
	buscando_recarga = false;
	recargando = false;
	contador = 0;
}

//! Problemas:

//! Hay posibilidad de que el rescatador no pueda hacer nada

// Action ComportamientoRescatador::ComportamientoRescatadorNivel_4(Sensores sensores) {
// 	Action accion = IDLE;

// 	// Actualizar el mapa con la información de los sensores
// 	SituarSensorEnMapa(sensores);

// 	if (sensores.superficie[0] == 'D') {
// 		tiene_zapatillas = true;
// 	}


// 	if(sensores.energia < 1000) {
// 		recargando = true;
// 	}

// 	desconocidas_busqueda = sensores.superficie[0];

// 	if ((sensores.posF == sensores.destinoF && sensores.posC == sensores.destinoC) || 
// 		(sensores.posF == objetivo_anterior.first && sensores.posC == objetivo_anterior.second)) {
		
// 		opcion_replanificacion = NO_PERMISIVA; // Ya he llegado al objetivo

// 		if (sensores.gravedad && tiempo_espera == 0) {
// 			accion = CALL_ON; // Llamar al auxiliar si es grave
// 		} else if(tiempo_espera > INF) {
// 			accion = CALL_OFF;
// 		} 
// 		else {
// 			accion = IDLE; // Esperar en el lugar
// 		}
// 		tiempo_espera++;

// 		// Si el objetivo cambia, reiniciar el estado
// 		if (objetivo_anterior.first != sensores.destinoF || objetivo_anterior.second != sensores.destinoC) {
// 			tiempo_espera = 0;
// 			objetivo_anterior = {sensores.destinoF, sensores.destinoC};

// 			// Si estoy en terreno desfavorable, buscar una casilla mejor
// 			// if (sensores.superficie[0] == 'A' || sensores.superficie[0] == 'T') {
// 			// 	PlanCasillas(sensores, {'C', 'S', 'D', 'X'});
// 			// }
// 		}

// 		return accion;
// 	}

// 	// Si la energía es insuficiente para llegar al objetivo o necesito recargar, buscar un puesto base
// 	if (sensores.energia < energia_necesaria || recargando || !conozco_bases) {
// 		if (sensores.superficie[0] == 'X') {
// 			// Si ya está en un puesto base, recargar hasta 3000 de energía
// 			recargando = true;
// 			conozco_bases = true;
// 			if(sensores.energia < 3000)
// 				return IDLE;
// 			else recargando = false;
// 		} else {
// 			if(hayPlan) {
// 				accion = plan[index];
// 			}
// 			if (!hayPlan || HayQueReplanificar(sensores, accion)) {
// 				// Generar un plan para ir al puesto base más cercano
// 				index = 0;
// 				plan.clear();
// 				char objetivo = 'X';
// 				if (!conozco_bases && !veo_base) objetivo = '?';
// 				EstadoR inicio;
// 				inicio.f = sensores.posF;
// 				inicio.c = sensores.posC;
// 				inicio.brujula = sensores.rumbo;
// 				inicio.zapatillas = tiene_zapatillas;
// 				plan = BuscaCasillas(inicio, sensores, {objetivo}).secuencia;
// 				camino_duro = CaminoDuro(inicio, {INF,INF}, plan);
// 				VisualizaPlan(inicio, plan);
// 				hayPlan = !plan.empty();
// 			}
// 			if(!hayPlan) {
// 				opcion_busqueda = PERMISIVA;
// 				opcion_replanificacion = PERMISIVA;
// 			}

// 			// Ejecutar el plan
// 			if (hayPlan && index < plan.size()) {
// 				accion = plan[index];
// 				++index;
// 			}

// 			// Si se completó el plan, resetear
// 			if (index == plan.size()) {
// 				hayPlan = false;
// 				opcion_busqueda = NO_PERMISIVA;
// 				opcion_replanificacion = NO_PERMISIVA;
// 			}

// 			return accion;
// 		}
// 	}

// 	cout << "Hay plan? " << boolalpha << hayPlan << endl;

// 	// Si no necesita recargar, ir al objetivo
// 	if(hayPlan) {
// 		accion = plan[index];
// 	}
// 	if (!hayPlan || HayQueReplanificar(sensores, accion)) {
// 		// Generar un plan para ir al objetivo
// 		index = 0;
// 		plan.clear();
// 		char objetivo = 'X';
// 		if (!conozco_bases) objetivo = '?';
// 		EstadoR inicio, fin;
// 		inicio.f = sensores.posF;
// 		inicio.c = sensores.posC;
// 		inicio.brujula = sensores.rumbo;
// 		inicio.zapatillas = tiene_zapatillas;
// 		fin.f = sensores.destinoF;
// 		fin.c = sensores.destinoC;
// 		NodoR nodo_objetivo = A_estrella(inicio, fin, sensores);
// 		camino_duro = CaminoDuro(inicio, fin, nodo_objetivo.secuencia);
// 		bool imposible = nodo_objetivo.secuencia.size() == 0;
// 		cout << "Imposible?" << boolalpha << imposible << endl;
// 		NodoR nodo_base;
// 		if(!imposible && conozco_bases) 
// 			nodo_base = BuscaCasillas(nodo_objetivo.estado, sensores, {objetivo});
		
// 		int coste = nodo_objetivo.g + nodo_base.g;
// 		bool llego = coste <= sensores.energia;
// 		if(llego) {
// 			plan = nodo_objetivo.secuencia;
// 		}
// 		else {
// 			recargando = true;
// 		}
// 		energia_necesaria = coste;
// 		VisualizaPlan(inicio, plan);
// 		hayPlan = !plan.empty();
// 	}
// 	cout << "Hay plan B? " << boolalpha << hayPlan << endl;
// 	if(!hayPlan) {
// 		opcion_busqueda = PERMISIVA;
// 		opcion_replanificacion = PERMISIVA;
// 	}

// 	// Ejecutar el plan
// 	if (hayPlan && index < plan.size()) {
// 		accion = plan[index];
// 		++index;
// 	}

// 	// Si se completó el plan, resetear
// 	if (index == plan.size()) {
// 		hayPlan = false;
// 	}

// 	return accion;
// }


Action ComportamientoRescatador::ComportamientoRescatadorNivel_4(Sensores sensores)
{
	Action accion = IDLE;
	instantes++;

	// cout << "Necesito recargar? " << boolalpha << necesito_recargar << endl;
	// cout << "Recargando? " << boolalpha << recargando << endl;

	int S = mapaResultado.size();
	bool mapa_grande = S > 60;

	// const int UMBRAL_TIEMPO = 1;//0*S;
	const int INF_ENERGIA = mapa_grande ? 1000 : 500;
	const int SUP_ENERGIA = mapa_grande ? 3000 : 2000;
	const int MAX_ENERGIA = 3000;
	const int TIEMPO_RECARGA_CASUAL = 0;
	const int ACCIONES_EXPLORACION = mapa_grande ? 100 : 50;
	// const int UMBRAL_INSTANTES = 1500 + S*sqrt(S);

	/**********************************************************************************************
	 * ? ACCIONES INICIALES
	 **********************************************************************************************/

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

	// cout << "Buscando recarga? " << boolalpha << buscando_recarga << endl;
	if(buscando_recarga) {
		recargando = sensores.energia < min(max(SUP_ENERGIA, energia_necesaria), MAX_ENERGIA);
		// cout << "Recargando = " << boolalpha << recargando << endl;
	}
	else {
		necesito_recargar = sensores.energia < max(INF_ENERGIA, energia_necesaria);
	}
	energia_necesaria = 0;

	if(recargando && sensores.choque) {
		AnularRecarga();
	}

	if(sensores.superficie[0] == 'X') {
		conozco_bases = true;
		if(recargando || contador < TIEMPO_RECARGA_CASUAL) {	
			contador++;
			return IDLE;
		} else {
			AnularRecarga();
		}
	}


	/**********************************************************************************************
	 * ? LLAMAR A AUXILIAR CUNADO LLEGO AL OBJETIVO
	 **********************************************************************************************/

	if((sensores.posF == sensores.destinoF && sensores.posC == sensores.destinoC) || (sensores.posF == objetivo_anterior.first && sensores.posC == objetivo_anterior.second)) {
		// cout << "Ya he llegado" << endl;

		opcion_replanificacion = NO_PERMISIVA; // Ya he llegado
		if(sensores.gravedad)
			accion = LlamarAuxiliar(sensores);
		// cout << "Plan casillas " << endl;
		// cout << "Superficie: " << (char)(sensores.superficie[0]) << endl;
		// cout << "Vida: " << sensores.vida << endl;
		// cout << "objetivo_anterior: " << objetivo_anterior.first << " " << objetivo_anterior.second << endl;
		// cout << "objetivo: " << sensores.destinoF << " " << sensores.destinoC << endl;
		if((objetivo_anterior.first != sensores.destinoF || objetivo_anterior.second != sensores.destinoC)) {
			tiempo_espera = 0;
			objetivo_anterior = {sensores.destinoF, sensores.destinoC};
			// cout << "Nuevo objetivo" << endl;
			if(sensores.superficie[0] == 'A' || sensores.superficie[0] == 'T') {
				// cout << "Estoy en mal terreno" << endl;
				PlanCasillas(sensores, {'C','S','D','X'});
				// PintaPlan(plan, tiene_zapatillas);
			}
			
		}

		return accion;
	}

	/**********************************************************************************************
	 * ? RECARGAR ENERGÍA 
	 **********************************************************************************************/

	 if(necesito_recargar) {
		buscando_recarga = true;
		necesito_recargar = false;
		// Dijkstra a puestos base
		// cout << "EJECUTANDO PLAN DE RECARGA" << endl;
		accion = PlanRecarga(sensores);
		return accion;
	}

	/**********************************************************************************************
	 * ? EXPLORAR UN POCO DE MAPA AL INICIO AL MENOS HASTA ENCONTRAR UN PUESTO BASE
	 **********************************************************************************************/
	
	if(!conozco_bases) {
		// Nivel 1
		if(sensores.superficie[0] == 'S' || sensores.superficie[0] == 'D' || sensores.superficie[0] == 'X') {
			modo_tonto = false;
			return ComportamientoRescatadorNivel_1(sensores);
		}
		// Nivel 0
		if(sensores.superficie[0] == 'C')  {
			modo_tonto = false;
			return ComportamientoRescatadorNivel_0(sensores);
		}
		else if(!modo_tonto){
			// Buscar camino
			bool factible = PlanCasillas(sensores, {'S','C','D','X'});
			if(!factible) {
				modo_tonto = true;
			}	
		}
		if(modo_tonto) {
			accion = UltimoRecurso(sensores);
			return accion;
		}
	}
	else if(instantes < ACCIONES_EXPLORACION) {
		return ComportamientoRescatadorNivel_1(sensores);
	}

	
	/**********************************************************************************************
	 * ? IR A OBJETIVO
	 **********************************************************************************************/

	else if(!hayPlan){

		// Si me choco o me voy a caer, recalculo el plan 
		// cout << "Tengo plan? " << boolalpha << hayPlan << endl;
		// cout << "Tengo que replanificar? " << boolalpha << HayQueReplanificar(sensores, accion) << endl;
		
		BuscaObjetivo(sensores);
		if(hayPlan) {
			ejecutando_plan_objetivo = true;
		}
		else {
			opcion_busqueda = PERMISIVA;
			opcion_replanificacion = PERMISIVA;
		}
		
	}
	// cout << "Hay plan restrictivo? " << boolalpha << hay_plan_restrictivo << endl;
	accion = OrganizaPlan(sensores);
	for(int i=1; i<16;++i) {
		pair<int,int> pos = VtoM(i,sensores.rumbo,{sensores.posF, sensores.posC});
		mapaEntidades[pos.first][pos.second] = '?';
	}
	

	return accion;
}

