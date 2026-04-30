#include <iostream>
#include <vector>
#include <algorithm>
#include <limits>
#include <iomanip>
using namespace std;

// ─── Estructuras ─────────────────────────────────────────────────────────────
struct Rect {
    double minX, minY, maxX, maxY;
    Rect(double minX = 0, double minY = 0, double maxX = 0, double maxY = 0)
        : minX(minX), minY(minY), maxX(maxX), maxY(maxY) {}

    bool overlaps(const Rect& other) const {
        return !(minX > other.maxX || maxX < other.minX ||
            minY > other.maxY || maxY < other.minY);
    }
    bool contains(const Rect& other) const {
        return (minX <= other.minX && maxX >= other.maxX &&
            minY <= other.minY && maxY >= other.maxY);
    }
    Rect combine(const Rect& other) const {
        return Rect(min(minX, other.minX), min(minY, other.minY),
            max(maxX, other.maxX), max(maxY, other.maxY));
    }
    double area() const { return (maxX - minX) * (maxY - minY); }

    bool equals(const Rect& other) const {
        return (minX == other.minX && minY == other.minY &&
            maxX == other.maxX && maxY == other.maxY);
    }
};

const int MAX_ENTRIES = 4;
const int MIN_ENTRIES = 2;

struct Node;

struct Entry {
    Rect  rect;
    Node* child;
    Entry() : rect(Rect()), child(nullptr) {}
    Entry(const Rect& r, Node* c = nullptr) : rect(r), child(c) {}
};

struct Node {
    bool isLeaf;
    vector<Entry> entries;
    Node(bool l = true) : isLeaf(l) {}
};

// ─── RTree ────────────────────────────────────────────────────────────────────
class RTree {
    Node* root;


    int chooseleaf(Node* node, const Rect& rect) {
        double minEnlargement = numeric_limits<double>::max();

        double minArea = numeric_limits<double>::max();

        int bestIdx = 0;
        for (int i = 0; i < (int)node->entries.size(); i++) {
            Rect combined = node->entries[i].rect.combine(rect);
            double enlargement = combined.area() - node->entries[i].rect.area();

            double area = node->entries[i].rect.area();
            if (enlargement < minEnlargement ||
                (enlargement == minEnlargement && area < minArea)) {
                minEnlargement = enlargement;
                minArea = area; 
                bestIdx = i;
            }
        }
        return bestIdx;
    }


    int chooseSubtree(Node* node, const Rect& rect) {
        double minEnlargement = numeric_limits<double>::max();
        int bestIdx = 0;
        for (int i = 0; i < (int)node->entries.size(); i++) {
            Rect combined = node->entries[i].rect.combine(rect);
            double enlargement = combined.area() - node->entries[i].rect.area();
            if (enlargement < minEnlargement) {
                minEnlargement = enlargement;
                bestIdx = i;
            }
        }
        return bestIdx;
    }

    void adjustTree(vector<Node*>& path, vector<int>& indices,
        Node* child1, Node* child2) {
        int i = path.size() - 1;
        while (i >= 0) {
            Node* parent = path[i];
            int idx = indices[i];
            parent->entries[idx].rect =
                boundingRect(parent->entries[idx].child->entries);
            if (child2) {
                parent->entries.push_back(
                    Entry(boundingRect(child2->entries), child2));
                if ((int)parent->entries.size() > MAX_ENTRIES) {
                    Node* newSibling = splitNode(parent);
                    child2 = newSibling;
                }
                else {
                    child2 = nullptr;
                }
            }
            --i;
        }
        if (child2) {
            Node* oldRoot = root;
            root = new Node(false);
            root->entries.push_back(Entry(boundingRect(oldRoot->entries), oldRoot));
            root->entries.push_back(Entry(boundingRect(child2->entries), child2));
        }
    }

    Rect boundingRect(const vector<Entry>& es) {
        double mnX = numeric_limits<double>::max();
        double mnY = numeric_limits<double>::max();
        double mxX = numeric_limits<double>::lowest();
        double mxY = numeric_limits<double>::lowest();
        for (int i = 0; i < (int)es.size(); i++) {
            mnX = min(mnX, es[i].rect.minX); mnY = min(mnY, es[i].rect.minY);
            mxX = max(mxX, es[i].rect.maxX); mxY = max(mxY, es[i].rect.maxY);
        }
        return Rect(mnX, mnY, mxX, mxY);
    }

