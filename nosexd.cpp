#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <limits>
#include <iomanip>
using namespace std;

// =============================================================
//  CLASES DADAS POR EL ENUNCIADO
// =============================================================

class Rect {
public:
    float x_min, x_max, y_min, y_max;

    Rect(float x1 = 0, float x2 = 0, float y1 = 0, float y2 = 0)
        : x_min(x1), x_max(x2), y_min(y1), y_max(y2) {}

    float area() const {
        return (x_max - x_min) * (y_max - y_min);
    }

    // suma de longitudes de los lados (usado en Split → ChooseSplitAxis)
    float margin() const {
        return 2.0f * ((x_max - x_min) + (y_max - y_min));
    }

    // centro del rectángulo (usado en ReInsert → RI1)
    float centerX() const { return (x_min + x_max) / 2.0f; }
    float centerY() const { return (y_min + y_max) / 2.0f; }

    // área de intersección con otro rect (usado en Split → ChooseSplitIndex)
    float overlapWith(const Rect& o) const {
        float ox = max(0.0f, min(x_max, o.x_max) - max(x_min, o.x_min));
        float oy = max(0.0f, min(y_max, o.y_max) - max(y_min, o.y_min));
        return ox * oy;
    }

    // MBR que engloba this y o
    Rect combine(const Rect& o) const {
        return Rect(min(x_min, o.x_min), max(x_max, o.x_max),
            min(y_min, o.y_min), max(y_max, o.y_max));
    }

    // cuánto crece el área al combinar con o (usado en ChooseSubTree)
    float enlargement(const Rect& o) const {
        return combine(o).area() - area();
    }

    void print() const {
        cout << fixed << setprecision(1)
            << "[x:" << x_min << "-" << x_max
            << " y:" << y_min << "-" << y_max << "]";
    }
};

class RStarNode {
public:
    vector<Rect> data;  // rectángulos contenidos en este nodo
    string       name;  // nombre para identificarlo en el main

    RStarNode(string n = "") : name(n) {}

    void print() const {
        cout << "Nodo \"" << name << "\" (" << data.size() << " rects):\n";
        for (const auto& r : data) {
            cout << "  "; r.print(); cout << "\n";
        }
    }
};

// =============================================================
//  PARÁMETROS DEL R*-TREE  (paper: m ≈ 40% de M, p ≈ 30% de M)
// =============================================================
const int M = 4;
const int m = max(1, (int)ceil(0.4 * M));
const int P = max(1, (int)ceil(0.3 * M));

// =============================================================
//  CLASE RStar
// =============================================================
class RStar {
public:
    // nodos del árbol (simplificado para el main: un vector plano)
    vector<RStarNode*> nodes;
    RStarNode* root;

    RStar() {
        root = new RStarNode("root");
        nodes.push_back(root);
    }

    // -- dadas por el enunciado --
    // ChooseSubTree: devuelve el nodo con menor enlargement para insertar rect
    RStarNode* ChooseSubTree(Rect input) {
        RStarNode* best = root;
        float      bestEnl = numeric_limits<float>::max();
        for (auto* node : nodes) {
            Rect mbr = boundingRect(node->data);
            float enl = mbr.enlargement(input);
            if (enl < bestEnl) { bestEnl = enl; best = node; }
        }
        return best;
    }

    // Delete: borra el rectángulo del nodo que lo contiene
    void Delete(Rect a) {
        for (auto* node : nodes) {
            auto& d = node->data;
            auto it = find_if(d.begin(), d.end(), [&](const Rect& r) {
                return r.x_min == a.x_min && r.x_max == a.x_max &&
                r.y_min == a.y_min && r.y_max == a.y_max;
                });
            if (it != d.end()) { d.erase(it); return; }
        }
    }

    // Find: devuelve el nodo donde está el rectángulo
    RStarNode* Find(Rect a) {
        for (auto* node : nodes) {
            for (const auto& r : node->data)
                if (r.x_min == a.x_min && r.x_max == a.x_max &&
                    r.y_min == a.y_min && r.y_max == a.y_max)
                    return node;
        }
        return nullptr;
    }

    // -- función pedida en el examen --
    void OverflowTreatment(RStarNode* node);

    // imprime todos los nodos
    void printAll() const {
        cout << "\n--- Estado del arbol (" << nodes.size() << " nodos) ---\n";
        for (auto* n : nodes) n->print();
    }

private:
    // -- internas de OverflowTreatment --
    void ReInsert(RStarNode* node);
    void Split(RStarNode* node);

    // -- auxiliares de Split --
    float        sumOfMargins(vector<Rect> sorted);
    vector<Rect> chooseSplitAxis(vector<Rect> rects);
    int          chooseSplitIndex(const vector<Rect>& sorted);

    // -- auxiliar general --
    Rect boundingRect(const vector<Rect>& rects);

