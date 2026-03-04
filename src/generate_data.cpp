#include <iostream>
#include <fstream>
#include <random>

using namespace std;

int main() {

    int n;
    cout << "Nhap so luong so can tao: ";
    cin >> n;

    string filename;
    cout << "Nhap ten file (vd data/numbers1.bin): ";
    cin >> filename;

    ofstream out(filename, ios::binary);

    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<double> dist(-1000.0, 1000.0);

    for(int i = 0; i < n; i++){
        double x = dist(gen);
        out.write(reinterpret_cast<char*>(&x), sizeof(double));
    }

    out.close();

    cout << "Da tao file " << filename << " voi " << n << " so double.\n";

    return 0;
}