// pesanan.cpp - Layanan Pesanan (klien) yang memanggil Layanan Stok
// Pemakaian:
//   ./pesanan <socket|xmlrpc> cek <produk>
//   ./pesanan <socket|xmlrpc> pesan <produk> <jumlah>
//   ./pesanan <socket|xmlrpc> tambah <produk> <jumlah>
// Opsional: variabel lingkungan STOK_HOST (bawaan 127.0.0.1)
// Alur "pesan": cek stok dulu, lalu kurangi stok. Keluaran satu baris
// supaya mudah dibaca skrip uji.
#include <chrono>
#include <cstdlib>
#include "net.hpp"
#include "stok.hpp"

static std::string host() {
    const char* h = getenv("STOK_HOST");
    return h ? h : "127.0.0.1";
}

// ---------- Transport 1: socket (pesan teks per baris) ----------
static int via_socket(const std::string& perintah, std::string& err) {
    int fd = sambung(host(), 9000);
    if (fd < 0) { err = "tidak bisa terhubung ke layanan stok"; return -99; }
    kirim_semua(fd, perintah + "\n");
    std::string jawab;
    bool ok = baca_baris(fd, jawab);
    tutup(fd);
    if (!ok) { err = "tidak ada balasan"; return -99; }
    if (jawab.rfind("OK ", 0) == 0) return std::stoi(jawab.substr(3));
    err = jawab.substr(4);
    return -1;
}

// ---------- Transport 2: XML-RPC di atas HTTP ----------
static int via_xmlrpc(const std::string& metode, const std::string& produk,
                      int n, bool pakai_n, std::string& err) {
    std::string xml = "<?xml version=\"1.0\"?><methodCall><methodName>" + metode +
                      "</methodName><params><param><value><string>" + produk +
                      "</string></value></param>";
    if (pakai_n)
        xml += "<param><value><int>" + std::to_string(n) + "</int></value></param>";
    xml += "</params></methodCall>";
    int fd = sambung(host(), 8000);
    if (fd < 0) { err = "tidak bisa terhubung ke layanan stok"; return -99; }
    std::string req = "POST /RPC2 HTTP/1.1\r\nHost: " + host() +
                      "\r\nContent-Type: text/xml\r\nContent-Length: " +
                      std::to_string(xml.size()) + "\r\nConnection: close\r\n\r\n" + xml;
    kirim_semua(fd, req);
    std::string resp;
    char buf[1024];
    int k;
    while ((k = terima(fd, buf, sizeof(buf))) > 0) resp.append(buf, k);
    tutup(fd);
    size_t f = resp.find("<fault>");
    if (f != std::string::npos) {
        f = resp.find("faultString", f);
        size_t a = resp.find("<string>", f) + 8, b = resp.find("</string>", a);
        err = resp.substr(a, b - a);
        return -1;
    }
    size_t a = resp.find("<int>");
    if (a == std::string::npos) { err = "balasan tidak valid"; return -99; }
    return std::stoi(resp.substr(a + 5));
}

// Satu antarmuka untuk dua transport
struct Klien {
    bool rpc;
    int panggil(const std::string& op, const std::string& p, int n, std::string& err) {
        if (rpc) {
            if (op == "CEK") return via_xmlrpc("stok.cek", p, 0, false, err);
            return via_xmlrpc(op == "KURANGI" ? "stok.kurangi" : "stok.tambah", p, n, true, err);
        }
        std::string cmd = op + " " + p;
        if (op != "CEK") cmd += " " + std::to_string(n);
        return via_socket(cmd, err);
    }
};

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Pemakaian: " << argv[0]
                  << " <socket|xmlrpc> <cek|pesan|tambah> <produk> [jumlah]\n";
        return 2;
    }
    std::string mode = argv[1], aksi = argv[2], produk = argv[3];
    int n = argc > 4 ? atoi(argv[4]) : 0;
    Klien k{mode == "xmlrpc"};
    std::string err;
    auto t0 = std::chrono::steady_clock::now();
    int hasil;

    if (aksi == "cek") {
        hasil = k.panggil("CEK", produk, 0, err);
        if (hasil >= 0) std::cout << "OK stok " << produk << " = " << hasil;
    } else if (aksi == "tambah") {
        hasil = k.panggil("TAMBAH", produk, n, err);
        if (hasil >= 0) std::cout << "OK stok " << produk << " menjadi " << hasil;
    } else if (aksi == "pesan") {
        int ada = k.panggil("CEK", produk, 0, err);          // langkah 1: cek
        if (ada < 0) { hasil = ada; }
        else {
            hasil = k.panggil("KURANGI", produk, n, err);    // langkah 2: kurangi
            if (hasil >= 0)
                std::cout << "OK pesanan " << produk << " x" << n
                          << " diterima, sisa stok " << hasil;
        }
    } else {
        std::cerr << "aksi tidak dikenal\n";
        return 2;
    }
    auto ms = std::chrono::duration<double, std::milli>(
                  std::chrono::steady_clock::now() - t0).count();
    if (hasil < 0) std::cout << "GAGAL " << err;
    std::cout << " (" << mode << ", " << ms << " ms)" << std::endl;
    return hasil >= 0 ? 0 : 1;
}
