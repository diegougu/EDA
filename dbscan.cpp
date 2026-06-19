#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>
#include <chrono>
#include <algorithm>
#include <string>

using namespace std;

struct Punto {
    vector<double> coordenadas;
};

double distanciaCuadrada(const Punto& a, const Punto& b) {
    double suma = 0.0;
    for (size_t i = 0; i < a.coordenadas.size(); i++) {
        double d = a.coordenadas[i] - b.coordenadas[i];
        suma += d * d;
    }
    return suma;
}

double distancia(const Punto& a, const Punto& b) {
    return sqrt(distanciaCuadrada(a, b));
}

vector<int> dbscanClasico(const vector<Punto>& puntos, double eps, int minPuntos) {
    int n = puntos.size();
    vector<int> indicesCluster(n, 0);
    int indiceActual = 0;
    double epsCuadrado = eps * eps;

    for (int p = 0; p < n; p++) {
        if (indicesCluster[p] != 0)
            continue;

        vector<int> porProcesar;
        porProcesar.push_back(p);
        indicesCluster[p] = -1;
        indiceActual = indiceActual + 1;

        size_t i = 0;
        while (i < porProcesar.size()) {
            int q = porProcesar[i];
            i++;

            vector<int> vecinos;
            for (int j = 0; j < n; j++) {
                if (distanciaCuadrada(puntos[q], puntos[j]) <= epsCuadrado)
                    vecinos.push_back(j);
            }

            if ((int)vecinos.size() < minPuntos)
                continue;

            indicesCluster[q] = indiceActual;

            for (int w : vecinos) {
                if (indicesCluster[w] <= 0) {
                    bool yaEsta = false;
                    for (int x : porProcesar) {
                        if (x == w) { yaEsta = true; break; }
                    }
                    if (!yaEsta)
                        porProcesar.push_back(w);
                }
            }
        }
    }
    return indicesCluster;
}

struct NodoKD {
    int indice;
    NodoKD* izquierda;
    NodoKD* derecha;
    int eje;
};

class KDTree {
public:
    KDTree(const vector<Punto>& puntos) {
        datos = puntos;
        dimension = puntos.empty() ? 0 : puntos[0].coordenadas.size();
        vector<int> indices(puntos.size());
        for (size_t i = 0; i < indices.size(); i++)
            indices[i] = i;
        raiz = construir(indices, 0);
    }

    ~KDTree() {
        liberar(raiz);
    }

    vector<int> puntosEnEsfera(const Punto& centro, double eps) {
        vector<int> resultado;
        double epsCuadrado = eps * eps;
        buscar(raiz, centro, eps, epsCuadrado, resultado);
        return resultado;
    }

private:
    vector<Punto> datos;
    NodoKD* raiz;
    int dimension;

    NodoKD* construir(vector<int>& indices, int profundidad) {
        if (indices.empty())
            return nullptr;

        int eje = profundidad % dimension;
        sort(indices.begin(), indices.end(), [&](int a, int b) {
            return datos[a].coordenadas[eje] < datos[b].coordenadas[eje];
            });

        int medio = indices.size() / 2;
        NodoKD* nodo = new NodoKD();
        nodo->indice = indices[medio];
        nodo->eje = eje;

        vector<int> izquierda(indices.begin(), indices.begin() + medio);
        vector<int> derecha(indices.begin() + medio + 1, indices.end());

        nodo->izquierda = construir(izquierda, profundidad + 1);
        nodo->derecha = construir(derecha, profundidad + 1);
        return nodo;
    }

    void buscar(NodoKD* nodo, const Punto& centro, double eps, double epsCuadrado, vector<int>& resultado) {
        if (nodo == nullptr)
            return;

        if (distanciaCuadrada(datos[nodo->indice], centro) <= epsCuadrado)
            resultado.push_back(nodo->indice);

        int eje = nodo->eje;
        double diferencia = centro.coordenadas[eje] - datos[nodo->indice].coordenadas[eje];

        if (diferencia <= 0) {
            buscar(nodo->izquierda, centro, eps, epsCuadrado, resultado);
            if (diferencia * diferencia <= epsCuadrado)
                buscar(nodo->derecha, centro, eps, epsCuadrado, resultado);
        }
        else {
            buscar(nodo->derecha, centro, eps, epsCuadrado, resultado);
            if (diferencia * diferencia <= epsCuadrado)
                buscar(nodo->izquierda, centro, eps, epsCuadrado, resultado);
        }
    }

    void liberar(NodoKD* nodo) {
        if (nodo == nullptr)
            return;
        liberar(nodo->izquierda);
        liberar(nodo->derecha);
        delete nodo;
    }
};