    // control de reinserción por nivel (se resetea en cada Insert externo)
    vector<bool> reinsertedLevels = vector<bool>(10, false);
public:
    void forceReinsertedLevel(int l) { reinsertedLevels[l] = true; }
private:
};

// =============================================================
//  OverflowTreatment  [R*]
//  paper: OT1 — primer overflow en el nivel → ReInsert
//                cualquier overflow posterior  → Split
// =============================================================


void RStar::OverflowTreatment(RStarNode* node) {

    // OT1: si no es la raíz y es el primer overflow en este nivel → ReInsert
    int  nodeLevel = 0;
    bool isRoot = (node == root);

    if (!isRoot && !reinsertedLevels[nodeLevel]) {
        // marcar este nivel como ya re-insertado para no volver a hacer ReInsert
        reinsertedLevels[nodeLevel] = true;
        ReInsert(node);
    }
    else {
        // overflow ya ocurrió antes en este nivel, o es la raíz → Split
        Split(node);
    }
}

// =============================================================
//  ReInsert  [R*]
//  paper: RI1 → RI4
//  Saca los P rectángulos más lejanos al centro del MBR del nodo
//  y los re-inserta en el árbol desde la raíz (close reinsert)
// =============================================================


void RStar::ReInsert(RStarNode* node) {

    Rect  mbr = boundingRect(node->data);
    float cx = mbr.centerX();
    float cy = mbr.centerY();

    sort(node->data.begin(), node->data.end(),
        [cx, cy](const Rect& a, const Rect& b) {
            float da = hypot(a.centerX() - cx, a.centerY() - cy);
    float db = hypot(b.centerX() - cx, b.centerY() - cy);
    return da > db;
        });

    vector<Rect> toReinsert(node->data.begin(),
        node->data.begin() + P);
    node->data.erase(node->data.begin(),
        node->data.begin() + P);


    for (int i = (int)toReinsert.size() - 1; i >= 0; i--) {
        RStarNode* target = ChooseSubTree(toReinsert[i]);
        target->data.push_back(toReinsert[i]);
         
        if ((int)target->data.size() > M)
            OverflowTreatment(target);
    }
}

// =============================================================
//  Split  [R*]
//  paper: S1 → S3
//  Elige el mejor eje y la mejor distribución, luego parte el nodo
// =============================================================


void RStar::Split(RStarNode* node) {

    vector<Rect> sorted = chooseSplitAxis(node->data); 

    int splitIdx = chooseSplitIndex(sorted);

    RStarNode* sibling = new RStarNode(node->name + "'"); //xd
    sibling->data.assign(sorted.begin() + splitIdx, sorted.end());
    node->data.assign(sorted.begin(), sorted.begin() + splitIdx);

    nodes.push_back(sibling); //xd

}

// =============================================================
//  sumOfMargins
//  auxiliar de Split → ChooseSplitAxis (CSA1)
// =============================================================

float RStar::sumOfMargins(vector<Rect> sorted) {
    float S = 0;
    int distribution = M - 2 * m + 2;
    for (int k = 1; k <= distribution; k++) {
        int split = (m - 1) + k;
        vector<Rect>g1(sorted.begin(), sorted.begin() + split);
        vector<Rect>g2(sorted.begin() + split, sorted.end());
        S += boundingRect(g1).margin() + boundingRect(g2).margin();
    }
    return S;
}

float RStar::sumOfMargins(vector<Rect> sorted) {
    float S = 0;
    int   distributions = M - 2 * m + 2;
    for (int k = 1; k <= distributions; k++) {
        int split = (m - 1) + k;
        vector<Rect> g1(sorted.begin(), sorted.begin() + split);
        vector<Rect> g2(sorted.begin() + split, sorted.end());
        S += boundingRect(g1).margin() + boundingRect(g2).margin();
    }
    return S;
}

// =============================================================
//  chooseSplitAxis  [R*]
//  Split → S1: ChooseSplitAxis (CSA1, CSA2)
// =============================================================
vector<Rect> RStar::chooseSplitAxis(vector<Rect> rects) {

    vector<Rect> byMinX = rects;
    sort(byMinX.begin(), byMinX.end(),
        [](const Rect& a, const Rect& b) { return a.x_min < b.x_min; });

    vector<Rect> byMaxX = rects;
    sort(byMaxX.begin(), byMaxX.end(),
        [](const Rect& a, const Rect& b) { return a.x_max < b.x_max; });

    vector<Rect> byMinY = rects;
    sort(byMinY.begin(), byMinY.end(),
        [](const Rect& a, const Rect& b) { return a.y_min < b.y_min; });

    vector<Rect> byMaxY = rects;
    sort(byMaxY.begin(), byMaxY.end(),
        [](const Rect& a, const Rect& b) { return a.y_max < b.y_max; });

    float SX = sumOfMargins(byMinX) + sumOfMargins(byMaxX);
    float SY = sumOfMargins(byMinY) + sumOfMargins(byMaxY);

    if (SX <= SY)
        return (sumOfMargins(byMinX) <= sumOfMargins(byMaxX)) ? byMinX : byMaxX;
    else
        return (sumOfMargins(byMinY) <= sumOfMargins(byMaxY)) ? byMinY : byMaxY;
}

