#include "../Comportamientos_Jugador/rescatador.hpp"
#include "motorlib/util.h"
#include <iostream>
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
		mapaFrecuencias[f][c]++;

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
	bool check1 = false, check2 = false, check3 = false, check4 = false;
	check1 = siguiente != 'P' && siguiente != 'M' && siguiente != 'B';
	check2 = inter != 'P' && inter != 'M' && inter != 'B';
	check3 = (dif <= 1) || (st.zapatillas && (dif <= 2));
	check4 = 3 <= next.f && next.f < mapaResultado.size()-3 && 3 <= next.c && next.c < mapaResultado[0].size()-3;
	if(nivel == 2)
		return check1 && check2 && check3;
	else {
		return check1 && check2 && (check3 || siguiente == '?' || mapaResultado[st.f][st.c] == '?') && check4;
	}
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
	int impulso = 1;
	switch (accion)
	{
	case RUN:
		impulso = 2;
	case WALK:
		if (CasillaAccesibleRescatador(st, impulso, sensores.nivel)) {
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
		case RUN:
			switch(casilla)
			{
				case 'A':
					coste = 150;
					inc = 15;
					break;
				case 'T':
					coste = 35;
					inc = 5;
					break;
				case 'S':
					coste = 3;
					inc = 2;
					break;
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
				case 'A':
					coste = 30;
					break;
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

vector<Action> ComportamientoRescatador::A_estrella(const EstadoR &inicio, const EstadoR &final, const Sensores & sensores) {
	NodoR current_node;
	min_priority_queue frontier;
	set<EstadoR> explored;
	vector<Action> path;

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
		cout << "Dijkstra" << endl;
		if(sensores.nivel == 2)
			plan = Dijkstra(inicio, fin, sensores);
		else // Nivel 4
			plan = A_estrella(inicio, fin, sensores);
		cout << "No es Dijkstra" << endl;
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
		index = 0;
		hayPlan = false;
	}
	return accion;
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

bool ComportamientoRescatador::HayQueReplanificar(const Sensores & sensores, const Action & accion, const EstadoR & estado) {
	if(sensores.choque) return true;
	if(accion != WALK && accion != RUN) return false;
	int impulso = (accion == RUN) ? 2 : 1;
	if(!CasillaAccesibleRescatador(estado, impulso, 2)) return true;

	return false;
}

//TODO : A* para rescatador en lugar de Dijkstra
//TODO : A* para puestos base (multidirigido) en lugar de reactivo
//TODO : mejorar comportamiento nivel 1 para auxiliar
//TODO : maybe cambiar condición de redirección para evitar cruzar mucha agua
//TODO : recargar energía al máximo


//! Errores:
//! Core dump al replanificar después de recargar (solo da core el rescatador)

//? Teorías: 
//? En el A* hay un momento en el que se puede acceder a una cola vacía, esto solo ocurre
//? si el A* no puede alcanzar el destino, ya que se acaban los nodos por explorar

//? Por tanto lo más probable es que la razón del core sea que el A* no puede llegar al objetivo después
//? de recargar, tan solo habría que ver por qué no puede. Antes dio core porque consideraba las '?' como
//? no transitables y no podía llegar al objetivo, así que habría que imprimir que nodos se exploran para ver cuántos
//? se exploran y ver cuál es el último.

Action ComportamientoRescatador::ComportamientoRescatadorNivel_4(Sensores sensores)
{
	Action accion = IDLE;

	const int UMBRAL_TIEMPO = 300;
	const int UMBRAL_ENERGIA = 2700;
	const int RECARGA = 100;

	if((sensores.energia <= UMBRAL_ENERGIA && (sensores.superficie[0] == 'C' || sensores.superficie[0] == 'X')) || recargar) {
		// Low battery, abandonar posible comportamiento deliberativo para recargar
		hayPlan = false;
		plan.clear();

		cout << tiempo_recarga << endl;
		
		if(tiempo_recarga == 0) {
			recargar = true;
			hayPlan = false;
			plan.clear();
		}
		++tiempo_recarga;
		if(tiempo_recarga > RECARGA) {
			recargar = false;
			tiempo_recarga = 0;
		}
		return ComportamientoRescatadorNivel_0(sensores);
	}
	cout << "uwu1" << endl;
	// Print mapa alturas
	SituarSensorEnMapa(sensores);

	// for(int i=0; i<mapaCotas.size(); ++i) {
	// 	for(int j=0; j<mapaCotas[i].size(); ++j) {
	// 		cout << (int)mapaCotas[i][j] << " ";
	// 	}
	// 	cout << endl;
	// }
	if(sensores.superficie[0] == 'D') {
		tiene_zapatillas = true;
	}
	cout << "uwu2" << endl;
	// Cuando llegue al objetivo, llamo al auxiliar
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
	// Llamar a A* considerando las casillas desconocidas como transitables
	else {
		cout << "No obj" << endl;
		EstadoR estado;
		estado.f = sensores.posF;
		estado.c = sensores.posC;
		estado.brujula = sensores.rumbo;
		estado.zapatillas = tiene_zapatillas;

		// Si me choco o me voy a caer, recalculo el plan 
		if(hayPlan && HayQueReplanificar(sensores, plan[index], estado)) {
			// cout << "uwu" << endl;
			hayPlan = false;
		}
		cout << "planifico" << endl;
		cout << boolalpha << hayPlan << endl;
		cout << plan.size() << endl;
		accion = ComportamientoRescatadorNivel_2(sensores);
	}
	cout << "uwu3" << endl;

	// Repeat
	// if(accion == CALL_ON) {
	// 	cout << "TE ESTOY LLAMANDO 100% !!!" << endl;
	// }

	return accion;
}