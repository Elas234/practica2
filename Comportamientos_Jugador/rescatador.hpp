#ifndef COMPORTAMIENTORESCATADOR_H
#define COMPORTAMIENTORESCATADOR_H

#include <chrono>
#include <time.h>
#include <thread>
#include <list>
#include <set>
#include <queue>

#include "comportamientos/comportamiento.hpp"

// Opciones de replanificacion

#define ESTANDAR 0
#define PERMISIVA 1
#define NO_PERMISIVA 2

struct EstadoR
{
	int f;
	int c;
	int brujula;
	bool zapatillas;

	bool operator==(const EstadoR &st) const
	{
		return f == st.f && c == st.c && brujula == st.brujula && zapatillas == st.zapatillas;
	}
	bool operator<(const EstadoR &st) const
	{
		if (f < st.f) return true;
		else if (f == st.f && c < st.c) return true;
		else if (f == st.f && c == st.c && brujula < st.brujula) return true;
		else if (f == st.f && c == st.c && brujula == st.brujula && zapatillas < st.zapatillas) return true;
		else return false;
	}
	bool operator>(const EstadoR &st) const
	{
		return !(*this < st);
	}
};

struct NodoR
{
	EstadoR estado;
	vector<Action> secuencia;
	bool path_inseguro;
	int g;
	int h;

	NodoR() : g(0) , h(0), path_inseguro(false) {}

	bool operator==(const NodoR &nodo) const
	{
		return estado == nodo.estado;
	}

	bool operator<(const NodoR &node) const {
		int f1 = g + h;
		int f2 = node.g + node.h;
		if (f1 != f2) return f1 < f2;
		else if (f1 == f2) {
			if (g != node.g) return g < node.g;
			// Rompe empates por componentes del estado
			if (estado.f != node.estado.f) return estado.f < node.estado.f;
			if (estado.c != node.estado.c) return estado.c < node.estado.c;
			if (estado.brujula != node.estado.brujula) return estado.brujula < node.estado.brujula;
			return estado.zapatillas < node.estado.zapatillas;
		}
		else return false;
	}

	bool operator>(const NodoR &node) const {
		int f1 = g + h;
		int f2 = node.g + node.h;
		if (f1 != f2) return f1 > f2;
		else if(f1 == f2) {
		// Rompe empates al revés que operator<
			if (g != node.g) return g > node.g;
			if (estado.f != node.estado.f) return estado.f > node.estado.f;
			if (estado.c != node.estado.c) return estado.c > node.estado.c;
			if (estado.brujula != node.estado.brujula) return estado.brujula > node.estado.brujula;
			return estado.zapatillas > node.estado.zapatillas;
		}
		else return false;
	}
};

class ComportamientoRescatador : public Comportamiento
{

public:
	/**
	 * @brief Constructor por defecto para los niveles 0,1,4
	 * @param size Tamaño del mapa
	 */
	ComportamientoRescatador(unsigned int size = 0) : Comportamiento(size)
	{
		// Inicializar Variables de Estado Niveles 0,1,4 (constructor)

		last_action = IDLE;
		tiene_zapatillas = false;
		acciones_pendientes.resize(4, 0);
		num_acciones = 0;
		decision = -1;
		contador = 0;
		baneados.resize(16, false);
		mapaFrecuencias.resize(size, vector<int>(size, 0));
		baneadas.resize(size, vector<bool>(size, false));

		tiempo_espera = tiempo_recarga = 0;
		necesito_recargar = false;
		buscando_recarga = false;
		hayPlan = false;
		plan.clear();
		index = 0;
		// planBases.clear();
		// hayPlanBases = false;
		// index = indexBases = 0;
		objetivo_anterior = {0,0};
		conozco_bases = false;
		conozco_zapatillas = false;
		plan_inseguro = false;
		energia_necesaria = 0;
		ejecutando_plan_objetivo = false;
		instantes = 0;
		opcion_replanificacion = NO_PERMISIVA;
		opcion_busqueda = NO_PERMISIVA;
		camino_duro = false;
		modo_tonto = false;
		hay_plan_restrictivo = true;
		recargando = false;
		desconocidas_busqueda = '?';
		veo_base = false;
		//UMBRAL_ENERGIA = size*sqrt(size);
	}

	/**
	 * @brief Constructor para los niveles 2,3
	 * @param mapaR Mapa de terreno
	 * @param mapaC Mapa de alturas
	 */
	ComportamientoRescatador(std::vector<std::vector<unsigned char>> mapaR, std::vector<std::vector<unsigned char>> mapaC) : Comportamiento(mapaR, mapaC)
	{
		// Inicializar Variables de Estado Niveles 2,3
		hayPlan = false;
		index = 0;
		plan.clear();
		opcion_busqueda = PERMISIVA;
	}

	/**
	 * @brief Constructor de copia
	 * @param comport Comportamiento a copiar
	 */
	ComportamientoRescatador(const ComportamientoRescatador &comport) : Comportamiento(comport) {}

