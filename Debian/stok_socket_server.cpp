// stok_socket_server.cpp - Layanan Stok via socket TCP (message passing)
// Protokol teks, satu perintah per baris:
//   CEK <produk>             -> OK <stok> | ERR <pesan>
//   KURANGI <produk> <n>     -> OK <stok_baru> | ERR <pesan>
//   TAMBAH <produk> <n>      -> OK <stok_baru> | ERR <pesan>
// Port bawaan 9000. Satu thread per koneksi klien.
#include <csignal>
#include <sstream>
#include <thread>
#include "net.hpp"
#include "stok.hpp"

static Stok stok;

static std::string proses(const std::string& baris) {
    std::istringstream in(baris);
    std::string cmd, produk;
    int n = 0;
    in >> cmd >> produk;
    int hasil;
    if (cmd == "CEK") {
        hasil = stok.cek(produk);
    } else if (cmd == "KURANGI" || cmd == "TAMBAH") {
        if (!(in >> n)) return "ERR jumlah tidak valid\n";
        hasil = (cmd == "KURANGI") ? stok.kurangi(produk, n) : stok.tambah(produk, n);
    } else {
        return "ERR perintah tidak dikenal\n";
    }
    if (hasil >= 0) return "OK " + std::to_string(hasil) + "\n";
    return "ERR " + pesan_error(hasil) + "\n";
}

static void layani(int fd) {
    std::string baris;
    while (baca_baris(fd, baris)) {
        std::cout << "[stok-socket] terima: " << baris << std::endl;
        std::string jawab = proses(baris);
        if (!kirim_semua(fd, jawab)) break;
    }
    close(fd);
}

int main(int argc, char** argv) {
    int port = argc > 1 ? std::stoi(argv[1]) : 9000;
    signal(SIGPIPE, SIG_IGN);
    int srv = buat_server(port);
    if (srv < 0) return 1;
    std::cout << "[stok-socket] mendengarkan di port " << port << std::endl;
    while (true) {
        int c = accept(srv, nullptr, nullptr);
        if (c < 0) continue;
        std::thread(layani, c).detach();
    }
}
