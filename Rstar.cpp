#include <iostream>
#include <vector>
#include <algorithm>
#include <limits>
#include <cmath>
#include <iomanip>
#include <numeric>
using namespace std;

// =============================================================
//  ESTRUCTURAS BASE
// =============================================================

struct Rect {
    double minX, minY, maxX, maxY;

    Rect(double minX = 0, double minY = 0, double maxX = 0, double maxY = 0)
        : minX(minX), minY(minY), maxX(maxX), maxY(maxY) {}

    bool overlaps(const Rect& o) const {
        return !(minX > o.maxX || maxX < o.minX ||
                 minY > o.maxY || maxY < o.minY);
    }

    bool contains(const Rect& o) const {
        return minX <= o.minX && maxX >= o.maxX &&
               minY <= o.minY && maxY >= o.maxY;
    }

    double area() const {
        return max(0.0, maxX - minX) * max(0.0, maxY - minY);
    }

    double margin() const {
        return 2.0 * (max(0.0, maxX - minX) + max(0.0, maxY - minY));
    }

    double overlapWith(const Rect& o) const {
        double ox = max(0.0, min(maxX, o.maxX) - max(minX, o.minX));
        double oy = max(0.0, min(maxY, o.maxY) - max(minY, o.minY));
        return ox * oy;
    }

    double centerX() const { return (minX + maxX) / 2.0; }
    double centerY() const { return (minY + maxY) / 2.0; }

    Rect combine(const Rect& o) const {
        return Rect(min(minX, o.minX), min(minY, o.minY),
                    max(maxX, o.maxX), max(maxY, o.maxY));
    }

    double enlargement(const Rect& o) const {
        return combine(o).area() - area();
    }
};

// =============================================================
//  PARAMETROS  (m = 40% de M, p = 30% de M)
// =============================================================
const int M = 4;
const int m = max(1, (int)ceil(0.4 * M));
const int P = max(1, (int)ceil(0.3 * M));

// =============================================================
//  NODO Y ENTRADA
// =============================================================
struct Node;

struct Entry {
    Rect  rect;
    Node* child;
    int   oid;
    Entry(const Rect& r, Node* c = nullptr, int id = -1)
        : rect(r), child(c), oid(id) {}
};

struct Node {
    bool          isLeaf;
    vector<Entry> entries;
    Node(bool leaf = true) : isLeaf(leaf) {}
};

// =============================================================
//  R*-TREE
// =============================================================
class RStarTree {
    Node* root;

    // ----------------------------------------------------------
    //  boundingRect
    // ----------------------------------------------------------
    Rect boundingRect(const vector<Entry>& es) const {
        double x1 =  numeric_limits<double>::max();
        double y1 =  numeric_limits<double>::max();
        double x2 = -numeric_limits<double>::max();
        double y2 = -numeric_limits<double>::max();
        for (const auto& e : es) {
            x1 = min(x1, e.rect.minX); y1 = min(y1, e.rect.minY);
            x2 = max(x2, e.rect.maxX); y2 = max(y2, e.rect.maxY);
        }
        return Rect(x1, y1, x2, y2);
    }

    // ----------------------------------------------------------
    //  updateMBR
    // ----------------------------------------------------------
    void updateMBR(Entry& e) {
        if (e.child) e.rect = boundingRect(e.child->entries);
    }

    // ----------------------------------------------------------
    //  chooseSubtree  [R*]
    //  Insert → I1: ChooseSubtree (CS1, CS2, CS3)
    //  usa: overlapWith, enlargement
    // ----------------------------------------------------------
    int chooseSubtree(Node* node, const Rect& r) const {
        bool childrenAreLeaves = node->entries[0].child &&
                                 node->entries[0].child->isLeaf;

        if (childrenAreLeaves) {
            double bestOverlap = numeric_limits<double>::max();
            double bestEnlarge = numeric_limits<double>::max();
            double bestArea    = numeric_limits<double>::max();
            int    bestIdx     = 0;

            for (int i = 0; i < (int)node->entries.size(); i++) {
                const Rect& ri = node->entries[i].rect;
                Rect combined  = ri.combine(r);

                double overlapBefore = 0, overlapAfter = 0;
                for (int j = 0; j < (int)node->entries.size(); j++) {
                    if (j == i) continue;
                    overlapBefore += ri.overlapWith(node->entries[j].rect);
                    overlapAfter  += combined.overlapWith(node->entries[j].rect);
                }
                double deltaOverlap = overlapAfter - overlapBefore;
                double deltaArea    = ri.enlargement(r);
                double area         = ri.area();

                if (deltaOverlap < bestOverlap ||
                   (deltaOverlap == bestOverlap && deltaArea < bestEnlarge) ||
                   (deltaOverlap == bestOverlap && deltaArea == bestEnlarge && area < bestArea)) {
                    bestOverlap = deltaOverlap;
                    bestEnlarge = deltaArea;
                    bestArea    = area;
                    bestIdx     = i;
                }
            }
            return bestIdx;

        } else {
            double bestEnlarge = numeric_limits<double>::max();
            double bestArea    = numeric_limits<double>::max();
            int    bestIdx     = 0;

            for (int i = 0; i < (int)node->entries.size(); i++) {
                double enl  = node->entries[i].rect.enlargement(r);
                double area = node->entries[i].rect.area();
                if (enl < bestEnlarge || (enl == bestEnlarge && area < bestArea)) {
                    bestEnlarge = enl;
                    bestArea    = area;
                    bestIdx     = i;
                }
            }
            return bestIdx;
        }
    }

