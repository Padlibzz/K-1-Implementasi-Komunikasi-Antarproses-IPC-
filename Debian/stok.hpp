// stok.hpp - data stok bersama (dipakai server socket dan server XML-RPC)
// Semua akses dilindungi mutex supaya aman saat banyak pesanan masuk bersamaan.
#pragma once
#include <map>
#include <mutex>
#include <string>

// Kode hasil: >=0 berarti sukses (nilai = stok terbaru)
const int E_TIDAK_ADA = -1;   // produk tidak ditemukan
const int E_KURANG    = -2;   // stok tidak cukup
const int E_JUMLAH    = -3;   // jumlah tidak valid

struct Stok {
    std::map<std::string, int> d{{"kopi", 50}, {"teh", 30}, {"gula", 20}};
    std::mutex m;

    int cek(const std::string& p) {
        std::lock_guard<std::mutex> l(m);
        auto i = d.find(p);
        return i == d.end() ? E_TIDAK_ADA : i->second;
    }
    int kurangi(const std::string& p, int n) {
        std::lock_guard<std::mutex> l(m);
        auto i = d.find(p);
        if (i == d.end()) return E_TIDAK_ADA;
        if (n <= 0) return E_JUMLAH;
        if (i->second < n) return E_KURANG;
        i->second -= n;
        return i->second;
    }
    int tambah(const std::string& p, int n) {
        std::lock_guard<std::mutex> l(m);
        auto i = d.find(p);
        if (i == d.end()) return E_TIDAK_ADA;
        if (n <= 0) return E_JUMLAH;
        i->second += n;
        return i->second;
    }
};

inline std::string pesan_error(int kode) {
    switch (kode) {
        case E_TIDAK_ADA: return "produk tidak ditemukan";
        case E_KURANG:    return "stok tidak cukup";
        case E_JUMLAH:    return "jumlah tidak valid";
    }
    return "kesalahan tidak dikenal";
}
