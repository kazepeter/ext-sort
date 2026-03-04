#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <queue>
#include <string>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

// Đọc/Ghi số double ở dạng nhị phân (8 bytes) 

// Đọc 1 số double từ file nhị phân
static bool readDouble(ifstream &in, double &x) {
    return (bool)in.read(reinterpret_cast<char*>(&x), sizeof(double));
}

// Ghi 1 số double vào file nhị phân
static void writeDouble(ofstream &out, double x) {
    out.write(reinterpret_cast<const char*>(&x), sizeof(double));
}

// Đếm tổng số phần tử double trong file
static uint64_t countDoublesInFile(const string &path) {
    ifstream in(path, ios::binary);
    if (!in) return 0;

    in.seekg(0, ios::end);
    uint64_t bytes = (uint64_t)in.tellg();

    return bytes / sizeof(double);
}

// Hiển thị trước một số phần tử đầu của file
static void previewFile(const string &path, int k = 10) {
    ifstream in(path, ios::binary);
    if (!in) {
        cout << "Khong mo duoc file: " << path << "\n";
        return;
    }

    cout << "\n[Preview] " << path << " (first " << k << ")\n";

    double x;
    int cnt = 0;

    while (cnt < k && readDouble(in, x)) {
        cout << "  [" << cnt << "] "
             << fixed << setprecision(6) << x << "\n";
        cnt++;
    }

    if (cnt == 0) cout << "  (empty)\n";

    cout << "Tong so double trong file = "
         << countDoublesInFile(path) << "\n";
}


// Liệt kê và chọn file dữ liệu nguồn 
// Lấy danh sách tất cả file .bin trong thư mục
static vector<string> listBinFiles(const string &dir) {
    vector<string> files;

    if (!fs::exists(dir)) return files;

    for (auto &entry : fs::directory_iterator(dir)) {
        if (entry.is_regular_file()) {
            auto p = entry.path();

            if (p.extension() == ".bin")
                files.push_back(p.string());
        }
    }

    sort(files.begin(), files.end());

    return files;
}

// Cho người dùng chọn file dữ liệu nguồn trong thư mục data
static string chooseBinFileFromData(const string &dir = "data") {

    auto files = listBinFiles(dir);

    cout << "\n=== CHON TAP TIN DU LIEU NGUON (*.bin) TRONG '" << dir << "' ===\n";

    // Nếu không tìm thấy file .bin
    if (files.empty()) {
        cout << "Khong tim thay file .bin trong '" << dir << "'.\n";
        cout << "Nhap duong dan file nguon thu cong: ";

        string p;
        cin >> p;

        return p;
    }

    // Hiển thị danh sách file
    for (size_t i = 0; i < files.size(); i++) {
        cout << (i + 1) << ") "
             << files[i]
             << "  (n=" << countDoublesInFile(files[i]) << ")\n";
    }

    cout << "0) Nhap duong dan thu cong\n";
    cout << "Chon: ";

    int pick;
    cin >> pick;

    if (pick == 0) {
        cout << "Nhap duong dan file nguon: ";

        string p;
        cin >> p;

        return p;
    }

    if (pick < 1 || pick > (int)files.size()) {
        cout << "Lua chon khong hop le. Nhap duong dan thu cong: ";

        string p;
        cin >> p;

        return p;
    }

    return files[pick - 1];
}


// Thuật toán External Sort
// Node dùng trong hàng đợi ưu tiên khi trộn nhiều run
struct Node {
    double val;
    int runId;

    bool operator>(const Node &o) const {
        return val > o.val;
    }
};


// Tạo các run đã được sắp xếp 
static vector<string> generateRuns(const string &inputPath,
                                   const string &tempDir,
                                   size_t MEM_N) {

    ifstream in(inputPath, ios::binary);

    if (!in)
        throw runtime_error("Khong mo duoc input file!");

    vector<string> runs;

    // Buffer trong RAM
    vector<double> buf;
    buf.reserve(MEM_N);

    int runIdx = 0;

    while (true) {

        buf.clear();
        double x;

        // Đọc tối đa MEM_N phần tử vào RAM
        while (buf.size() < MEM_N && readDouble(in, x))
            buf.push_back(x);

        if (buf.empty()) break;

        // Sắp xếp trong RAM
        sort(buf.begin(), buf.end());

        // Ghi ra file run
        string runPath =
            tempDir + "/run_" + to_string(runIdx++) + ".bin";

        ofstream out(runPath, ios::binary);

        for (double v : buf)
            writeDouble(out, v);

        out.close();

        cout << "Tao " << runPath
             << " (size=" << buf.size() << ")\n";

        // In thử vài phần tử đầu
        cout << "        few: ";

        for (size_t i = 0; i < min<size_t>(buf.size(), 8); i++) {
            cout << fixed << setprecision(6)
                 << buf[i]
                 << (i + 1 < min<size_t>(buf.size(), 8) ? ", " : "");
        }

        if (buf.size() > 8) cout << ", ...";

        cout << "\n";

        runs.push_back(runPath);

        if (!in) break;
    }

    return runs;
}


