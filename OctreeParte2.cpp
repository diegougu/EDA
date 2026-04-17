#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cfloat>
using namespace std;

struct Point {
    float x, y, z;
    Point() : x(0), y(0), z(0) {}
    Point(float a, float b, float c) : x(a), y(b), z(c) {}
};

float minDistToCube(Point A, Point bl, double h) {
    float dx = max(0.0f, max((float)bl.x - A.x, A.x - (float)(bl.x + h)));
    float dy = max(0.0f, max((float)bl.y - A.y, A.y - (float)(bl.y + h)));
    float dz = max(0.0f, max((float)bl.z - A.z, A.z - (float)(bl.z + h)));
    return sqrt(dx * dx + dy * dy + dz * dz);
}

float euclidina(const Point& a, const Point& b) {
    float x1 = a.x - b.x, y1 = a.y - b.y, z1 = a.z - b.z;
    return sqrt(x1 * x1 + y1 * y1 + z1 * z1);
}

class Octree {
private:
    Octree* children[8];
    vector<Point> points;
    Point bottomLeft;
    double h;
    int capacity;
    int nPoints = 0;

    void split(vector<Point>& pts, Octree*& node);
    bool search(const Point& p, Octree*& node);
    void fn(const Point& p, float& radius, Point& pclose, float& mdist);

public:
    Octree(double _h, int _c, Point bl) : h(_h), capacity(_c), bottomLeft(bl) {
        for (int i = 0; i < 8; i++) children[i] = nullptr;
    }

    bool exist(const Point& p);
    void insert(const Point& p);
    Point find_closest(const Point& p, int radius);
    void print(int depth);
    void printRootInfo();

    void exportOBJ(ofstream& file, int& index) {
        if (children[0] == nullptr) {
            if (nPoints > 0) {
                double x = bottomLeft.x;
                double y = bottomLeft.y;
                double z = bottomLeft.z;
                double h = this->h;

                file << "v " << x << " " << y << " " << z << "\n";
                file << "v " << x + h << " " << y << " " << z << "\n";
                file << "v " << x + h << " " << y + h << " " << z << "\n";
                file << "v " << x << " " << y + h << " " << z << "\n";
                file << "v " << x << " " << y << " " << z + h << "\n";
                file << "v " << x + h << " " << y << " " << z + h << "\n";
                file << "v " << x + h << " " << y + h << " " << z + h << "\n";
                file << "v " << x << " " << y + h << " " << z + h << "\n";

                file << "f " << index << " " << index + 1 << " " << index + 2 << " " << index + 3 << "\n";
                file << "f " << index + 4 << " " << index + 5 << " " << index + 6 << " " << index + 7 << "\n";
                file << "f " << index << " " << index + 1 << " " << index + 5 << " " << index + 4 << "\n";
                file << "f " << index + 2 << " " << index + 3 << " " << index + 7 << " " << index + 6 << "\n";
                file << "f " << index + 1 << " " << index + 2 << " " << index + 6 << " " << index + 5 << "\n";
                file << "f " << index + 3 << " " << index << " " << index + 4 << " " << index + 7 << "\n";

                index += 8;
            }
            return;
        }
        for (int i = 0; i < 8; i++)
            if (children[i]) children[i]->exportOBJ(file, index);
    }
};

bool Octree::search(const Point& p, Octree*& node) {
    while (node->children[0] != nullptr) {
        Point blH(node->bottomLeft.x + node->h / 2, node->bottomLeft.y + node->h / 2, node->bottomLeft.z + node->h / 2);
        if (p.x < blH.x && p.y < blH.y && p.z < blH.z) node = node->children[0];
        else if (p.x >= blH.x && p.y < blH.y && p.z < blH.z) node = node->children[1];
        else if (p.x < blH.x && p.y >= blH.y && p.z < blH.z) node = node->children[2];
        else if (p.x >= blH.x && p.y >= blH.y && p.z < blH.z) node = node->children[3];
        else if (p.x < blH.x && p.y < blH.y && p.z >= blH.z) node = node->children[4];
        else if (p.x >= blH.x && p.y < blH.y && p.z >= blH.z) node = node->children[5];
        else if (p.x < blH.x && p.y >= blH.y && p.z >= blH.z) node = node->children[6];
        else                                            node = node->children[7];
    }
    return node->nPoints < node->capacity;
}

