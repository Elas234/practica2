#include "../Comportamientos_Jugador/auxiliar.hpp"
#include <iostream>
#include "motorlib/util.h"

#define TURN_SR_pendientes 0
#define WALK_pendientes 1

const int INF = 1e6;

const int sf[8] = {-1, -1, 0,  1,  1,  1,  0, -1};
const int sc[8] = { 0,  1, 1,  1,  0, -1, -1, -1};

Action ComportamientoAuxiliar::think(Sensores sensores)
{
	Action accion = IDLE;

	switch (sensores.nivel)
	{
	case 0:
		accion = ComportamientoAuxiliarNivel_0(sensores);
		break;
	case 1:
		// accion = ComportamientoAuxiliarNivel_1 (sensores);
		break;
	case 2:
		// accion = ComportamientoAuxiliarNivel_2 (sensores);
		break;
	case 3:
		// accion = ComportamientoAuxiliarNivel_3 (sensores);
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

pair<int, int> ComportamientoAuxiliar::VtoM(int i, Orientacion rumbo, pair<int, int> & orig)
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
	
	/******************************************************************************/

	// NUEVA VERSIÓN CON VTOM

	int f = sensores.posF;
	int c = sensores.posC;
	pair<int,int> orig = {f, c};
	for(int i=0; i<16; ++i) {
		pair<int,int> casilla = VtoM(i, sensores.rumbo, orig);
		int nf = casilla.first;
		int nc = casilla.second;
		mapaResultado[nf][nc] = sensores.superficie[i];
		mapaCotas[nf][nc] = sensores.cota[i];
		if(sensores.superficie[i] == 'X' && sensores.agentes[i] == 'r') {
			mapaEntidades[nf][nc] = 'r';
		}
	}

	/******************************************************************************/

	// VERSIÓN ANTERIOR 

	// int y = sensores.posF;
	// int x = sensores.posC;
	// int cont = 0;
	// int mx, my;
	// Orientacion o = sensores.rumbo;
	// int sgn[2] = {1, -1};
	// int s = sgn[o/4];

	// if(o&1) {
	// 	for(int i=0; i<4; ++i) {
	// 		for(int j=i;j>0;--j) {
	// 			switch(o) {
	// 				case noreste:
	// 				case suroeste:
	// 					my = y-s*i;
	// 					mx = x+s*(i-j);
	// 					break;
	// 				case sureste:
	// 				case noroeste:
	// 					my = y+s*(i-j);
	// 					mx = x+s*i;
	// 					break;
	// 			}
	// 			m[my][mx] = sensores.superficie[cont];
	// 			a[my][mx] = sensores.cota[cont];
	// 			++cont;
	// 		}


	// 		// Noreste (s=1)
	// 		//? m[y-i][x+i-j] = sensores.superficie[cont];

	// 		// Sureste (s=1)
	// 		//? m[y+i-j][x+i] = sensores.superficie[cont];

	// 		// Suroeste (s=-1)
	// 		//? m[y+i][x-i+j] = sensores.superficie[cont];

	// 		// Noroeste (s=-1)
	// 		//? m[y-i+j][x-i] = sensores.superficie[cont];

	// 		for(int j=0;j<=i;++j) {
	// 			switch(o) {
	// 				case noreste:
	// 				case suroeste:
	// 					my = y-s*(i-j);
	// 					mx = x+s*i;
	// 					break;
	// 				case sureste:
	// 				case noroeste:
	// 					my = y+s*i;
	// 					mx = x+s*(i-j);
	// 					break;
	// 			}
	// 			m[my][mx] = sensores.superficie[cont];
	// 			a[my][mx] = sensores.cota[cont];
	// 			++cont;
	// 		}
	// 	}
	// }
	// else {

	// 	for(int i=0; i<4; ++i) {
	// 		for(int j=-i;j<=i;++j) {
	// 			switch (o) {
	// 				case norte:
	// 				case sur:
	// 					mx = x + s*j;
	// 					my = y - s*i;
	// 					break;
	// 				case este:
	// 				case oeste:
	// 					mx = x + s*i;
	// 					my = y + s*j;
	// 					break;
	// 			}
	// 			m[my][mx] = sensores.superficie[cont];
	// 			a[my][mx] = sensores.cota[cont];
	// 			++cont;
	// 		}
	// 	}
	// }


	// Norte
	//? m[x+j][y-i] = sensores.superficie[cont];

	// Sur	
	//? m[x-j][y+i] = sensores.superficie[cont];

	// Este
	//? m[x+i][y+j] = sensores.superficie[cont];

	// Oeste
	//? m[x-i][y-j] = sensores.superficie[cont];
	
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
	const vector<unsigned char> altura = sensores.cota;
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

					// Puntuación inversamente proporcional a la diferencia de altura
					puntuacion_destino[i] += (POINTS[k]/(1+abs(altura[M[i][j]]-altura[DESTINOS[i]])));
					
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

int ComportamientoAuxiliar::SelectCasillaAllAround(const pair<int,int> & orig, const vector<int> & casillas_interesantes, 
	const vector<bool> & is_interesting, Orientacion rumbo)
{

	const int ALCANZABLES = 8;
	const int NUM_RELEVANT = 2;

	const char RELEVANT[NUM_RELEVANT] = {'X', 'C'};
	const int POINTS[NUM_RELEVANT] = {100, 10};

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

	int peso_altura = 1;
	int peso_frecuencia_adyacentes = 5;
	int peso_frecuencia = 10;
	int puntuacion_destino[ALCANZABLES] = {0};
	int puntuaciones_iniciales[ALCANZABLES] = 
	{100, 50, 10, 5, 1, 5 , 10, 50};

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

	cout << "Puntuaciones: " << endl;
	for(int i=0; i<ALCANZABLES; ++i) {
		cout << i << ": " << puntuacion_destino[i] << endl;
	}
	cout << "Ganador: " << decision << endl;

	return decision;

}

void ComportamientoAuxiliar::CasillasInteresantes (const Sensores & sensores, const vector<bool> & accesible, 
	vector<bool> & is_interesting, vector<int> & casillas_interesantes) {

	is_interesting.resize(16, false);
	casillas_interesantes.clear();

	vector<unsigned char> vision = sensores.superficie;
	vector<unsigned char> altura = sensores.cota;
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

void ComportamientoAuxiliar::CasillasInteresantesAllAround(const pair<int,int> & orig, const vector<bool> & accesible, 
	vector<bool> & is_interesting, vector<int> & casillas_interesantes) {

	int f = orig.first;
	int c = orig.second;

	const int ALCANZABLES = 8;
	const int NUM_RELEVANT = 2;
	const char RELEVANT[NUM_RELEVANT] = {'X', 'C'};

	is_interesting.resize(ALCANZABLES, false);
	casillas_interesantes.clear();

	for(char x : RELEVANT) {
		for(int i=0; i<ALCANZABLES; ++i) {
			int nf = f + sf[i];
			int nc = c + sc[i];
			if(mapaResultado[nf][nc] == x && accesible[i]) {
				if(mapaEntidades[nf][nc] != '_') continue;
				if(x=='X') {
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

	// if(num_acciones % 1000 == 0) {
	// 	// Inicializo el mapa de frecuencias
	// 	for(int i=0; i<mapaFrecuencias.size(); ++i) {
	// 		for(int j=0; j<mapaFrecuencias[i].size(); ++j) {
	// 			mapaFrecuencias[i][j] = 0;
	// 		}
	// 	}
	// }

	// Actualización de variables de estado

	SituarSensorEnMapa(sensores);
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

	if (sensores.superficie[0] == 'X') {
		action = IDLE;
	}
	else if(!TengoTareasPendientes(sensores, action)) {
		// Casillas que puedo atravesar corriendo
		vector<bool> transitable(8);

		// Casillas a las que puedo acceder (accesible ==> transitable pero no al revés)
		vector<bool> accesible(8);

		// Casilla genérica
		// for(int i=1; i<16; ++i) {
		// 	transitable[i] = (sensores.superficie[i] != 'P' && sensores.superficie[i] != 'M' && (sensores.superficie[i] != 'B' || tiene_zapatillas) );
		// 	accesible[i] = transitable[i] && ViablePorAltura(sensores.cota[i]-sensores.cota[0]);
		// }

		int f = sensores.posF;
		int c = sensores.posC;
		for(int i=0; i<8; ++i) {
			int nf = f + sf[i];
			int nc = c + sc[i];
			char s = mapaResultado[nf][nc];
			int h = mapaCotas[nf][nc];
			char e = mapaEntidades[nf][nc];
			transitable[i] = (s != 'P' && s != 'M' && (s != 'B' || tiene_zapatillas) && s != '?' && e == '_');
			accesible[i] = transitable[i] && ViablePorAltura(h-mapaCotas[f][c]);
		}

		vector<int> casillas_interesantes;
		vector<bool> is_interesting(8, false);

		// CasillasInteresantes(sensores, accesible, is_interesting, casillas_interesantes);
		// int decision = SelectCasilla(sensores, casillas_interesantes, is_interesting);

		CasillasInteresantesAllAround({f,c}, accesible, is_interesting, casillas_interesantes);

		cout << "Casillas interesantes: ";
		for(int i=0; i<casillas_interesantes.size(); ++i) {
			cout << casillas_interesantes[i] << " ";
		}
		cout << endl;
		int decision = SelectCasillaAllAround({f,c}, casillas_interesantes, is_interesting, sensores.rumbo);
		action = SelectAction(decision, sensores.rumbo);

	}

	if((action == WALK && sensores.agentes[2] != '_')) {
		// Resetea movimientos peligrosos
		cout << "Auxiliar: me voy a chocar, doy la vuelta" << endl;
		fill(acciones_pendientes.begin(), acciones_pendientes.end(), 0);
		acciones_pendientes[TURN_SR_pendientes] = 3;
		action = TURN_SR;
	}

	// Resetea mapa de entidades
	for(int i=0; i<mapaEntidades.size(); ++i) {
		for(int j=0; j<mapaEntidades[i].size(); ++j) {
			if(mapaResultado[i][j] != 'X' || mapaEntidades[i][j] != 'r') {
				mapaEntidades[i][j] = '_';
			}
		}
	}

	last_action = action;
	++num_acciones;

	return action;
}

/*********************************** NIVEL 1 **********************************************/
Action ComportamientoAuxiliar::ComportamientoAuxiliarNivel_1(Sensores sensores)
{
}

/*********************************** NIVEL 2 **********************************************/
Action ComportamientoAuxiliar::ComportamientoAuxiliarNivel_2(Sensores sensores)
{
}

/*********************************** NIVEL 3 **********************************************/
Action ComportamientoAuxiliar::ComportamientoAuxiliarNivel_3(Sensores sensores)
{
}

/*********************************** NIVEL 4 **********************************************/
Action ComportamientoAuxiliar::ComportamientoAuxiliarNivel_4(Sensores sensores)
{
}

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

bool ComportamientoAuxiliar::IsSolution(const EstadoA &estado, const EstadoA &final)
{
	return estado.f == final.f && estado.c == final.c;
}

bool ComportamientoAuxiliar::CasillaAccesibleAuxiliar(const EstadoA &st, const vector<vector<unsigned char>> &terreno,
													  const vector<vector<unsigned char>> &altura)
{
	EstadoA next = NextCasillaAuxiliar(st);
	bool check1 = false, check2 = false, check3 = false;
	check1 = terreno[next.f][next.c] != 'P' && terreno[next.f][next.c] != 'M';
	check2 = terreno[next.f][next.c] != 'B' || (terreno[next.f][next.c] == 'B' && st.zapatillas);
	check3 = abs(altura[next.f][next.c] - altura[st.f][st.c]) <= 1;
	return check1 && check2 && check3;
}

EstadoA ComportamientoAuxiliar::NextCasillaAuxiliar(const EstadoA &st)
{
	// EstadoA next = st;
	// int avance_f[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
	// int avance_c[8] = {0, 1, 1, 1, 0, -1, -1, -1};
	// next.f += avance_f[st.brujula];
	// next.c += avance_c[st.brujula];
	// return next;

	EstadoA siguiente = st;
	switch (st.brujula)
	{
	case norte:
		siguiente.f = st.f - 1;
		break;
	case noreste:
		siguiente.f = st.f - 1;
		siguiente.c = st.c + 1;
		break;
	case este:
		siguiente.c = st.c + 1;
		break;
	case sureste:
		siguiente.f = st.f + 1;
		siguiente.c = st.c + 1;
		break;
	case sur:
		siguiente.f = st.f + 1;
		break;
	case suroeste:
		siguiente.f = st.f + 1;
		siguiente.c = st.c - 1;
		break;
	case oeste:
		siguiente.c = st.c - 1;
		break;
	case noroeste:
		siguiente.f = st.f - 1;
		siguiente.c = st.c - 1;
	}
	return siguiente;
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

void ComportamientoAuxiliar::PintaPlan(const list<Action> &plan, bool zap)
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

void ComportamientoAuxiliar::VisualizaPlan(const EstadoA &st, const list<Action> &plan)
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

EstadoA ComportamientoAuxiliar::applyA(Action accion, const EstadoA &st, const vector<vector<unsigned char>> &terreno,
									   const vector<vector<unsigned char>> &altura)
{
	EstadoA next = st;
	switch (accion)
	{
	case WALK:
		if (CasillaAccesibleAuxiliar(st, terreno, altura))
		{
			next = NextCasillaAuxiliar(st);
		}
		break;
	case TURN_SR:
		next.brujula = (next.brujula + 1) % 8;
		break;
	}
	return next;
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

list<Action> ComportamientoAuxiliar::AnchuraAuxiliar(const EstadoA &inicio, const EstadoA &final,
													 const vector<vector<unsigned char>> &terreno, const vector<vector<unsigned char>> &altura)
{
	NodoA current_node;
	list<NodoA> frontier;
	list<NodoA> explored;
	list<Action> path;

	current_node.estado = inicio; // Asigna el estado inicial al nodo actual
	frontier.push_back(current_node);
	bool SolutionFound = IsSolution(current_node.estado, final);

	while (!SolutionFound && !frontier.empty())
	{
		frontier.pop_front();
		explored.push_back(current_node);

		// for each action applicable to current_node do
		// begin
		// 	child ← problem.apply(action, current_node)
		// 	if (problem.Is_Solution(child) then current_node = child
		// 	else if child.state() is not in explored or frontier then
		// 		frontier.insert(child)
		// end
		// if (!problem.Is_Solution(current_node) then
		// 	current_node ← frontier.next()

		if (terreno[current_node.estado.f][current_node.estado.c] == 'D')
		{
			current_node.estado.zapatillas = true;
		}

		NodoA child_WALK;
		child_WALK.estado = applyA(WALK, current_node.estado, terreno, altura);
		child_WALK.secuencia.push_back(WALK);
		if (IsSolution(child_WALK.estado, final))
		{
			current_node = child_WALK;
			SolutionFound = true;
		}

		else if (!Find(child_WALK, explored) && !Find(child_WALK, frontier))
		{
			child_WALK.secuencia.push_back(WALK);
			frontier.push_back(child_WALK);
		}

		if (!SolutionFound)
		{
			NodoA child_TURN_SR = current_node;
			child_TURN_SR.estado = applyA(TURN_SR, current_node.estado, terreno, altura);
			child_TURN_SR.secuencia.push_back(TURN_SR);
			if (!Find(child_TURN_SR, explored) && !Find(child_TURN_SR, frontier))
			{
				child_TURN_SR.secuencia.push_back(TURN_SR);
				frontier.push_back(child_TURN_SR);
			}
		}

		if (!SolutionFound && !frontier.empty())
		{
			current_node = frontier.front();
			SolutionFound = (current_node.estado.f == final.f && current_node.estado.c == final.c);
		}
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
		// Invocar al método de búsqueda
		EstadoA inicio, fin;
		inicio.f = sensores.posF;
		inicio.c = sensores.posC;
		inicio.brujula = sensores.rumbo;
		fin.f = sensores.destinoF;
		fin.c = sensores.destinoC;
		plan = AnchuraAuxiliar(inicio, fin, mapaResultado, mapaCotas);
		VisualizaPlan(inicio, plan);
		hayPlan = plan.size() != 0;
	}
	if (hayPlan && plan.size() > 0)
	{
		accion = plan.front();
		plan.pop_front();
	}
	if (plan.size() == 0)
	{
		hayPlan = false;
	}
	return accion;
}