#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <random>

using namespace std;

float euclidiana(const vector<float>& v1, const vector<float>& v2) {
    float suma = 0;
    for (int i = 0; i < v1.size(); i++) {
        suma += pow(v1[i] - v2[i], 2);
    }
    return sqrt(suma);
}

int main() {

    int n = 100;
    int d = 5000;

    vector<vector<float>> vc;

    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<> dis(0.0, 1.0);

    for (int i = 0; i < n; i++) {
        vector<float> punto;
        for (int j = 0; j < d; j++) {
            punto.push_back(dis(gen));
        }
        vc.push_back(punto);
    }

    ofstream archivo("100x5000.csv");

    for (int i = 0; i < vc.size(); i++) {
        for (int j = 0; j < vc.size(); j++) {
            float dist = euclidiana(vc[i], vc[j]);
            archivo << dist << "\n";
        }
    }

    archivo.close();

    cout << "Archivo generado: distancias.csv" << endl;

}