// =============================================================
//  chooseSplitIndex  [R*]
//  Split → S2: ChooseSplitIndex (CSI1)
// =============================================================
int RStar::chooseSplitIndex(const vector<Rect>& sorted) {
    float bestOverlap = numeric_limits<float>::max();
    float bestArea = numeric_limits<float>::max();
    int   bestK = m;

    int distributions = M - 2 * m + 2;
    for (int k = 1; k <= distributions; k++) {
        int split = (m - 1) + k;
        vector<Rect> g1(sorted.begin(), sorted.begin() + split);
        vector<Rect> g2(sorted.begin() + split, sorted.end());

        float ov = boundingRect(g1).overlapWith(boundingRect(g2));
        float area = boundingRect(g1).area() + boundingRect(g2).area();

        if (ov < bestOverlap || (ov == bestOverlap && area < bestArea)) {
            bestOverlap = ov;
            bestArea = area;
            bestK = split;
        }
    }
    return bestK;
}

// =============================================================
//  boundingRect
//  auxiliar general — calcula el MBR de un conjunto de Rect
// =============================================================
Rect RStar::boundingRect(const vector<Rect>& rects) {
    float x1 = numeric_limits<float>::max();
    float x2 = -numeric_limits<float>::max();
    float y1 = numeric_limits<float>::max();
    float y2 = -numeric_limits<float>::max();
    for (const auto& r : rects) {
        x1 = min(x1, r.x_min); x2 = max(x2, r.x_max);
        y1 = min(y1, r.y_min); y2 = max(y2, r.y_max);
    }
    return Rect(x1, x2, y1, y2);
}

// =============================================================
//  MAIN — prueba OverflowTreatment (Split y ReInsert)
// =============================================================
int main() {
    cout << "=== Prueba de OverflowTreatment (M=" << M
        << ", m=" << m << ", p=" << P << ") ===\n";

    // ----------------------------------------------------------
    // PRUEBA 1: Split
    // Llenamos el nodo raíz con M+1 rectángulos para forzar overflow.
    // Como es la raíz, OT1 va directo a Split (nunca ReInsert en raíz).
    // ----------------------------------------------------------
    cout << "\n--- PRUEBA 1: overflow en la raiz → debe hacer Split ---\n";
    {
        RStar tree;
        tree.root->data = {
            Rect(0,2, 0,2),
            Rect(3,5, 0,2),
            Rect(6,8, 0,2),
            Rect(9,11,0,2),
            Rect(1,3, 3,5)   // 5to rect → overflow (M+1)
        };

        cout << "Antes:\n"; tree.root->print();
        cout << "Llamando OverflowTreatment...\n";
        tree.OverflowTreatment(tree.root);
        tree.printAll();
    }

    // ----------------------------------------------------------
    // PRUEBA 2: ReInsert
    // Creamos un nodo no-raíz con M+1 rectángulos.
    // Es el primer overflow en ese nivel → debe hacer ReInsert.
    // ----------------------------------------------------------
    cout << "\n--- PRUEBA 2: overflow en nodo hoja → debe hacer ReInsert ---\n";
    {
        RStar tree;

        // nodo hijo (no es la raíz)
        RStarNode* hijo = new RStarNode("hijo");
        hijo->data = {
            Rect(0, 2, 0, 2),
            Rect(1, 3, 1, 3),
            Rect(8,10, 8,10),
            Rect(9,11, 9,11),
            Rect(5, 7, 5, 7)   // 5to rect → overflow (M+1)
        };
        tree.nodes.push_back(hijo);

        cout << "Antes:\n"; hijo->print();
        cout << "Llamando OverflowTreatment...\n";
        tree.OverflowTreatment(hijo);
        tree.printAll();
    }

    // ----------------------------------------------------------
    // PRUEBA 3: segundo overflow en el mismo nivel → Split aunque no sea raíz
    // ----------------------------------------------------------
    cout << "\n--- PRUEBA 3: segundo overflow en mismo nivel → Split ---\n";
    {
        RStar tree;

        RStarNode* hijo = new RStarNode("hijo");
        hijo->data = {
            Rect(0,2,0,2), Rect(1,3,1,3), Rect(8,10,8,10),
            Rect(9,11,9,11), Rect(5,7,5,7)
        };
        tree.nodes.push_back(hijo);

        // marcar el nivel como ya re-insertado → forzar Split en vez de ReInsert
        tree.forceReinsertedLevel(0);

        cout << "Antes:\n"; hijo->print();
        cout << "Llamando OverflowTreatment (nivel ya re-insertado)...\n";
        tree.OverflowTreatment(hijo);
        tree.printAll();
    }

    return 0;
}
