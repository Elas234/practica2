#ifndef COMPORTAMIENTOAUXILIAR_H
#define COMPORTAMIENTOAUXILIAR_H

#include <chrono>
#include <time.h>
#include <thread>
#include <list>
#include <map>
#include "comportamientos/comportamiento.hpp"

#define ESTANDAR 0
#define PERMISIVA 1
#define NO_PERMISIVA 2

struct EstadoA
{
	int f;
	int c;
	int brujula;
	bool zapatillas;

	bool operator==(const EstadoA &st) const
	{
		return f == st.f && c == st.c && brujula == st.brujula && zapatillas == st.zapatillas;
	}
	bool operator<(const EstadoA &st) const
	{
		if (f < st.f) return true;
		else if (f == st.f && c < st.c) return true;
		else if (f == st.f && c == st.c && brujula < st.brujula) return true;
		else if (f == st.f && c == st.c && brujula == st.brujula && zapatillas < st.zapatillas) return true;
		else return false;
	}
	bool operator>(const EstadoA &st) const
	{
		return !(*this < st) && !(*this == st);
	}
};

struct NodoA
{
	EstadoA estado;
	vector<Action> secuencia;
	int g;
	int h;
	bool path_inseguro;

	NodoA() : g(0), h(0) , path_inseguro(false) {}

	bool operator==(const NodoA &nodo) const
	{
		return estado == nodo.estado;
	}