    // ----------------------------------------------------------
    //  sumOfMargins  [R*]
    //  auxiliar de Split → ChooseSplitAxis (CSA1)
    // ----------------------------------------------------------
    double sumOfMargins(vector<Entry> sorted) const {
        double S = 0;
        int distributions = M - 2*m + 2;
        for (int k = 1; k <= distributions; k++) {
            int split = (m - 1) + k;
            vector<Entry> g1(sorted.begin(), sorted.begin() + split);
            vector<Entry> g2(sorted.begin() + split, sorted.end());
            S += boundingRect(g1).margin() + boundingRect(g2).margin();
        }
        return S;
    }

    // ----------------------------------------------------------
    //  chooseSplitAxis  [R*]
    //  Split → S1: ChooseSplitAxis (CSA1, CSA2)
    //  usa: sumOfMargins
    // ----------------------------------------------------------
    vector<Entry> chooseSplitAxis(vector<Entry> entries) const {
        vector<Entry> sortedByMinX = entries;
        sort(sortedByMinX.begin(), sortedByMinX.end(),
             [](const Entry& a, const Entry& b){ return a.rect.minX < b.rect.minX; });

        vector<Entry> sortedByMaxX = entries;
        sort(sortedByMaxX.begin(), sortedByMaxX.end(),
             [](const Entry& a, const Entry& b){ return a.rect.maxX < b.rect.maxX; });

        vector<Entry> sortedByMinY = entries;
        sort(sortedByMinY.begin(), sortedByMinY.end(),
             [](const Entry& a, const Entry& b){ return a.rect.minY < b.rect.minY; });

        vector<Entry> sortedByMaxY = entries;
        sort(sortedByMaxY.begin(), sortedByMaxY.end(),
             [](const Entry& a, const Entry& b){ return a.rect.maxY < b.rect.maxY; });

        double SX = sumOfMargins(sortedByMinX) + sumOfMargins(sortedByMaxX);
        double SY = sumOfMargins(sortedByMinY) + sumOfMargins(sortedByMaxY);

        if (SX <= SY) {
            return (sumOfMargins(sortedByMinX) <= sumOfMargins(sortedByMaxX))
                   ? sortedByMinX : sortedByMaxX;
        } else {
            return (sumOfMargins(sortedByMinY) <= sumOfMargins(sortedByMaxY))
                   ? sortedByMinY : sortedByMaxY;
        }
    }

    // ----------------------------------------------------------
    //  chooseSplitIndex  [R*]
    //  Split → S2: ChooseSplitIndex (CSI1)
    //  usa: boundingRect
    // ----------------------------------------------------------
    int chooseSplitIndex(const vector<Entry>& sorted) const {
        double bestOverlap = numeric_limits<double>::max();
        double bestArea    = numeric_limits<double>::max();
        int    bestK       = m;

        int distributions = M - 2*m + 2;
        for (int k = 1; k <= distributions; k++) {
            int split = (m - 1) + k;
            vector<Entry> g1(sorted.begin(), sorted.begin() + split);
            vector<Entry> g2(sorted.begin() + split, sorted.end());
            Rect bb1 = boundingRect(g1);
            Rect bb2 = boundingRect(g2);

            double ov   = bb1.overlapWith(bb2);
            double area = bb1.area() + bb2.area();

            if (ov < bestOverlap || (ov == bestOverlap && area < bestArea)) {
                bestOverlap = ov;
                bestArea    = area;
                bestK       = split;
            }
        }
        return bestK;
    }

