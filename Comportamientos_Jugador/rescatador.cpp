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

	SituarSensorEnMapa(sensores);
	mapaFrecuencias[f][c]++;

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

Action ComportamientoRescatador::ComportamientoRescatadorNivel_2(Sensores sensores)
{
}

Action ComportamientoRescatador::ComportamientoRescatadorNivel_3(Sensores sensores)
{
}

Action ComportamientoRescatador::ComportamientoRescatadorNivel_4(Sensores sensores)
{
}