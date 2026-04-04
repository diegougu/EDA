#include <iostream>
#include <vector>
using namespace std;

struct Node {
    vector<Node*> quad = vector<Node*>(4, nullptr);
    int valor;
    int x, y;
    int size;
    Node(int v, int _x, int _y, int _s) : valor(v), x(_x), y(_y), size(_s) {}
    bool isLeaf() { return quad[0] == nullptr; }
};

class RegionQuadTree {
public:
    int limite;
    Node* root = nullptr;
    vector<vector<int>> grid;

    RegionQuadTree(int lim) : limite(lim), grid(lim, vector<int>(lim, 0)) {}

    void build();
    void print();
    void set(int x, int y, int val);

private:
    bool isUniform(int x, int y, int size);
    Node* buildRecursive(int x, int y, int size);
    void printRecursive(Node* node, int depth);
};

bool RegionQuadTree::isUniform(int x, int y, int size) {
    int value = grid[x][y];
    for (int i = x; i < x + size; i++) {
        for (int j = y; j < y + size; j++) {
            if (grid[i][j] != value) {
                return false;
            }
        }
    }
    return true;
}

void RegionQuadTree::printRecursive(Node* node, int depth) {
    if (!node) return;
    string indent(depth * 2, ' ');
    if (node->isLeaf()) {
        cout << indent << "Hoja(" << node->x << "," << node->y
            << ") val=" << node->valor << " size=" << node->size
            << " depth=" << depth << "\n";
    }
    else {
        cout << indent << "Nodo(" << node->x << "," << node->y
            << ") size=" << node->size << "\n";
    }
    for (int i = 0; i < 4; i++)
        printRecursive(node->quad[i], depth + 1);
}

Node* RegionQuadTree::buildRecursive(int x, int y, int size) {
    if (size == 1 || isUniform(x, y, size)) {
        Node* newNode = new Node(grid[x][y], x, y, size);
        return newNode;
    }

    Node* newNode = new Node(-1, x, y, size);
    int half = size / 2;
    newNode->quad[0] = buildRecursive(x, y, half);
    newNode->quad[1] = buildRecursive(x + half, y, half);
    newNode->quad[2] = buildRecursive(x, y + half, half);
    newNode->quad[3] = buildRecursive(x + half, y + half, half);

    return newNode;
}



void RegionQuadTree::build() {
    root = nullptr;
    root = buildRecursive(0, 0, limite);
}

void RegionQuadTree::set(int x, int y, int v) {
    grid[x][y] = v;
    build();
}

void RegionQuadTree::print() {
    printRecursive(root, 0);
}


int main() {
    RegionQuadTree qt(4);

    qt.grid = {
        {1, 0, 0, 0},
        {1, 1, 1, 0},
        {1, 0, 1, 1},
        {0, 0, 1, 1}
    };

    qt.build();
    qt.print();

    return 0;
}