    Rect mbrNode(Node* node) { return boundingRect(node->entries); }

    Node* splitNode(Node* node) {
        return splitNodeGreene(node);
    }

    // ════════════════════════════════════════════════════════════════════════
    //  SPLIT CUADRÁTICO
    // ════════════════════════════════════════════════════════════════════════


    pair<int, int> pickSeedsCuadratico(Node* node) {
        double maxWaste = numeric_limits<double>::lowest();
        int bestI = 0, bestJ = 1;
        for (int i = 0; i < (int)node->entries.size(); i++)
            for (int j = i + 1; j < (int)node->entries.size(); j++) {
                double waste =
                    node->entries[i].rect.combine(node->entries[j].rect).area()
                    - node->entries[i].rect.area()
                    - node->entries[j].rect.area();
                if (waste > maxWaste) {
                    maxWaste = waste;
                    bestI = i; bestJ = j;
                }
            }
        return make_pair(bestI, bestJ);
    }

    
    void pickNextCuadratico(vector<Entry>& entries, vector<bool>& assigned,
        pair<Node*, Node*>& groups, int& count) {
        double maxDiff = numeric_limits<double>::lowest();
        int sel = -1;
        Node* target = nullptr;
        for (int i = 0; i < (int)entries.size(); i++) {
            if (assigned[i]) continue;
            double d1 = mbrNode(groups.first).combine(entries[i].rect).area()
                - mbrNode(groups.first).area();
            double d2 = mbrNode(groups.second).combine(entries[i].rect).area()
                - mbrNode(groups.second).area();
            double diff = abs(d1 - d2);
            if (diff > maxDiff) {
                maxDiff = diff; sel = i;
                if (d1 < d2) target = groups.first;
                else if (d2 < d1) target = groups.second;
                else target = (groups.first->entries.size() <= groups.second->entries.size())
                    ? groups.first : groups.second;
            }
        }
        if (sel != -1) {
            assigned[sel] = true;
            target->entries.push_back(entries[sel]);
            count++;
        }
    }

    Node* splitNodeCuadratico(Node* node) {
        // FIX: reemplaza auto [iA, iB] con acceso explicito a pair
        pair<int, int> seeds = pickSeedsCuadratico(node);
        int iA = seeds.first;
        int iB = seeds.second;

        Node* g1 = new Node(node->isLeaf); g1->entries.push_back(node->entries[iA]);
        Node* g2 = new Node(node->isLeaf); g2->entries.push_back(node->entries[iB]);
        pair<Node*, Node*> groups = make_pair(g1, g2);

        int total = (int)node->entries.size();
        vector<bool> assigned(total, false);
        assigned[iA] = true;
        assigned[iB] = true;
        int count = 2;

        while (count < total) {
            int rem = total - count;
            if ((int)g1->entries.size() + rem == MIN_ENTRIES) {
                for (int i = 0; i < total; i++)
                    if (!assigned[i]) {
                        g1->entries.push_back(node->entries[i]);
                        assigned[i] = true; count++;
                    }
                break;
            }
            if ((int)g2->entries.size() + rem == MIN_ENTRIES) {
                for (int i = 0; i < total; i++)
                    if (!assigned[i]) {
                        g2->entries.push_back(node->entries[i]);
                        assigned[i] = true; count++;
                    }
                break;
            }
            pickNextCuadratico(node->entries, assigned, groups, count);
        }
        node->entries = g1->entries;
        return g2;
    }

    // ════════════════════════════════════════════════════════════════════════
    //  SPLIT LINEAL
    // ════════════════════════════════════════════════════════════════════════+

