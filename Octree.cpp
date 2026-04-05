#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
using namespace std;

struct Point {
    int x;
    int y;
    int z;
    Point() : x(0), y(0), z(0) {}
    Point(int a, int b, int c) : x(a), y(b), z(c) {}

};

float minDistToCube(Point A, Point bl, double h) {
    float dx = max(0.0f, max((float)bl.x - A.x, A.x - (float)(bl.x + h)));
    float dy = max(0.0f, max((float)bl.y - A.y, A.y - (float)(bl.y + h)));
    float dz = max(0.0f, max((float)bl.z - A.z, A.z - (float)(bl.z + h)));
    return sqrt(dx * dx + dy * dy + dz * dz);
}

float euclidina(const Point& a, const Point& b) {
    float x1 = float(a.x - b.x);
    float y1 = float(a.y - b.y);
    float z1 = float(a.z - b.z);
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
    void split(vector<Point>& points, Octree*& node);
    bool search(const Point& p, Octree*& node);
    void fn(const Point& p, float& radius, Point& pclose, float& mdist);

public:
    Octree(double _h, int _c, Point bl) : h(_h), capacity(_c), bottomLeft(bl)  {
        for (int i = 0; i < 8; i++) {
            children[i] = nullptr;
        }
    }
    bool exist(const Point& p);
    void insert(const Point& p);
    Point find_closest(const Point& p, int radius);
    void print(int depth = 0);
};

bool Octree::search(const Point& p, Octree*& node) {
    while (node->children[0] != nullptr) {
        Point blH(node->bottomLeft.x + (node->h / 2), node->bottomLeft.y + (node->h / 2), node->bottomLeft.z + (node->h / 2));
        if (p.x < blH.x && p.y < blH.y && p.z < blH.z) {
            node = node->children[0];
        }
        else if (p.x >= blH.x && p.y < blH.y && p.z < blH.z) {
            node = node->children[1];
        }
        else if (p.x < blH.x && p.y >= blH.y && p.z < blH.z) {
            node = node->children[2];
        }
        else if (p.x >= blH.x && p.y >= blH.y && p.z < blH.z) {
            node = node->children[3];
        }
        else if (p.x < blH.x && p.y < blH.y && p.z >= blH.z) {
            node = node->children[4];
        }
        else if (p.x >= blH.x && p.y < blH.y && p.z >= blH.z) {
            node = node->children[5];
        }
        else if (p.x < blH.x && p.y >= blH.y && p.z >= blH.z) {
            node = node->children[6];
        }
        else if (p.x >= blH.x && p.y >= blH.y && p.z >= blH.z) {
            node = node->children[7];
        }
    }
    return node->nPoints < node->capacity;
}

void Octree::split(vector<Point>& points, Octree*& node) {
    double half = node->h / 2;
    for (int i = 0; i < 8; i++) {
        Point newBottomLeft;
        switch (i) {
        case 0:
            newBottomLeft.x = node->bottomLeft.x;
            newBottomLeft.y = node->bottomLeft.y;
            newBottomLeft.z = node->bottomLeft.z;
            break;
        case 1:
            newBottomLeft.x = node->bottomLeft.x + half;
            newBottomLeft.y = node->bottomLeft.y;
            newBottomLeft.z = node->bottomLeft.z;
            break;
        case 2:
            newBottomLeft.x = node->bottomLeft.x;
            newBottomLeft.y = node->bottomLeft.y + half;
            newBottomLeft.z = node->bottomLeft.z;
            break;
        case 3:
            newBottomLeft.x = node->bottomLeft.x + half;
            newBottomLeft.y = node->bottomLeft.y + half;
            newBottomLeft.z = node->bottomLeft.z;
            break;
        case 4:
            newBottomLeft.x = node->bottomLeft.x;
            newBottomLeft.y = node->bottomLeft.y;
            newBottomLeft.z = node->bottomLeft.z + half;
            break;
        case 5:
            newBottomLeft.x = node->bottomLeft.x + half;
            newBottomLeft.y = node->bottomLeft.y;
            newBottomLeft.z = node->bottomLeft.z + half;
            break;
        case 6:
            newBottomLeft.x = node->bottomLeft.x;
            newBottomLeft.y = node->bottomLeft.y + half;
            newBottomLeft.z = node->bottomLeft.z + half;
            break;
        case 7:
            newBottomLeft.x = node->bottomLeft.x + half;
            newBottomLeft.y = node->bottomLeft.y + half;
            newBottomLeft.z = node->bottomLeft.z + half;
            break;
        }
        node->children[i] = new Octree(half, node->capacity, newBottomLeft);
    }

    vector<Point> old = node->points;
    node->points.clear();   
    node->nPoints = 0;
    for (auto& pt : old) {
        insert(pt);
    }
}


void Octree::insert(const Point& p) {
    Octree* node = this;
    if (search(p, node)) {
        node->nPoints++;
        node->points.push_back(p);
    }
    else {
        split(node->points, node);
        insert(p);
    }
}

void Octree::print(int depth) {
    string indent(depth * 2, ' ');
    if (children[0] == nullptr) {
        cout << indent << "Hoja | bl=(" << bottomLeft.x << ","
            << bottomLeft.y << "," << bottomLeft.z
            << ") h=" << h << " nPoints=" << nPoints << "\n";
        for (auto& p : points)
            cout << indent << "  (" << p.x << "," << p.y << "," << p.z << ")\n";
    }
    else {
        cout << indent << "Nodo | bl=(" << bottomLeft.x << ","
            << bottomLeft.y << "," << bottomLeft.z
            << ") h=" << h << "\n";
        for (int i = 0; i < 8; i++)
            if (children[i]) children[i]->print(depth + 1);
    }
}

void Octree::fn(const Point& p, float& radius, Point& pclose, float& mdist) {
    if (this->children[0] == nullptr) {
        for (int i = 0; i < this->points.size(); i++) {
            float dist = euclidina(p, this->points[i]);
            if (dist <= radius && dist < mdist) {
                mdist = dist;
                pclose = this->points[i];
            }
        }
        return;
    }
    for (int i = 0; i < 8; i++) {
        if (this->children[i] == nullptr) continue;
        float dc = minDistToCube(p, this->children[i]->bottomLeft, this->children[i]->h);
        if (dc < mdist) {
            this->children[i]->fn(p, radius, pclose, mdist);
        }
    }
}

Point Octree::find_closest(const Point& p, int radius) {
    Point pclose;
    float radius2 = (float)radius;
    float mdist = radius2;
    fn(p, radius2, pclose, mdist);
    return pclose;
}

bool Octree::exist(const Point& p) {
    Octree* node = this;
    search(p, node);
    for (auto& pt : node->points)
        if (pt.x == p.x && pt.y == p.y && pt.z == p.z) return true;
    return false;
}

int main() {
    Octree tree(8.0, 2, Point(0, 0, 0));

    tree.insert(Point(1, 1, 1));
    tree.insert(Point(6, 6, 6));
    tree.insert(Point(2, 2, 2)); 
    tree.insert(Point(5, 5, 5));
    tree.insert(Point(1, 2, 1));

    cout << "=== Arbol ===" << endl;
    tree.print(0);


    Point A(1, 1, 1);
    Point res = tree.find_closest(A, 25);
    cout << "Mas cercano a (1,1,1) con radio 25: ";
    cout << "(" << res.x << "," << res.y << "," << res.z << ")" << endl;
}