void Octree::split(vector<Point>& pts, Octree*& node) {
    double half = node->h / 2;
    Point b = node->bottomLeft;
    Point offsets[8] = {
        {b.x,            b.y,            b.z           },
        {b.x + (float)half,b.y,            b.z           },
        {b.x,            b.y + (float)half,b.z           },
        {b.x + (float)half,b.y + (float)half,b.z           },
        {b.x,            b.y,            b.z + (float)half},
        {b.x + (float)half,b.y,            b.z + (float)half},
        {b.x,            b.y + (float)half,b.z + (float)half},
        {b.x + (float)half,b.y + (float)half,b.z + (float)half},
    };
    for (int i = 0; i < 8; i++) node->children[i] = new Octree(half, node->capacity, offsets[i]);
    vector<Point> old = node->points; node->points.clear(); node->nPoints = 0;
    for (auto& pt : old) insert(pt);
}

void Octree::insert(const Point& p) {
    Octree* node = this;
    if (search(p, node)) { node->nPoints++; node->points.push_back(p); }
    else { split(node->points, node); insert(p); }
}

void Octree::print(int depth) {
    string indent(depth * 2, ' ');
    if (children[0] == nullptr) {
        cout << indent << "Hoja | bl=(" << bottomLeft.x << "," << bottomLeft.y << "," << bottomLeft.z << ") h=" << h << " nPoints=" << nPoints << "\n";
    }
    else {
        cout << indent << "Nodo | bl=(" << bottomLeft.x << "," << bottomLeft.y << "," << bottomLeft.z << ") h=" << h << "\n";
        for (int i = 0; i < 8; i++) if (children[i]) children[i]->print(depth + 1);
    }
}

void Octree::printRootInfo() {
    cout << "=== Nodo Raiz ===" << endl;
    cout << "bottomLeft = (" << bottomLeft.x << ", " << bottomLeft.y << ", " << bottomLeft.z << ")" << endl;
    cout << "h (lado)   = " << h << endl;
}

void Octree::fn(const Point& p, float& radius, Point& pclose, float& mdist) {
    if (children[0] == nullptr) {
        for (auto& pt : points) {
            float dist = euclidina(p, pt);
            if (dist <= radius && dist < mdist) { mdist = dist; pclose = pt; }
        }
        return;
    }
    for (int i = 0; i < 8; i++) {
        if (!children[i]) continue;
        float dc = minDistToCube(p, children[i]->bottomLeft, children[i]->h);
        if (dc < mdist) children[i]->fn(p, radius, pclose, mdist);
    }
}

Point Octree::find_closest(const Point& p, int radius) {
    Point pclose; float r = (float)radius, mdist = r;
    fn(p, r, pclose, mdist); return pclose;
}

bool Octree::exist(const Point& p) {
    Octree* node = this; search(p, node);
    for (auto& pt : node->points) if (pt.x == p.x && pt.y == p.y && pt.z == p.z) return true;
    return false;
}

vector<Point> loadXYZ(const string& filename) {
    vector<Point> pts;
    ifstream file(filename);
    if (!file.is_open()) { cerr << "No se pudo abrir: " << filename << endl; return pts; }
    float x, y, z;
    while (file >> x >> y >> z) pts.push_back(Point(x, y, z));
    cout << "Puntos cargados: " << pts.size() << endl;
    return pts;
}

int main() {
    const string ARCHIVO = "aguila.xyz";
    const int    CAPACITY = 15;

    vector<Point> pts = loadXYZ(ARCHIVO);
    if (pts.empty()) return 1;

    float minX = pts[0].x, maxX = pts[0].x, minY = pts[0].y, maxY = pts[0].y, minZ = pts[0].z, maxZ = pts[0].z;
    for (auto& p : pts) {
        minX = min(minX, p.x); maxX = max(maxX, p.x);
        minY = min(minY, p.y); maxY = max(maxY, p.y);
        minZ = min(minZ, p.z); maxZ = max(maxZ, p.z);
    }
    double h = max({ (double)(maxX - minX),(double)(maxY - minY),(double)(maxZ - minZ) }) * 1.01;
    Point bl(minX, minY, minZ);

    Octree tree(h, CAPACITY, bl);
    for (auto& p : pts) tree.insert(p);

    tree.printRootInfo();

    ofstream outFile("octree.obj");
    int index = 1;
    tree.exportOBJ(outFile, index);
    outFile.close();
    cout << "OBJ generado -> octree.obj" << endl;

}
