// stok_xmlrpc_server.cpp - Layanan Stok via XML-RPC (RPC di atas HTTP POST)
// Metode: stok.cek(produk), stok.kurangi(produk, n), stok.tambah(produk, n)
// Implementasi XML-RPC sederhana tanpa library eksternal (port bawaan 8000).
#include <csignal>
#include <thread>
#include <vector>
#include "net.hpp"
#include "stok.hpp"

static Stok stok;

// Ambil isi di antara tag pertama setelah posisi 'dari'
static std::string antara(const std::string& s, const std::string& buka,
                          const std::string& tutup, size_t& dari) {
    size_t a = s.find(buka, dari);
    if (a == std::string::npos) return "";
    a += buka.size();
    size_t b = s.find(tutup, a);
    if (b == std::string::npos) return "";
    dari = b + tutup.size();
    return s.substr(a, b - a);
}

// Hapus tag XML di dalam sebuah string (<string>x</string> -> x)
static std::string buang_tag(const std::string& s) {
    std::string out;
    bool dalam = false;
    for (char c : s) {
        if (c == '<') dalam = true;
        else if (c == '>') dalam = false;
        else if (!dalam) out += c;
    }
    return out;
}

static std::string balasan_ok(int nilai) {
    return "<?xml version=\"1.0\"?><methodResponse><params><param><value><int>" +
           std::to_string(nilai) + "</int></value></param></params></methodResponse>";
}

static std::string balasan_fault(int kode, const std::string& pesan) {
    return "<?xml version=\"1.0\"?><methodResponse><fault><value><struct>"
           "<member><name>faultCode</name><value><int>" + std::to_string(kode) +
           "</int></value></member><member><name>faultString</name><value><string>" +
           pesan + "</string></value></member></struct></value></fault></methodResponse>";
}

static std::string proses(const std::string& xml) {
    size_t pos = 0;
    std::string metode = antara(xml, "<methodName>", "</methodName>", pos);
    std::vector<std::string> arg;
    while (true) {
        std::string v = antara(xml, "<value>", "</value>", pos);
        if (v.empty()) break;
        arg.push_back(buang_tag(v));
    }
    int hasil;
    if (metode == "stok.cek" && arg.size() == 1) {
        hasil = stok.cek(arg[0]);
    } else if ((metode == "stok.kurangi" || metode == "stok.tambah") && arg.size() == 2) {
        int n = atoi(arg[1].c_str());
        hasil = (metode == "stok.kurangi") ? stok.kurangi(arg[0], n) : stok.tambah(arg[0], n);
    } else {
        return balasan_fault(-10, "metode atau parameter tidak dikenal");
    }
    if (hasil >= 0) return balasan_ok(hasil);
    return balasan_fault(hasil, pesan_error(hasil));
}

static void layani(int fd) {
    // Baca header HTTP sampai baris kosong
    std::string baris;
    size_t panjang = 0;
    if (!baca_baris(fd, baris)) { close(fd); return; }   // request line
    while (baca_baris(fd, baris) && !baris.empty()) {
        if (baris.size() > 15 && strncasecmp(baris.c_str(), "Content-Length:", 15) == 0)
            panjang = std::stoul(baris.substr(15));
    }
    std::string body;
    if (!baca_n(fd, body, panjang)) { close(fd); return; }
    std::cout << "[stok-xmlrpc] permintaan " << body.size() << " byte" << std::endl;
    std::string xml = proses(body);
    std::string resp = "HTTP/1.1 200 OK\r\nContent-Type: text/xml\r\nContent-Length: " +
                       std::to_string(xml.size()) + "\r\nConnection: close\r\n\r\n" + xml;
    kirim_semua(fd, resp);
    close(fd);
}

int main(int argc, char** argv) {
    int port = argc > 1 ? std::stoi(argv[1]) : 8000;
    signal(SIGPIPE, SIG_IGN);
    int srv = buat_server(port);
    if (srv < 0) return 1;
    std::cout << "[stok-xmlrpc] mendengarkan di port " << port << std::endl;
    while (true) {
        int c = accept(srv, nullptr, nullptr);
        if (c < 0) continue;
        std::thread(layani, c).detach();
    }
}
