#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <sstream>

using namespace std;

float euclidiana(vector<float>& v1, vector<float>& v2) {

    float suma = 0;

    for (int i = 0; i < v1.size(); i++) {
        suma += pow(v1[i] - v2[i], 2);
    }

    return sqrt(suma);
}

vector<float> leerCSV(string nombre) {

    ifstream archivo(nombre);
    vector<float> datos;
    string linea;

    while (getline(archivo, linea)) {
        datos.push_back(stof(linea));
    }

    return datos;
}

int main() {

    vector<float> archivo1 = leerCSV("1.csv");
    vector<float> archivo2 = leerCSV("2.csv");
    vector<float> archivo3 = leerCSV("3.csv");
    vector<float> archivo4 = leerCSV("4.csv");

    float d1 = euclidiana(archivo1, archivo2);
    float d2 = euclidiana(archivo1, archivo3);
    float d3 = euclidiana(archivo1, archivo4);

    cout << "Distancia 1.csv vs 2.csv: " << d1 << endl;
    cout << "Distancia 1.csv vs 3.csv: " << d2 << endl;
    cout << "Distancia 1.csv vs 4.csv: " << d3 << endl;

    float menor = d1;
    string archivoMasSimilar = "2.csv";

    if (d2 < menor) {
        menor = d2;
        archivoMasSimilar = "3.csv";
    }

    if (d3 < menor) {
        menor = d3;
        archivoMasSimilar = "4.csv";
    }

    cout << "El archivo mas similar a 1.csv es: " << archivoMasSimilar << " y con distancia de: " << menor << endl;

}
