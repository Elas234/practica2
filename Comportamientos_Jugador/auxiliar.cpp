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
		accion = ComportamientoAuxiliarNivel_E(sensores);
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

int ComportamientoAuxiliar::VeoCasillaInteresanteA(char i, char c, char d)
{
	if (c == 'X')
		return 2;
	else if (i == 'X')
		return 1;
	else if (d == 'X')
		return 3;
	else if (c == 'C')
		return 2;
	else if (i == 'C')
		return 1;
	else if (d == 'C')
		return 3;
	else
		return 0;
}

char ComportamientoAuxiliar::ViablePorAlturaA(char casilla, int dif)
{
	if (abs(dif) <= 1)
		return casilla;
	else
		return 'P';
}

void ComportamientoAuxiliar::SituarSensorEnMapaA(vector<vector<unsigned char>> &m, vector<vector<unsigned char>> &a, Sensores sensores)
{

	int x = sensores.posF;
	int y = sensores.posC;
	int cont = 0;
	int mx, my;
	Orientacion o = sensores.rumbo;
	int sgn[2] = {1, -1};
	for (int i = 0; i < 4; ++i)
	{
		for (int j = -i; j <= i; ++j)
		{
			switch (o)
			{
			case norte:
			case sur:
				mx = x + j;
				my = y + sgn[o / 4] * i;
				break;
			case este:
			case oeste:
				mx = x + sgn[o / 4] * i;
				my = y + j;
				break;
			}
			m[mx][my] = sensores.superficie[cont];
			++cont;
		}
	}

	// Norte
	// m[x+j][y+i] = sensores.superficie[cont];

	// Sur
	// m[x+j][y-i] = sensores.superficie[cont];

	// Este
	// m[x+i][y+j] = sensores.superficie[cont];

	// Oeste
	// m[x-i][y+j] = sensores.superficie[cont];

	for (int i = 0; i < 4; ++i)
	{
		for (int j = i; j > 0; ++j)
		{
			switch (o)
			{
			case noreste:
			case suroeste:
				mx = x + sgn[o / 4] * (i - j);
				my = y + sgn[o / 4] * i;
				break;
			case este:
			case oeste:
				mx = x + sgn[o / 4] * i;
				my = y + sgn[o / 4] * (i - j);
				break;
			}
			m[mx][my] = sensores.superficie[cont];
			++cont;
		}

		// Noreste
		// m[x+i-j][y+i] = sensores.superficie[cont];

		// Sureste
		// m[x+i][y+i-j] = sensores.superficie[cont];

		// Suroeste
		// m[x-i+j][y-i] = sensores.superficie[cont];

		// Noroeste
		// m[x-i][y-i+j] = sensores.superficie[cont];

		for (int j = 0; j <= i; ++j)
		{
			switch (o)
			{
			case noreste:
			case suroeste:
				mx = x + sgn[o / 4] * i;
				my = y + sgn[o / 4] * (i - j);
				break;
			case este:
			case oeste:
				mx = x + sgn[o / 4] * (i - j);
				my = y + sgn[o / 4] * i;
				break;
			}
			m[mx][my] = sensores.superficie[cont];
			++cont;
		}

		// Noreste
		// m[x+i][y+i-j] = sensores.superficie[cont];

		// Sureste
		// m[x+i-j][y+i] = sensores.superficie[cont];

		// Suroeste
		// m[x-i][y-i+j] = sensores.superficie[cont];

		// Noroeste
		// m[x-i+j][y-i] = sensores.superficie[cont];
	}
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
	// SituarSensorEnMapaA(mapaResultado, mapaCotas, sensores);

	if (sensores.superficie[0] == 'D')
	{
		tiene_zapatillas = true;
	}

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
		action = TURN_SR;
	}
	else
	{
		char i = ViablePorAlturaA(sensores.superficie[1], sensores.cota[1] - sensores.cota[0]);
		char c = ViablePorAlturaA(sensores.superficie[2], sensores.cota[2] - sensores.cota[0]);
		char d = ViablePorAlturaA(sensores.superficie[3], sensores.cota[3] - sensores.cota[0]);
		int pos = VeoCasillaInteresanteA(i, c, d);
		switch (pos)
		{
		case 2:
			action = WALK;
			break;
		case 1:
			giro45izq = 6;
			action = TURN_SR;
			break;
		case 3:
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

bool IsSolution(const EstadoA &estado, const EstadoA &final)
{
	return estado.f == final.f && estado.c == final.c;
}

bool CasillaAccesibleAuxiliar(const EstadoA &st, const vector<vector<unsigned char>> &terreno,
							  const vector<vector<unsigned char>> &altura)
{
	EstadoA next = NextCasillaAuxiliar(st);
	bool check1 = false, check2 = false, check3 = false;
	check1 = terreno[next.f][next.c] != 'P' && terreno[next.f][next.c] != 'M';
	check2 = terreno[next.f][next.c] != 'B' || (terreno[next.f][next.c] == 'B' && st.zapatillas);
	check3 = abs(altura[next.f][next.c] - altura[st.f][st.c]) <= 1;
	return check1 && check2 && check3;
}

EstadoA NextCasillaAuxiliar(const EstadoA &st)
{
	EstadoA next = st;
	int avance_f[8] = {1, 1, 0, -1, -1, -1, 0, 1};
	int avance_c[8] = {0, 1, 1, 1, 0, -1, -1, -1};
	next.f += avance_f[st.brujula];
	next.c += avance_c[st.brujula];
	return next;
}

EstadoA applyA(Action accion, const EstadoA &st, const vector<vector<unsigned char>> &terreno,
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
		if (IsSolution(child_WALK.estado, final)) {
			current_node = child_WALK;
			SolutionFound = true;
		}

		else if (false);
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
		plan = AvanzaASaltosDeCaballo();
		hayPlan = true;
	}
	if (hayPlan and plan.size() > 0)
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