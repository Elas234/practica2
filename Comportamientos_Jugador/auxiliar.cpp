#include "../Comportamientos_Jugador/auxiliar.hpp"
#include <iostream>
#include "motorlib/util.h"

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

int ComportamientoAuxiliar::TomarDecision (const vector<unsigned char> & vision) {

	
	//? Primero compruebo si puedo ir a algún lado, y solo hago cálculos si puedo ir a más de uno

	const int ALCANZABLES = 3;
	const int NUM_RELEVANT = 2;

	const char RELEVANT[NUM_RELEVANT] = {'X', 'C'};
	const char POINTS[NUM_RELEVANT] = {10, 1};

	bool is_interesting[ALCANZABLES] = {false};
	vector<int> casillas_interesantes;
	for(char c : RELEVANT) {
		for(int i=1; i<=ALCANZABLES; ++i)
			if(vision[i] == c) {
				if(c=='X') return i;
				is_interesting[i] = true;
				casillas_interesantes.push_back(i);
				break;
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
	const int CASILLAS_POR_SECCION = 9;
	const int M[ALCANZABLES][CASILLAS_POR_SECCION] = 
	{
		{1,2,4,5,6,9,10,11,12}, // izquierda (incluye columna central)
		{2,3,6,7,8,12,13,14,15}, // derecha (incluye columna central)
		{1,2,3,5,6,7,11,12,13}	// centro
	}; 

	int puntuacion_camino[3] = {0};
	int max_points = 0;
	int decision = 0;
	for(int i=0; i<ALCANZABLES; ++i) {
		if(!is_interesting[i]) continue;
		for(int j=0; j<CASILLAS_POR_SECCION; ++j) {
			for(int k=0; k<NUM_RELEVANT; ++k) {
				if(vision[M[i][j]] == RELEVANT[k]) {
					puntuacion_camino[i] += POINTS[k];
				}
			}
		}
		// Actualizar máximo
		if(puntuacion_camino[i] > max_points) {
			max_points = puntuacion_camino[i];
			decision = i;
		}
	}
	// Ajuste de índices (1-base)
	return decision+1;

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


char ComportamientoAuxiliar::ViablePorAltura(char casilla, int dif)
{
	if (abs(dif) <= 1)
		return casilla;
	else
		return 'P';
}

void ComportamientoAuxiliar::SituarSensorEnMapa(vector<vector<unsigned char>> &m, vector<vector<unsigned char>> &a, Sensores sensores) {
	
	int y = sensores.posF;
	int x = sensores.posC;
	int cont = 0;
	int mx, my;
	Orientacion o = sensores.rumbo;
	int sgn[2] = {1, -1};
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
				m[my][mx] = sensores.superficie[cont];
				a[my][mx] = sensores.cota[cont];
				++cont;
			}
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
	SituarSensorEnMapa(mapaResultado, mapaCotas, sensores);

	/*
		Fase 2: Actuar

		Giro a la izquierda: TURN_L + TURN_SR

		Hago un giro a la izquierda, guardo en la variable de estado
		que estoy girando (la pongo a 1) y giro a la derecha
	*/

	if (sensores.superficie[0] == 'X')
	{
		action = IDLE;
	}
	else if (giro45izq != 0)
	{
		action = TURN_SR;
		giro45izq--;
	}
	else if (giro180 != 0)
	{
		action = TURN_SR;
		giro180--;
	}
	else if (sensores.agentes[2] != '_')
	{
		giro180 = 3;
		// Resetea movimientos peligrosos
		avanza = giro45izq = 0;
		action = TURN_SR;
	}
	else if(avanza != 0) {
		action = WALK;
		avanza--;
	}
	else
	{
		vector<unsigned char> vision_con_cotas(16);
		for(int i=1; i<16; ++i) {
			if(i<4)
				vision_con_cotas[i] = ViablePorAltura(sensores.superficie[i], sensores.cota[i]-sensores.cota[0]);
			else
				vision_con_cotas[i] = sensores.superficie[i];
		}

		int pos = TomarDecision(vision_con_cotas);
		switch (pos)
		{
		case 1:
			giro45izq = 6;
			avanza = 1;
			action = TURN_SR;
			break;
		case 2:
			action = WALK;
			break;
		case 3:
			avanza = 1;
			action = TURN_SR;
			break;
		case 0:
			action = TURN_SR;
			break;
		}
	}

	last_action = action;

	return action;
}

Action ComportamientoAuxiliar::ComportamientoAuxiliarNivel_1(Sensores sensores)
{
}

Action ComportamientoAuxiliar::ComportamientoAuxiliarNivel_2(Sensores sensores)
{
}

Action ComportamientoAuxiliar::ComportamientoAuxiliarNivel_3(Sensores sensores)
{
}

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
		inicio.zapatillas = tiene_zapatillas;
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