    // ----------------------------------------------------------
    //  splitNode  [R*]
    //  OverflowTreatment → Split (S1, S2, S3)
    //  usa: chooseSplitAxis, chooseSplitIndex
    // ----------------------------------------------------------
    Node* splitNode(Node* node) {
        vector<Entry> all    = node->entries;
        vector<Entry> sorted = chooseSplitAxis(all);
        int splitIdx         = chooseSplitIndex(sorted);

        node->entries.assign(sorted.begin(), sorted.begin() + splitIdx);
        Node* sibling = new Node(node->isLeaf);
        sibling->entries.assign(sorted.begin() + splitIdx, sorted.end());
        return sibling;
    }

    // ----------------------------------------------------------
    //  computeReinsertEntries  [R*]
    //  OverflowTreatment → ReInsert (RI1, RI2, RI3)
    //  usa: boundingRect
    // ----------------------------------------------------------
    vector<Entry> computeReinsertEntries(Node* node) {
        Rect nodeMBR = boundingRect(node->entries);
        double cx = nodeMBR.centerX(), cy = nodeMBR.centerY();

        sort(node->entries.begin(), node->entries.end(),
             [cx, cy](const Entry& a, const Entry& b){
                 double da = hypot(a.rect.centerX()-cx, a.rect.centerY()-cy);
                 double db = hypot(b.rect.centerX()-cx, b.rect.centerY()-cy);
                 return da > db;
             });

        vector<Entry> toReinsert(node->entries.begin(),
                                 node->entries.begin() + P);
        node->entries.erase(node->entries.begin(),
                            node->entries.begin() + P);
        return toReinsert;
    }

    // ----------------------------------------------------------
    //  overflowTreatment  [R*]
    //  Insert → I2: OverflowTreatment (OT1)
    //  usa: computeReinsertEntries (→ ReInsert RI4), splitNode (→ Split)
    // ----------------------------------------------------------
    Node* overflowTreatment(Node* node, int level,
                            vector<bool>& reinsertedLevels,
                            int treeHeight) {
        if (level != 0 && !reinsertedLevels[level]) {
            reinsertedLevels[level] = true;
            vector<Entry> toReinsert = computeReinsertEntries(node);

            for (int i = (int)toReinsert.size()-1; i >= 0; i--)
                insertAtLevel(toReinsert[i], level, reinsertedLevels, treeHeight);

            return nullptr;
        } else {
            return splitNode(node);
        }
    }

    // ----------------------------------------------------------
    //  insertAtLevel  [R*]
    //  ReInsert → RI4: re-llamada a Insert en nivel especifico
    //  usa: chooseSubtree, overflowTreatment, propagateSplit
    // ----------------------------------------------------------
    void insertAtLevel(const Entry& entry, int targetLevel,
                       vector<bool>& reinsertedLevels, int treeHeight) {
        Node* node = root;
        vector<Node*> path;
        vector<int>   pathIdx;
        int curLevel = 0;

        while (curLevel < targetLevel) {
            path.push_back(node);
            int idx = chooseSubtree(node, entry.rect);
            pathIdx.push_back(idx);
            node = node->entries[idx].child;
            curLevel++;
        }

        node->entries.push_back(entry);

        if ((int)node->entries.size() > M) {
            Node* sibling = overflowTreatment(node, curLevel,
                                              reinsertedLevels, treeHeight);
            if (sibling) propagateSplit(path, pathIdx, node, sibling);
        } else {
            for (int i = (int)path.size()-1; i >= 0; i--)
                path[i]->entries[pathIdx[i]].rect =
                    boundingRect(path[i]->entries[pathIdx[i]].child->entries);
        }
    }

    // ----------------------------------------------------------
    //  propagateSplit
    //  Insert → I3: propagar OverflowTreatment hacia arriba (I3, I4)
    //  usa: updateMBR, boundingRect, splitNode
    // ----------------------------------------------------------
    void propagateSplit(vector<Node*>& path, vector<int>& pathIdx,
                        Node*, Node* sibling) {
        for (int i = (int)path.size()-1; i >= 0; i--) {
            Node* parent = path[i];
            int   idx    = pathIdx[i];

            updateMBR(parent->entries[idx]);
            parent->entries.push_back(
                Entry(boundingRect(sibling->entries), sibling));

            if ((int)parent->entries.size() <= M) {
                sibling = nullptr;
                for (int j = i-1; j >= 0; j--)
                    updateMBR(path[j]->entries[pathIdx[j]]);
                break;
            }

            sibling = splitNode(parent);
        }

        if (sibling) {
            Node* oldRoot = root;
            root = new Node(false);
            root->entries.push_back(Entry(boundingRect(oldRoot->entries), oldRoot));
            root->entries.push_back(Entry(boundingRect(sibling->entries), sibling));
        }
    }