    pair<int, int> pickSeedsLineal(Node* node) {
        double maxMinX = numeric_limits<double>::lowest();
        double minMaxX = numeric_limits<double>::max();
        double maxMinY = numeric_limits<double>::lowest();
        double minMaxY = numeric_limits<double>::max();
        double globalMaxX = numeric_limits<double>::lowest();
        double globalMinX = numeric_limits<double>::max();
        double globalMaxY = numeric_limits<double>::lowest();
        double globalMinY = numeric_limits<double>::max();

        int idxMaxMinX = 0, idxMinMaxX = 0;
        int idxMaxMinY = 0, idxMinMaxY = 0;

        for (int i = 0; i < (int)node->entries.size(); i++) {
            const Rect& r = node->entries[i].rect;
            globalMaxX = max(globalMaxX, r.maxX);
            globalMinX = min(globalMinX, r.minX);
            globalMaxY = max(globalMaxY, r.maxY);
            globalMinY = min(globalMinY, r.minY);
            if (r.minX > maxMinX) { maxMinX = r.minX; idxMaxMinX = i; }
            if (r.maxX < minMaxX) { minMaxX = r.maxX; idxMinMaxX = i; }
            if (r.minY > maxMinY) { maxMinY = r.minY; idxMaxMinY = i; }
            if (r.maxY < minMaxY) { minMaxY = r.maxY; idxMinMaxY = i; }
        }

        if (idxMaxMinX == idxMinMaxX && idxMaxMinY == idxMinMaxY)
            return make_pair(0, 1);

        double sepX = max(0.0, maxMinX - minMaxX) / max(1.0, globalMaxX - globalMinX);
        double sepY = max(0.0, maxMinY - minMaxY) / max(1.0, globalMaxY - globalMinY);

        return (sepX >= sepY)
            ? make_pair(idxMaxMinX, idxMinMaxX)
            : make_pair(idxMaxMinY, idxMinMaxY);
    }

    void pickNextLineal(vector<Entry>& entries, vector<bool>& assigned,
        pair<Node*, Node*>& groups, int& count) {
        pickNextCuadratico(entries, assigned, groups, count);
    }

    Node* splitNodeLineal(Node* node) {
        // FIX: reemplaza auto [iA, iB] con acceso explicito a pair
        pair<int, int> seeds = pickSeedsLineal(node);
        int iA = seeds.first;
        int iB = seeds.second;

        Node* g1 = new Node(node->isLeaf); g1->entries.push_back(node->entries[iA]);
        Node* g2 = new Node(node->isLeaf); g2->entries.push_back(node->entries[iB]);
        pair<Node*, Node*> groups = make_pair(g1, g2);

        int total = (int)node->entries.size();
        vector<bool> assigned(total, false);
        assigned[iA] = true;
        assigned[iB] = true;
        int count = 2;

        while (count < total) {
            int rem = total - count;
            if ((int)g1->entries.size() + rem == MIN_ENTRIES) {
                for (int i = 0; i < total; i++)
                    if (!assigned[i]) {
                        g1->entries.push_back(node->entries[i]);
                        assigned[i] = true; count++;
                    }
                break;
            }
            if ((int)g2->entries.size() + rem == MIN_ENTRIES) {
                for (int i = 0; i < total; i++)
                    if (!assigned[i]) {
                        g2->entries.push_back(node->entries[i]);
                        assigned[i] = true; count++;
                    }
                break;
            }
            pickNextLineal(node->entries, assigned, groups, count);
        }
        node->entries = g1->entries;
        return g2;
    }

    // ════════════════════════════════════════════════════════════════════════
    //  SPLIT DE GREENE
    // ════════════════════════════════════════════════════════════════════════


    int chooseAxisGreene(Node* node) {
        double hLX = numeric_limits<double>::lowest(), lHX = numeric_limits<double>::max();
        double hLY = numeric_limits<double>::lowest(), lHY = numeric_limits<double>::max();
        double mxX = numeric_limits<double>::lowest(), mnX = numeric_limits<double>::max();
        double mxY = numeric_limits<double>::lowest(), mnY = numeric_limits<double>::max();
        for (int i = 0; i < (int)node->entries.size(); i++) {
            const Entry& e = node->entries[i];
            mxX = max(mxX, e.rect.maxX); mnX = min(mnX, e.rect.minX);
            mxY = max(mxY, e.rect.maxY); mnY = min(mnY, e.rect.minY);
            if (e.rect.minX > hLX) hLX = e.rect.minX;
            if (e.rect.maxX < lHX) lHX = e.rect.maxX;
            if (e.rect.minY > hLY) hLY = e.rect.minY;
            if (e.rect.maxY < lHY) lHY = e.rect.maxY;
        }
        double sepX = max(0.0, hLX - lHX) / max(1.0, mxX - mnX);
        double sepY = max(0.0, hLY - lHY) / max(1.0, mxY - mnY);
        return (sepX >= sepY) ? 0 : 1;
    }