	/**
	 * @brief Destructor
	 */
	~ComportamientoRescatador() {}

	/**
	 * @brief Función principal del comportamiento
	 * @param sensores Sensores del agente
	 * @return Acción a realizar
	 */
	Action think(Sensores sensores);

	/**
	 * @brief Interactúa con el entorno
	 * @param accion Acción a realizar
	 * @param valor Valor de la acción
	 * @return Resultado de la interacción
	 */
	int interact(Action accion, int valor);

	/**
	 * @brief Convierte la posición de la casilla en el mapa a la posición de la casilla en el mapa
	 * @param i Posición de la casilla en el mapa
	 * @param rumbo Dirección del sensor
	 * @param orig Posición del agente
	 * @return Posición de la casilla en el mapa
	 */
	pair<int,int> VtoM(int i, Orientacion rumbo, const pair<int, int> & orig);

	/**
	 * @brief Coloca el sensor en el mapa (lo "pinta")
	 * @param sensores Sensores del agente
	 */
	void SituarSensorEnMapa(const Sensores & sensores);

	/**
	 * @brief Comprueba si tengo tareas pendientes
	 * @param sensores Sensores del agente
	 * @param action Acción a realizar
	 * @return Si tengo tareas pendientes
	 */
	bool TengoTareasPendientes(const Sensores & sensores, Action & action);

	/**
	 * @brief Comprueba si la casilla es viable por altura
	 * @param dif Diferencia de altura
	 * @param zap Si el agente tiene zapatillas
	 * @return Si la casilla es viable por altura
	 */
	bool ViablePorAltura(int dif, bool zap);
	
	/**
	 * @brief Obtiene las casillas interesantes visibles por el agente
	 * @param sensores Sensores del agente
	 * @param accesible Casillas accesibles
	 * @param transitable Casillas transitables
	 * @param is_interesting Casillas interesantes. Parámetro de salida
	 * @param casillas_interesantes Casillas interesantes. Parámetro de salida
	 * @param zap Si el agente tiene zapatillas
	 * 
	 * @note Las casillas seleccionadas son enteros relativos a la posición del agente:
	 * 
	 * 		9	10	11	12	13	14	15
	 * 			4	5	6	7	8
	 * 				1	2	3
	 * 					^		
	 */
	void CasillasInteresantes (const Sensores & sensores, const vector<bool> & accesible, const vector<bool> & transitable,
		vector<bool> & is_interesting, vector<int> & casillas_interesantes, bool zap);

	/**
	 * @brief Obtiene las casillas interesantes alrededor del agente
	 * @param orig Posición del agente
	 * @param accesible Casillas accesibles
	 * @param transitable Casillas transitables
	 * @param is_interesting Casillas interesantes. Parámetro de salida
	 * @param casillas_interesantes Casillas interesantes. Parámetro de salida
	 * @param zap Si el agente tiene zapatillas
	 * 
	 * @note Las casillas seleccionadas son enteros relativos a la posición del agente:
	 * 
	 * 		15		8		9
	 * 			7	0	1	
	 * 		14	6	^	2	10
	 * 			5	4	3	
	 * 		13		12		11
	 */
	void CasillasInteresantesAllAround(const pair<int,int> & orig, const vector<bool> & accesible, 
		const vector<bool> & transitable, vector<bool> & is_interesting, vector<int> & casillas_interesantes, bool zap);

	void CasillasInteresantesAllAround_LVL1(const pair<int,int> & orig, const vector<bool> & accesible, 
		const vector<bool> & transitable, vector<bool> & is_interesting, vector<int> & casillas_interesantes, bool zap);

	/**
	 * @brief Selecciona la casilla más interesante
	 * @param sensores Sensores del agente
	 * @param casillas_interesantes Casillas interesantes
	 * @param is_interesting Casillas interesantes
	 * @param zap Si el agente tiene zapatillas
	 * @return Casilla seleccionada
	 * 
	 * @note La casilla seleccionada es un entero relativo a la posición del agente:
	 * 
	 * 		9	10	11	12	13	14	15
	 * 			4	5	6	7	8
	 * 				1	2	3
	 * 					^	
	 */
	int SelectCasilla (const Sensores & sensores, const vector<int> & casillas_interesantes, 
			const vector<bool> & is_interesting);
	
	/**
	 * @brief Selecciona la casilla más interesante de entre las que puede ir con un giro y un avance
	 * @param orig Posición del agente
	 * @param casillas_interesantes Casillas interesantes
	 * @param is_interesting Casillas interesantes
	 * @param rumbo Dirección en la que está mirando el agente
	 * @return Casilla seleccionada
	 * 
	 * @note La casilla seleccionada es un entero relativo a la posición del agente:
	 * 
	 * 		15		8		9
	 * 			7	0	1	
	 * 		14	6	^	2	10
	 * 			5	4	3	
	 * 		13		12		11
	 * 		
	 */
	int SelectCasillaAllAround (const pair<int,int> & orig, const vector<int> & casillas_interesantes, 
		const vector<bool> & is_interesting, Orientacion rumbo, bool zap);

