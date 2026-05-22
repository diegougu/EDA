#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>

struct Node {
    std::vector<int> data;  
    int splitting_axis;     
    Node* left;             
    Node* right;            

    Node(std::vector<int> pt, int axis) {
        data = pt;
        splitting_axis = axis;
        left = nullptr;
        right = nullptr;
    }
};

std::vector<std::vector<int>> readCSV(const std::string& filename) {
    std::vector<std::vector<int>> data;
    std::ifstream file(filename);
    std::string line;

    if (!file.is_open()) {
        std::cerr << "Error abriendo el archivo: " << filename << "\n";
        return data;
    }

    while (std::getline(file, line)) {
        std::vector<int> row;
        std::stringstream ss(line);
        std::string value;

        while (std::getline(ss, value, ',')) {
            try {
                row.push_back(std::stoi(value));
            } catch (const std::exception& e) {
            }
        }
        
        if (!row.empty()) {
            data.push_back(row);
        }
    }

    file.close();
    return data;
}

Node* buildKDTreeWithMedian(std::vector<std::vector<int>>& points, int depth) {
    if (points.empty()) {
        return nullptr;
    }

    int axis = depth % 2;

    std::sort(points.begin(), points.end(), [axis](const std::vector<int>& a, const std::vector<int>& b) {
        return a[axis] < b[axis];
    });

    int medianIndex = points.size() / 2;
    std::vector<int> medianPoint = points[medianIndex];

    Node* node = new Node(medianPoint, axis);

    std::vector<std::vector<int>> leftPoints(points.begin(), points.begin() + medianIndex);
    std::vector<std::vector<int>> rightPoints(points.begin() + medianIndex + 1, points.end());

    node->left = buildKDTreeWithMedian(leftPoints, depth + 1);
    node->right = buildKDTreeWithMedian(rightPoints, depth + 1);

    return node;
}

void printPreOrder(Node* root) {
    if (root != nullptr) {
        char axis_char = (root->splitting_axis == 0) ? 'X' : 'Y';
        std::cout << "(" << root->data[0] << ", " << root->data[1] << ") " 
                  << "[Divisor: " << axis_char << "]\n";
        printPreOrder(root->left);
        printPreOrder(root->right);
    }
}

int main() {
    std::string filename = "datos.csv";
    
    std::vector<std::vector<int>> points = readCSV(filename);

    if (points.empty()) {
        std::cerr << "No se cargaron datos o el archivo no existe.\n";
        return 1;
    }

    Node* root = buildKDTreeWithMedian(points, 0);

    printPreOrder(root);

    return 0;
}