// Trộn các run bằng k-way merge
static void kWayMerge(const vector<string> &runs,
                      const string &outputPath) {

    vector<ifstream> ins(runs.size());

    for (size_t i = 0; i < runs.size(); i++) {
        ins[i].open(runs[i], ios::binary);

        if (!ins[i])
            throw runtime_error(
                "Khong mo duoc run file: " + runs[i]);
    }

    // Hàng đợi ưu tiên để lấy phần tử nhỏ nhất
    priority_queue<Node,
                   vector<Node>,
                   greater<Node>> pq;

    // Đưa phần tử đầu của mỗi run vào heap
    for (size_t i = 0; i < runs.size(); i++) {
        double x;

        if (readDouble(ins[i], x))
            pq.push({x, (int)i});
    }

    ofstream out(outputPath, ios::binary);

    if (!out)
        throw runtime_error("Khong mo duoc output file!");

    cout << "Bat dau tron " << runs.size() << " run...\n";

    uint64_t written = 0;

    while (!pq.empty()) {

        auto cur = pq.top();
        pq.pop();

        writeDouble(out, cur.val);
        written++;

        double nextVal;

        if (readDouble(ins[cur.runId], nextVal))
            pq.push({nextVal, cur.runId});

        // In thử 20 phần tử đầu
        if (written <= 20) {
            cout << "        write #"
                 << written
                 << " = "
                 << fixed << setprecision(6)
                 << cur.val
                 << "\n";
        }
    }

    cout << "Xong! Da ghi "
         << written
         << " double vao "
         << outputPath << "\n";
}


// Hàm điều phối toàn bộ External Sort 
static void externalSort(const string &inputPath,
                         const string &outputPath,
                         size_t MEM_N,
                         const string &tempDir = "temp") {

    fs::create_directories(tempDir);
    fs::create_directories(
        fs::path(outputPath).parent_path());

    cout << "\n=== EXTERNAL SORT (double 8 bytes, binary) ===\n";

    cout << "[Input ] " << inputPath << "\n";
    cout << "[Output] " << outputPath << "\n";
    cout << "[MEM_N ] " << MEM_N
         << " (so double toi da trong RAM)\n";

    cout << "-------------------------------------------\n";

    // Tạo các run
    auto runs = generateRuns(inputPath, tempDir, MEM_N);

    if (runs.empty()) {
        cout << "File input rong.\n";
        return;
    }

    // Trộn các run
    kWayMerge(runs, outputPath);

    cout << "Hoan tat sap xep.\n";
}


int main() {

    ios::sync_with_stdio(false);

    string selectedInput = "";

    while (true) {

        cout << "\n==============================\n";
        cout << "  EXTERNAL SORT DEMO (Console)\n";
        cout << "  Binary file: double (8 bytes)\n";
        cout << "==============================\n";

        cout << "File nguon hien tai: "
             << (selectedInput.empty()
                 ? "(chua chon)"
                 : selectedInput)
             << "\n\n";

        cout << "1) Chon tap tin du lieu nguon trong data/\n";
        cout << "2) Preview file nguon da chon\n";
        cout << "3) Sap xep ngoai (External Sort) file nguon\n";
        cout << "0) Thoat\n";
        cout << "Chon: ";

        int choice;
        cin >> choice;

        if (!cin) break;

        if (choice == 0)
            break;

        if (choice == 1) {

            selectedInput =
                chooseBinFileFromData("data");

            cout << "Da chon file nguon: "
                 << selectedInput << "\n";
        }

        else if (choice == 2) {

            if (selectedInput.empty()) {
                cout << "Chua chon file nguon. "
                     << "Hay chon o muc (1) truoc.\n";
            }
            else {
                previewFile(selectedInput, 10);
            }
        }

        else if (choice == 3) {

            if (selectedInput.empty()) {
                cout << "Chua chon file nguon. "
                     << "Hay chon o muc (1) truoc.\n";
                continue;
            }

            string outPath;
            size_t MEM_N;

            cout << "Nhap output file: ";
            cin >> outPath;

            cout << "Nhap MEM_N: ";
            cin >> MEM_N;

            try {

                externalSort(
                    selectedInput,
                    outPath,
                    MEM_N,
                    "temp");

            } catch (const exception &e) {

                cout << "Khong hop le "
                     << e.what() << "\n";
            }
        }

        else {
            cout << "Lua chon khong hop le.\n";
        }
    }

    cout << "End\n";

    return 0;
}