    // ----------------------------------------------------------
    //  treeHeight
    // ----------------------------------------------------------
    int treeHeight() const {
        int h = 0;
        Node* n = root;
        while (!n->isLeaf) { n = n->entries[0].child; h++; }
        return h;
    }

    // ----------------------------------------------------------
    //  searchNode
    // ----------------------------------------------------------
    void searchNode(Node* node, const Rect& query, vector<Rect>& result) const {
        for (const auto& e : node->entries) {
            if (e.rect.overlaps(query)) {
                if (node->isLeaf)
                    result.push_back(e.rect);
                else
                    searchNode(e.child, query, result);
            }
        }
    }

    // ----------------------------------------------------------
    //  printNode
    // ----------------------------------------------------------
    void printNode(Node* node, int nivel) const {
        string indent(nivel * 4, ' ');
        if (node->isLeaf) {
            cout << indent << "[Hoja]  " << node->entries.size() << " rect(s):\n";
            for (const auto& e : node->entries) {
                cout << indent << "    ";
                printRect(e.rect);
            }
        } else {
            cout << indent << "[Interno]  " << node->entries.size() << " hijo(s):\n";
            for (int i = 0; i < (int)node->entries.size(); i++) {
                cout << indent << "    Hijo " << i << " MBR: ";
                printRect(node->entries[i].rect);
                printNode(node->entries[i].child, nivel + 1);
            }
        }
    }

    // ----------------------------------------------------------
    //  printRect
    // ----------------------------------------------------------
    void printRect(const Rect& r) const {
        cout << fixed << setprecision(2)
             << "[" << r.minX << ", " << r.minY
             << ", " << r.maxX << ", " << r.maxY << "]\n";
    }

public:
    // ----------------------------------------------------------
    //  constructor
    // ----------------------------------------------------------
    RStarTree() { root = new Node(true); }

    // ----------------------------------------------------------
    //  insert  [R*]
    //  InsertData → Insert (I1, I2, I3, I4)
    //  usa: chooseSubtree, overflowTreatment, propagateSplit, updateMBR
    // ----------------------------------------------------------
    void insert(const Rect& rect, int oid = -1) {
        int height = treeHeight();
        vector<bool> reinsertedLevels(height + 1, false);

        Node* node = root;
        vector<Node*> path;
        vector<int>   pathIdx;
        int curLevel = 0;

        while (!node->isLeaf) {
            path.push_back(node);
            int idx = chooseSubtree(node, rect);
            pathIdx.push_back(idx);
            node = node->entries[idx].child;
            curLevel++;
        }

        node->entries.push_back(Entry(rect, nullptr, oid));

        if ((int)node->entries.size() <= M) {
            for (int i = (int)path.size()-1; i >= 0; i--)
                updateMBR(path[i]->entries[pathIdx[i]]);
            return;
        }

        Node* sibling = overflowTreatment(node, curLevel,
                                          reinsertedLevels, height);
        if (sibling)
            propagateSplit(path, pathIdx, node, sibling);
    }

    // ----------------------------------------------------------
    //  search
    // ----------------------------------------------------------
    vector<Rect> search(const Rect& query) const {
        vector<Rect> result;
        searchNode(root, query, result);
        return result;
    }

    // ----------------------------------------------------------
    //  print
    // ----------------------------------------------------------
    void print() const {
        cout << "\n===== R*-Tree (M=" << M << ", m=" << m
             << ", p=" << P << ") =====\n";
        printNode(root, 0);
        cout << "Altura del arbol: " << treeHeight() << "\n";
    }
};

// =============================================================
//  MAIN
// =============================================================
int main() {
    RStarTree tree;

    srand(42);
    cout << "Insertando 25 rectangulos...\n";
    for (int i = 0; i < 25; i++) {
        double x1     = rand() % 95;
        double y1     = rand() % 95;
        double length = 1 + (rand() % 5);
        double x2     = min(100.0, x1 + length);
        double y2     = min(100.0, y1 + length);
        tree.insert(Rect(x1, y1, x2, y2), i);
    }

    tree.print();

    Rect query(20, 20, 60, 60);
    cout << "\n--- Busqueda en [20,20,60,60] ---\n";
    vector<Rect> results = tree.search(query);
    cout << "Encontrados: " << results.size() << " rectangulo(s)\n";
    for (const auto& r : results)
        cout << "  [" << r.minX << "," << r.minY
             << "," << r.maxX << "," << r.maxY << "]\n";

    return 0;
}