	// bool operator<(const NodoA &node) const{
	// 	if (g+h < node.g+node.h) return true;
	// 	else if(g+h > node.g+node.h) return false;
	// 	// else if(g < node.g) return true;
	// 	else if (estado.f < node.estado.f) return true;
	// 	else if (estado.f == node.estado.f && estado.c < node.estado.c) return true;
	// 	else if (estado.f == node.estado.f && estado.c == node.estado.c && estado.brujula < node.estado.brujula) return true;
	// 	else if (estado.f == node.estado.f && estado.c == node.estado.c && estado.brujula == node.estado.brujula && estado.zapatillas < node.estado.zapatillas) return true;
	// 	else return false;
	// }
	bool operator<(const NodoA &node) const {
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

	bool operator>(const NodoA &node) const {
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

class ComportamientoAuxiliar : public Comportamiento
{

public:
	/**
	 * @brief Constructor por defecto para los niveles 0,1,4
	 * @param size Tamaño del mapa
	 */
	ComportamientoAuxiliar(unsigned int size = 0) : Comportamiento(size)
	{
		// Inicializar Variables de Estado Niveles 0,1,4

		last_action = IDLE;
		tiene_zapatillas = false;
		acciones_pendientes.resize(2, 0);
		num_acciones = 0;
		contador = 0;
		baneados.resize(16, false);
		decision = -1;
		mapaFrecuencias.resize(size, vector<int>(size, 0));
		baneadas.resize(size, vector<bool>(size, false));

		necesito_recargar = false;
		buscando_recarga = false;

		index = 0;

		hayPlan = false;
		plan.clear();
		recargar = false;
		conozco_bases = false;
		conozco_zapatillas = false;
		objetivo_anterior = {0,0};
		conozco_bases = false;
		conozco_zapatillas = false;
		plan_inseguro = false;
		energia_necesaria = 0;
		ejecutando_plan_objetivo = false;
		instantes = 0;
		opcion_replanificacion = NO_PERMISIVA;
		opcion_busqueda = NO_PERMISIVA;
		desconocidas_busqueda = 'A';
		desconocidas_replanificacion = 'A';
		camino_duro = false;
		modo_tonto = false;
		hay_plan_restrictivo = true;
		recargando = false;
		estoy_cerca_objetivo = false;
		veo_base = false;
		pos_base = pos_zapatillas = {-1,-1};
		ocioso = true;


		// hayPlanBases = false;
		// indexBases = 0;


	}
	/**
	 * @brief Constructor para los niveles 2,3
	 * @param mapaR Mapa de terreno
	 * @param mapaC Mapa de alturas
	 */
	ComportamientoAuxiliar(std::vector<std::vector<unsigned char>> mapaR, std::vector<std::vector<unsigned char>> mapaC) : Comportamiento(mapaR, mapaC)
	{
		// Inicializar Variables de Estado Niveles 2,3
		hayPlan = false;
		index = 0;
		plan.clear();
	}
	/**
	 * @brief Constructor de copia
	 * @param comport Comportamiento a copiar
	 */
	ComportamientoAuxiliar(const ComportamientoAuxiliar &comport) : Comportamiento(comport) {}

	/**
	 * @brief Destructor
	 */
	~ComportamientoAuxiliar() {}

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

	/********************************** NIVEL 0 ***************************************/

	/**
	 * @brief Convierte la posición de la casilla en el vector sensor a la posición de la casilla en el mapa
	 * @param i Posición de la casilla en el sensor
	 * @param rumbo Dirección del sensor
	 * @param orig Posición del agente
	 * @return Posición de la casilla en el mapa
	 */
	pair<int, int> VtoM(int i, Orientacion rumbo, const pair<int, int> & orig);

	/**
	 * @brief Coloca el sensor en el mapa (lo "pinta")
	 * @param sensores Sensores del agente
	 */
	void SituarSensorEnMapa(const Sensores & sensores);

		/**
	 * @brief Determina si tengo tareas pendientes
	 * @param sensores Sensores del agente
	 * @param action Acción a realizar
	 * @return true si tengo tareas pendientes, false en caso contrario
	 */
	bool TengoTareasPendientes(const Sensores & sensores, Action & action);

	/**
	 * @brief Comprueba si la casilla es viable por altura
	 * @param dif Diferencia de altura
	 * @return true si la casilla es viable por altura, false en caso contrario
	 */
	bool ViablePorAltura(int dif);

	/**
	 * @brief Selecciona las casillas interesantes visibles por el agente
	 * @param sensores Sensores del agente
	 * @param accesible Casillas accesibles
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
	void CasillasInteresantes(const Sensores & sensores, const vector<bool> & accesible, 
		vector<bool> & is_interesting, vector<int> & casillas_interesantes);

	/**
	 * @brief Selecciona las casillas interesantes alrededor del agente
	 * @param orig Posición del agente
	 * @param accesible Casillas accesibles
	 * @param is_interesting Casillas interesantes. Parámetro de salida
	 * @param casillas_interesantes Casillas interesantes. Parámetro de salida
	 * @param zap Si el agente tiene zapatillas
	 * 
	 * @note Las casillas seleccionadas son enteros relativos a la posición del agente:
	 * 
	 * 		7	0	1
	 * 		6	^	2
	 * 		5	4	3
	 */
	void CasillasInteresantesAllAround(const pair<int,int> & orig, const vector<bool> & accesible, 
		vector<bool> & is_interesting, vector<int> & casillas_interesantes);

	/**
	 * @brief Selecciona las casillas interesantes alrededor del agente
	 * @param orig Posición del agente
	 * @param accesible Casillas accesibles
	 * @param is_interesting Casillas interesantes. Parámetro de salida
	 * @param casillas_interesantes Casillas interesantes. Parámetro de salida
	 * @param zap Si el agente tiene zapatillas
	 * 
	 * @note Las casillas seleccionadas son enteros relativos a la posición del agente:
	 * 
	 * 		7	0	1
	 * 		6	^	2
	 * 		5	4	3
	 */
	void CasillasInteresantesAllAround_LVL1 (const pair<int,int> & orig, const vector<bool> & accesible, 
		vector<bool> & is_interesting, vector<int> & casillas_interesantes);

	/**
	 * @brief Selecciona la casilla más interesante de entre las que puede ir con un giro y un avance
	 * @param sensores Sensores del agente
	 * @param casillas_interesantes Casillas interesantes
	 * @param is_interesting Casillas interesantes
	 * @return Casilla seleccionada
	 * 
	 * @note La casilla seleccionada es un entero relativo a la posición del agente:
	 * 
	 * 		9	10	11	12	13	14	15
	 * 			4	5	6	7	8
	 * 				1	2	3
	 * 					^	
	 */ 
	int SelectCasilla(const Sensores & sensores, const vector<int> & casillas_interesantes, 
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
	 * 		7	0	1	
	 * 		6	^	2
	 * 		5	4	3	
	 */
	int SelectCasillaAllAround(const pair<int,int> & orig, const vector<int> & casillas_interesantes, 
		const vector<bool> & is_interesting, Orientacion rumbo);

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
	 * 		7	0	1	
	 * 		6	^	2
	 * 		5	4	3	
	 */
	int SelectCasillaAllAround_LVL1(const pair<int,int> & orig, const vector<int> & casillas_interesantes, 
		const vector<bool> & is_interesting, Orientacion rumbo);
	
	/**
	 * @brief Selecciona la acción a realizar en función de la casilla a la que quiero ir
	 * @param decision Casilla a la que quiero ir
	 * @param rumbo Dirección en la que está mirando el agente
	 * @return Acción a realizar
	 */
	Action SelectAction(int decision, Orientacion rumbo);	
	
	void OndaDeCalor(int f, int c);

	/***************************** NIVEL E ********************************************/

	vector<Action> AnchuraAuxiliar(const EstadoA &inicio, const EstadoA &final, const Sensores & sensores);

	bool IsSolution(const EstadoA &estado, const EstadoA &final);
	bool CasillaAccesibleAuxiliar(const EstadoA &st, int nivel);
	EstadoA NextCasillaAuxiliar(const EstadoA &st);
	EstadoA applyA(Action accion, const EstadoA &st, bool &accesible, const Sensores & sensores);
	bool Find(const NodoA &st, const list<NodoA> &lista);
	void VisualizaPlan(const EstadoA &st, const vector<Action> &plan);
	void PintaPlan(const vector<Action> &plan, bool zap);
	void AnularMatrizA(vector<vector<unsigned char>> &m);
	int Heuristica(const EstadoA &st, const EstadoA &final);
	int CalcularCoste(Action accion, const EstadoA &st);
	NodoA A_Estrella(const EstadoA &inicio, const EstadoA &final, const Sensores & sensores);
	vector<Action> DijkstraPuestosBase(const EstadoA &inicio, const Sensores & sensores);

	bool HayQueReplanificar(const Sensores & sensores, const Action & accion);
	void AnularRecarga();
	void AnularPlan();
	Action PlanRecarga(Sensores sensores);
	NodoA BuscaCasillas(const EstadoA &inicio, const Sensores & sensores, const vector<char>& casillas_objetivo);
	bool EncontreCasilla(const EstadoA &st, vector<char> casillas_objetivo);
	bool CaminoDuro(const EstadoA & inicio, const EstadoA & fin, const vector<Action> & plan);
	bool PlanCasillas(Sensores sensores, const vector<char> & casillas);
	Action UltimoRecurso(Sensores sensores);
	bool CasillaAccesibleAuxiliarOpciones(const EstadoA &st, int opcion = 0);
	Action OrganizaPlan(Sensores sensores);
	void DecideOpcionReplanificacion(const Sensores & sensores);
	void BuscaObjetivo(Sensores sensores, int f = -1, int c = -1);
	NodoA AproximacionObjetivo(const EstadoA &inicio, const EstadoA &final, const Sensores & sensores);

	Action ComportamientoAuxiliarNivel_0(Sensores sensores);
	Action ComportamientoAuxiliarNivel_1(Sensores sensores);
	Action ComportamientoAuxiliarNivel_2(Sensores sensores);
	Action ComportamientoAuxiliarNivel_3(Sensores sensores);
	Action ComportamientoAuxiliarNivel_4(Sensores sensores);

	Action ComportamientoAuxiliarNivel_E(Sensores sensores);


private:
	// Definir Variables de Estado

	// Variables de estado propuestas:

	Action last_action;
	bool tiene_zapatillas;
	vector<bool> baneados;
	int decision;
	int contador;
	vector<int> acciones_pendientes; // [TURN_SR, WALK]
	int num_acciones;
	vector< vector<int> > mapaFrecuencias;

	vector<Action> plan;
	int index;
	bool hayPlan;


	bool recargar;
	bool necesito_recargar;
	bool buscando_recarga;
	bool conozco_bases;
	bool conozco_zapatillas;
	vector<Action> planBases;
	// int indexBases;
	// bool hayPlanBases;

	// pair<int,int> objetivo_anterior;
	
	bool plan_inseguro;
	bool ejecutando_plan_objetivo;
	pair<int,int> objetivo_anterior;
	vector<vector<bool>> baneadas;
	int energia_necesaria;
	int comportamiento;

	const int UMBRAL_TIEMPO = 1;
	const int RADIO_INMERSION = 7;
	//int UMBRAL_ENERGIA;
	int instantes;
	int opcion_replanificacion;
	int opcion_busqueda;
	char desconocidas_replanificacion;
	char desconocidas_busqueda;
	bool camino_duro;
	bool modo_tonto;
	bool hay_plan_restrictivo;
	bool recargando;
	bool veo_base;
	pair<int,int> pos_base;
	pair<int,int> pos_zapatillas;
	bool ocioso;

	bool estoy_cerca_objetivo;

	
	
};

#endif