    Node* splitNodeGreene(Node* node) {
        int axis = chooseAxisGreene(node);
        if (axis == 0)
            sort(node->entries.begin(), node->entries.end(),
                [](const Entry& a, const Entry& b) { return a.rect.minX < b.rect.minX; });
        else
            sort(node->entries.begin(), node->entries.end(),
                [](const Entry& a, const Entry& b) { return a.rect.minY < b.rect.minY; });

        int half = (MAX_ENTRIES + 1) / 2;
        Node* g1 = new Node(node->isLeaf);
        Node* g2 = new Node(node->isLeaf);
        for (int i = 0; i < half; i++)
            g1->entries.push_back(node->entries[i]);
        for (int i = half; i < 2 * half && i < (int)node->entries.size(); i++)
            g2->entries.push_back(node->entries[i]);

        if ((MAX_ENTRIES + 1) % 2 == 1 &&
            (int)node->entries.size() > 2 * half) {
            const Entry& extra = node->entries[MAX_ENTRIES];
            double e1 = mbrNode(g1).combine(extra.rect).area() - mbrNode(g1).area();
            double e2 = mbrNode(g2).combine(extra.rect).area() - mbrNode(g2).area();
            (e1 <= e2 ? g1 : g2)->entries.push_back(extra);
        }
        node->entries = g1->entries;
        return g2;
    }

    //DELETE FUN

    Node* findLeaf(Node* node, const Rect& rect, vector<Node*>& path) {
        if (!node->isLeaf) {
            for (int i = 0; i < (int)node->entries.size(); i++) {
                if (node->entries[i].rect.overlaps(rect) ||
                    node->entries[i].rect.contains(rect)) {
                    path.push_back(node);

                    Node* result = findLeaf(node->entries[i].child, rect, path);

                    if (result != nullptr) {
                        return result;
                    }

                    path.pop_back();
                }
            }
            return nullptr;
        }

        for (int i = 0; i < (int)node->entries.size(); i++) {
            if (node->entries[i].rect.equals(rect)) {
                return node;
            }
        }
        return nullptr;
    }


    void condenseTree(Node* leaf, vector<Node*>& path) {

        Node* N = leaf;
        vector<Node*> Q; 

        while (!path.empty()) {
            Node* parent = path.back();
            path.pop_back();

            int idxInParent = -1;
            for (int i = 0; i < (int)parent->entries.size(); i++) {
                if (parent->entries[i].child == N) {
                    idxInParent = i;
                    break;
                }
            }

            if (idxInParent == -1) {
                N = parent;
                continue;
            }

            if ((int)N->entries.size() < MIN_ENTRIES) {
                parent->entries.erase(parent->entries.begin() + idxInParent);
                Q.push_back(N);
            }
            else {
                parent->entries[idxInParent].rect = boundingRect(N->entries);
            }
            N = parent;
        }

        for (int i = 0; i < (int)Q.size(); i++) {
            Node* qNode = Q[i];
            for (int j = 0; j < (int)qNode->entries.size(); j++) {
                insert(qNode->entries[j].rect);
            }
            delete qNode;
        }
    }


public:
    RTree() { root = new Node(true); }
  

    void insert(const Rect& rect) {
        Node* node = root;
        vector<Node*> path;
        vector<int>   indices;
        while (!node->isLeaf) {
            int idx = chooseSubtree(node, rect);
            path.push_back(node);
            indices.push_back(idx);
            node = node->entries[idx].child;
        }
        node->entries.push_back(Entry(rect));
        if ((int)node->entries.size() > MAX_ENTRIES) {
            Node* newSibling = splitNode(node);
            adjustTree(path, indices, node, newSibling);
        }
        else {
            adjustTree(path, indices, node, nullptr);
        }
    }