vector<int> dbscanKDTree(const vector<Punto>& puntos, double eps, int minPuntos) {
    int n = puntos.size();
    vector<int> indicesCluster(n, 0);
    int indiceActual = 0;

    KDTree arbol(puntos);

    for (int p = 0; p < n; p++) {
        if (indicesCluster[p] != 0)
            continue;

        vector<int> porProcesar;
        porProcesar.push_back(p);
        indicesCluster[p] = -1;
        indiceActual = indiceActual + 1;

        size_t i = 0;
        while (i < porProcesar.size()) {
            int q = porProcesar[i];
            i++;

            vector<int> vecinos = arbol.puntosEnEsfera(puntos[q], eps);

            if ((int)vecinos.size() < minPuntos)
                continue;

            indicesCluster[q] = indiceActual;

            for (int w : vecinos) {
                if (indicesCluster[w] <= 0) {
                    bool yaEsta = false;
                    for (int x : porProcesar) {
                        if (x == w) { yaEsta = true; break; }
                    }
                    if (!yaEsta)
                        porProcesar.push_back(w);
                }
            }
        }
    }
    return indicesCluster;
}

vector<Punto> cargarDatos(const string& archivo) {
    vector<Punto> puntos;
    ifstream entrada(archivo);
    string linea;

    bool primeraLinea = true;
    while (getline(entrada, linea)) {
        if (linea.empty())
            continue;

        if (primeraLinea) {
            primeraLinea = false;
            bool esEncabezado = false;
            for (char c : linea) {
                if (isalpha(c) && c != 'e' && c != 'E') {
                    esEncabezado = true;
                    break;
                }
            }
            if (esEncabezado)
                continue;
        }

        Punto punto;
        stringstream ss(linea);
        string valor;
        while (getline(ss, valor, ',')) {
            try {
                punto.coordenadas.push_back(stod(valor));
            }
            catch (...) {
            }
        }
        if (!punto.coordenadas.empty())
            puntos.push_back(punto);
    }
    return puntos;
}

void guardarResultados(const string& archivo, const vector<Punto>& puntos, const vector<int>& etiquetas) {
    ofstream salida(archivo);
    int dim = puntos.empty() ? 0 : puntos[0].coordenadas.size();
    for (int i = 0; i < dim; i++)
        salida << "dim" << i << ",";
    salida << "cluster\n";

    for (size_t i = 0; i < puntos.size(); i++) {
        for (double c : puntos[i].coordenadas)
            salida << c << ",";
        salida << etiquetas[i] << "\n";
    }
}

int contarOutliers(const vector<int>& etiquetas) {
    int contador = 0;
    for (int e : etiquetas) {
        if (e == -1)
            contador++;
    }
    return contador;
}

int main(int argc, char* argv[]) {
    string archivoEntrada = "datos_alta_dim.csv";
    double eps = 1.2;
    int minPuntos = 8;

    if (argc >= 2) archivoEntrada = argv[1];
    if (argc >= 3) eps = stod(argv[2]);
    if (argc >= 4) minPuntos = stoi(argv[3]);

    vector<Punto> puntos = cargarDatos(archivoEntrada);
    cout << "Puntos cargados: " << puntos.size() << "\n";
    if (!puntos.empty())
        cout << "Dimensiones: " << puntos[0].coordenadas.size() << "\n";
    cout << "eps: " << eps << " minPuntos: " << minPuntos << "\n\n";

    auto inicioClasico = chrono::high_resolution_clock::now();
    vector<int> etiquetasClasico = dbscanClasico(puntos, eps, minPuntos);
    auto finClasico = chrono::high_resolution_clock::now();
    double tiempoClasico = chrono::duration<double, milli>(finClasico - inicioClasico).count();

    auto inicioKD = chrono::high_resolution_clock::now();
    vector<int> etiquetasKD = dbscanKDTree(puntos, eps, minPuntos);
    auto finKD = chrono::high_resolution_clock::now();
    double tiempoKD = chrono::duration<double, milli>(finKD - inicioKD).count();

    cout << "DBSCAN Clasico:\n";
    cout << "  Tiempo: " << tiempoClasico << " ms\n";
    cout << "  Outliers: " << contarOutliers(etiquetasClasico) << "\n\n";

    cout << "DBSCAN KDTree:\n";
    cout << "  Tiempo: " << tiempoKD << " ms\n";
    cout << "  Outliers: " << contarOutliers(etiquetasKD) << "\n\n";

    cout << "Aceleracion: " << tiempoClasico / tiempoKD << "x\n";

    guardarResultados("resultado_clasico2.csv", puntos, etiquetasClasico);
    guardarResultados("resultado_kdtree2.csv", puntos, etiquetasKD);
    cout << "\nResultados guardados en resultado_clasico2.csv y resultado_kdtree2.csv\n";

    return 0;
}