	int SelectCasillaAllAround_LVL1 (const pair<int,int> & orig, const vector<int> & casillas_interesantes, 
		const vector<bool> & is_interesting, Orientacion rumbo, bool zap);
		
	
	/**
	 * @brief Selecciona la acción a realizar
	 * @param decision Casilla seleccionada
	 * @param rumbo Dirección del sensor
	 * @return Acción a realizar
	 * 
	 * @note La casilla seleccionada @p decision es un entero relativo a la posición del agente:
	 * 
	 * 		15		8		9
	 * 			7	0	1	
	 * 		14	6	^	2	10
	 * 			5	4	3	
	 * 		13		12		11
	 */
	Action SelectAction(int decision, Orientacion rumbo);

	void OndaDeCalor(int f, int c);

	bool IsSolution(const EstadoR &estado, const EstadoR &final);
	bool CasillaAccesibleRescatador(const EstadoR &st, int impulso, int nivel);
	bool CasillaAccesibleRescatadorOpciones(const EstadoR &st, int impulso, int opcion);
	EstadoR NextCasillaRescatador(const EstadoR &st, int impulso);
	EstadoR applyR(Action accion, const EstadoR &st, bool &accesible, const Sensores & sensores);
	void VisualizaPlan(const EstadoR &st, const vector<Action> &plan);
	void PintaPlan(const vector<Action> &plan, bool zap);
	void AnularMatrizA(vector<vector<unsigned char>> &m);
	int Heuristica(const EstadoR &st, const EstadoR &final);
	int CalcularCoste(Action accion, const EstadoR &st);

	vector<Action> Dijkstra(const EstadoR &inicio, const EstadoR &final, const Sensores & sensores);
	NodoR A_estrella(const EstadoR &inicio, const EstadoR &final, const Sensores & sensores);
	Action PlanRecarga(Sensores sensores);

	// pre : conozco_bases == true
	NodoR BuscaCasillas(const EstadoR &inicio, const Sensores & sensores, const vector<char>& casilla_objetivo);
	pair<int,int> CasillaMasFavorable(int f, int c);
	void BuscaObjetivo(Sensores sensores, int f = -1, int c = -1);
	bool EncontreCasilla(const EstadoR &st, vector<char> casillas_objetivo);
	bool HayQueReplanificar(const Sensores & sensores, const Action & accion);
	int SelectCasillaAllAround_LVL4(Sensores sensores, const vector<int> & casillas_interesantes, 
		const vector<bool> & is_interesting);
	Action Exploracion(Sensores sensores);
	void AnularPlan();
	Action UltimoRecurso(Sensores sensores);
	bool PlanCasillas(Sensores sensores, const vector<char> & casillas);
	Action LlamarAuxiliar(Sensores sensores);
	Action OrganizaPlan(Sensores sensores);
	bool CaminoDuro(const EstadoR & inicio, const EstadoR & fin, const vector<Action> & plan) ;
	void DecideOpcionReplanificacion(const Sensores & sensores);
	void AnularRecarga();

	Action ComportamientoRescatadorNivel_0(Sensores sensores);
	Action ComportamientoRescatadorNivel_1(Sensores sensores);
	Action ComportamientoRescatadorNivel_2(Sensores sensores);
	Action ComportamientoRescatadorNivel_3(Sensores sensores);
	Action ComportamientoRescatadorNivel_4(Sensores sensores);

private:
	// Variables de Estado

	// Variables de estado propuestas:

	Action last_action;
	bool tiene_zapatillas;
	int decision;
	vector<int> acciones_pendientes; // [TURN_L, TURN_SR, WALK, RUN]
	int num_acciones;
	int contador;
	vector<bool> baneados;
	vector<vector<int>> mapaFrecuencias;
	
	vector<Action> plan;
	int index;
	bool hayPlan;

	int tiempo_espera;
	int tiempo_recarga;
	bool necesito_recargar;
	bool buscando_recarga;
	bool conozco_bases;
	bool conozco_zapatillas;
	pair<int,int> objetivo_anterior;
	// bool hayPlanBases;
	// vector<Action> planBases;
	// int indexBases;
	bool plan_inseguro;
	bool ejecutando_plan_objetivo;
	vector<vector<bool>> baneadas;
	int energia_necesaria;
	int comportamiento;

	const int UMBRAL_TIEMPO = 500;
	const int RADIO_INMERSION = 5;
	//int UMBRAL_ENERGIA;
	int instantes;
	int opcion_replanificacion;
	int opcion_busqueda;
	bool camino_duro;
	bool modo_tonto;
	bool hay_plan_restrictivo;
	bool recargando;
	char desconocidas_busqueda;
	bool veo_base;
};

#endif