    void deleteEntry(const Rect& rect) {
        vector<Node*> path;
        Node* leaf = findLeaf(root, rect, path);
        if (leaf == nullptr) {
            cout << "  [DELETE] Rect no encontrado en el arbol.\n";
            return;
        }
        for (int i = 0; i < (int)leaf->entries.size(); i++) {
            if (leaf->entries[i].rect.equals(rect)) {
                leaf->entries.erase(leaf->entries.begin() + i);
                break;
            }
        }
        condenseTree(leaf, path);
        if (!root->isLeaf && (int)root->entries.size() == 1) {
            Node* oldRoot = root;
            root = root->entries[0].child;
            delete oldRoot; 
        }
    }

    void search(const Rect& query, vector<Rect>& result) {
        search(root, query, result);
    }

    void imprimir() {
        cout << "\nEstructura del R-Tree:\n";
        printNode(root, 0);
    }

    void printEntry(const Entry& e, int idx = -1) {
        if (idx >= 0) cout << "[Entry " << idx << "] ";
        cout << fixed << setprecision(2);
        cout << "Rect(" << e.rect.minX << ", " << e.rect.minY
            << ", " << e.rect.maxX << ", " << e.rect.maxY << ")";
        if (e.child) cout << "  -> child node";
        else         cout << "  -> dato (hoja)";
        cout << "\n";
    }

    void dumpNode(Node* node, int depth = 0) {
        if (!node) return;
        string indent(depth * 2, ' ');
        cout << indent
            << (node->isLeaf ? "Leaf" : "Internal")
            << " Node: entries=" << node->entries.size() << "\n";
        for (int i = 0; i < (int)node->entries.size(); i++) {
            const Entry& e = node->entries[i];
            cout << indent << "  [" << setw(2) << i << "] "
                << "MBR=[" << fixed << setprecision(2)
                << e.rect.minX << ", " << e.rect.minY << ", "
                << e.rect.maxX << ", " << e.rect.maxY << "]";
            if (e.child) cout << "  child";
            else         cout << "  (object)";
            cout << "\n";
        }
        if (!node->isLeaf)
            for (int i = 0; i < (int)node->entries.size(); i++)
                if (node->entries[i].child) dumpNode(node->entries[i].child, depth + 1);
    }

    void dump() {
        cout << "\n=== DUMP tecnico del arbol ===\n";
        dumpNode(root, 0);
    }



private:
    void search(Node* node, const Rect& query, vector<Rect>& result) {
        for (int i = 0; i < (int)node->entries.size(); i++) {
            if (node->entries[i].rect.overlaps(query)) {
                if (node->isLeaf) result.push_back(node->entries[i].rect);
                else              search(node->entries[i].child, query, result);
            }
        }
    }

    void printNode(Node* node, int nivel) {
        string indent(nivel * 4, ' ');
        if (node->isLeaf) {
            cout << indent << "[Hoja] " << node->entries.size() << " rectangulo(s):\n";
            for (int i = 0; i < (int)node->entries.size(); i++) {
                cout << indent << "    "; printRect(node->entries[i].rect);
            }
        }
        else {
            cout << indent << "[Interno] " << node->entries.size() << " hijo(s):\n";
            int i = 0;
            for (int j = 0; j < (int)node->entries.size(); j++) {
                cout << indent << "    Hijo " << i++ << " (MBR): "; printRect(node->entries[j].rect);
                printNode(node->entries[j].child, nivel + 1);
            }
        }
    }

    void printRect(const Rect& r) {
        cout << fixed << setprecision(2);
        cout << "[" << r.minX << ", " << r.minY << ", " << r.maxX << ", " << r.maxY << "]\n";
    }
};

// ─── Main de prueba ───────────────────────────────────────────────────────────
int main() {

    // ── PRUEBA 1: Inserción básica ────────────────────────────────────────
    cout << "========================================\n";
    cout << "  PRUEBA 1 - Insercion basica (N=10)\n";
    cout << "========================================\n";
    {
        double data[][4] = {
            { 0,  0,  2,  2},
            { 1,  0,  3,  2},
            { 0,  1,  2,  3},
            {30,  1, 32,  3},
            { 1, 20,  3, 22},
            { 0, 21,  2, 23},
            {15,  0, 17,  2},
            {14, 10, 16, 12},
            { 2, 11,  4, 13},
            {28, 15, 30, 17},
        };
        int n = sizeof(data) / sizeof(data[0]);
        RTree tree;
        for (int i = 0; i < n; i++)
            tree.insert(Rect(data[i][0], data[i][1], data[i][2], data[i][3]));
        tree.imprimir();
        tree.dump();
    }

    // ── PRUEBA 2: Búsqueda con resultados conocidos ───────────────────────
    cout << "\n========================================\n";
    cout << "  PRUEBA 2 - Busqueda con queries\n";
    cout << "========================================\n";
    {
        double data[][4] = {
            { 0,  0,  2,  2}, { 1,  0,  3,  2}, { 0,  1,  2,  3},
            {30,  1, 32,  3}, { 1, 20,  3, 22}, { 0, 21,  2, 23},
            {15,  0, 17,  2}, {14, 10, 16, 12}, { 2, 11,  4, 13},
            {28, 15, 30, 17},
        };
        int n = sizeof(data) / sizeof(data[0]);
        RTree tree;
        for (int i = 0; i < n; i++)
            tree.insert(Rect(data[i][0], data[i][1], data[i][2], data[i][3]));

        struct Query { double x1, y1, x2, y2; const char* desc; int expected; };
        Query queries[] = {
            {  0,  0, 32, 23, "Total (todo el espacio)",      10 },
            {  0,  0,  3,  3, "Cluster izquierda-abajo",       3 },
            {  0, 20,  3, 23, "Cluster arriba",                2 },
            { 29,  0, 33,  4, "Outlier derecha",               1 },
            { 50, 50, 60, 60, "Zona vacia (0 resultados)",     0 },
        };

        bool allOk = true;
        for (int q = 0; q < 5; q++) {
            vector<Rect> res;
            tree.search(Rect(queries[q].x1, queries[q].y1, queries[q].x2, queries[q].y2), res);
            bool ok = ((int)res.size() == queries[q].expected);
            allOk &= ok;
            cout << (ok ? "  [OK]   " : "  [FAIL] ")
                << queries[q].desc
                << "  -> " << res.size()
                << " (esperado: " << queries[q].expected << ")\n";
        }
        cout << "\n  Resultado: " << (allOk ? "TODOS OK" : "HAY FALLOS") << "\n";
    }

    // ── PRUEBA 3: Splits múltiples con datos aleatorios ───────────────────
    cout << "\n========================================\n";
    cout << "  PRUEBA 3 - Splits multiples (N=25)\n";
    cout << "========================================\n";
    {
        srand(42);
        RTree tree;
        int n = 25;
        for (int i = 0; i < n; i++) {
            double x1 = rand() % 95;
            double y1 = rand() % 95;
            double len = 1 + (rand() % 5);
            tree.insert(Rect(x1, y1, min(100.0, x1 + len), min(100.0, y1 + len)));
        }
        tree.imprimir();
        vector<Rect> res;
        tree.search(Rect(0, 0, 100, 100), res);
        cout << "\n  Insertados: " << n
            << "  |  Recuperados: " << res.size()
            << ((int)res.size() == n ? "  OK" : "  PERDIDA DE DATOS") << "\n";
    }

    // ── PRUEBA 4: Rectángulos idénticos (stress test del split) ───────────
    cout << "\n========================================\n";
    cout << "  PRUEBA 4 - Rectangulos identicos\n";
    cout << "========================================\n";
    {
        RTree tree;
        Rect mismo(5, 5, 10, 10);
        int n = 12;
        for (int i = 0; i < n; i++)
            tree.insert(mismo);
        vector<Rect> res;
        tree.search(mismo, res);
        cout << "  Insertados: " << n
            << "  |  Recuperados: " << res.size()
            << ((int)res.size() == n ? "  OK" : "  PERDIDA DE DATOS") << "\n";
        tree.dump();
    }

    return 0;
}
