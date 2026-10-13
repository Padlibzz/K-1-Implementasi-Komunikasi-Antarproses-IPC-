# Laporan Tugas Kelompok K-1 — Implementasi Komunikasi Antarproses (IPC)

Mata kuliah: Sistem Operasi (USA-WP2260110) · Kelas D · Tema: Cloud dan Layanan Digital UMKM
Kelompok: [isi nomor/nama kelompok] · Anggota: [nama 1 – NIM], [nama 2 – NIM], [nama 3 – NIM]

## 1. Latar Belakang dan Kasus
UMKM toko kopi memecah aplikasinya menjadi dua layanan: **Layanan Pesanan** dan **Layanan Stok**.
Keduanya proses terpisah (bisa di VM/mesin berbeda), sehingga butuh IPC agar pesanan hanya
diterima jika stok cukup dan stok berkurang saat pesanan diterima.

## 2. Mekanisme IPC yang Dipakai
**Message passing.** Proses tidak berbagi memori; data dikirim sebagai pesan lewat jaringan.
- **Socket TCP (port 9000):** pesan teks satu baris (`CEK`, `KURANGI`, `TAMBAH`), balasan `OK <n>` / `ERR <pesan>`.
- **XML-RPC (port 8000):** pemanggilan prosedur jarak jauh (`stok.cek`, `stok.kurangi`, `stok.tambah`).
  Klien memanggilnya seperti fungsi biasa; pesan dibungkus XML lewat HTTP POST.

**Mengapa bukan shared memory?** Shared memory hanya bekerja untuk proses di satu mesin yang
sama dan memerlukan sinkronisasi manual antarproses. Layanan cloud biasanya berjalan di
mesin/kontainer berbeda, jadi message passing lebih sesuai.

**Sinkronisasi di sisi server:** struktur stok dilindungi `std::mutex` (`stok.hpp`) karena server
melayani banyak klien dengan thread per koneksi. Tanpa mutex, dua pesanan serentak bisa
sama-sama lolos dan stok menjadi negatif (*race condition*).

## 3. Rancangan dan Diagram Alur
Lihat `README.md` (diagram urutan dan diagram blok). Alur pesanan: (1) CEK stok, (2) KURANGI stok,
(3) balas ke pelanggan. Sebutkan juga pembagian berkas: `pesanan.cpp` (klien), dua server, `stok.hpp`, `net.hpp`.

## 4. Implementasi (ringkasan)
| Berkas | Fungsi |
|--------|--------|
| `stok.hpp` | Data stok + mutex, kode error |
| `net.hpp` | Pembungkus socket POSIX (bind/listen/accept/connect/send/recv) |
| `stok_socket_server.cpp` | Server socket, thread per koneksi |
| `stok_xmlrpc_server.cpp` | Server XML-RPC: parse HTTP + XML, balas `methodResponse`/`fault` |
| `pesanan.cpp` | Klien: pilih transport `socket`/`xmlrpc`, alur cek→kurangi |

Catatan jujur: XML-RPC ditulis sendiri dengan parser sederhana (hanya tipe `string` dan `int`),
bukan memakai library penuh. gRPC tidak dipakai karena memerlukan Protocol Buffers dan toolchain
tambahan; ini bisa disebut sebagai pengembangan lanjutan.

## 5. Pengujian dan Hasil
Dijalankan dengan `make test` (keluaran lengkap di `hasil_pengujian.txt`, lampirkan sebagai bukti).
Lingkungan: Debian [versi] di VM [VirtualBox/VMware] pada Windows [versi]; g++ [versi].

| No | Skenario | Harapan | Socket | XML-RPC |
|----|----------|---------|--------|---------|
| 1 | Cek stok kopi | 50 | Lulus | Lulus |
| 2 | Pesan kopi 5 | sisa 45 | Lulus | Lulus |
| 3 | Pesan kopi 100 | ditolak: stok tidak cukup | Lulus | Lulus |
| 4 | Produk tidak ada | ditolak | Lulus | Lulus |
| 5 | Jumlah 0 | ditolak | Lulus | Lulus |
| 6 | Tambah gula 10 | 30 | Lulus | Lulus |
| 7 | 40 pesanan teh serentak (stok 30) | tepat 30 berhasil, 10 ditolak | Lulus | Lulus |
| 8 | Stok teh akhir | 0 (tidak oversell) | Lulus | Lulus |
| 9 | Layanan stok mati | gagal dengan pesan jelas | Lulus | Lulus |

Hasil: 18 dari 18 pengujian lulus. **Ganti/lengkapi tabel ini dengan hasil dari VM kelompok Anda sendiri**
(termasuk waktu respons dari `hasil_pengujian.txt` dan tangkapan layar terminal).

## 6. Analisis
**Kelebihan**
- *Socket:* ringan dan cepat (tanpa overhead XML/HTTP), kendali penuh atas protokol, tanpa dependensi.
- *XML-RPC:* klien memanggil fungsi jarak jauh tanpa memikirkan format byte; format standar,
  mudah dibaca, dan bisa dites dengan `curl`; ada fault code standar untuk galat.
- *Message passing umum:* proses terisolasi (kegagalan satu layanan tidak merusak memori layanan
  lain), mudah dipindah ke mesin/cloud lain, cocok untuk arsitektur layanan UMKM yang tumbuh.

**Keterbatasan**
- *Socket:* protokol harus dirancang dan di-parse sendiri; mudah salah (pesan terpotong, format);
  tidak ada enkripsi/autentikasi bawaan.
- *XML-RPC:* pesan lebih besar dan lebih lambat karena XML+HTTP; satu koneksi per panggilan pada
  implementasi ini; tipe data terbatas.
- *Umum:* latensi jaringan lebih tinggi daripada shared memory; harus menangani kegagalan jaringan
  dan layanan mati (uji 9); pola cek-lalu-kurangi tidak atomik di sisi klien, namun aman karena
  pengurangan stok atomik di server (uji 7). Stok disimpan di memori sehingga hilang saat server restart
  (produksi perlu basis data).

**Perbandingan ringkas pada kasus kelas**

| Aspek | Socket | XML-RPC |
|-------|--------|---------|
| Kecepatan | Lebih cepat | Sedikit lebih lambat |
| Kemudahan pakai | Perlu rancang protokol | Seperti panggil fungsi |
| Interoperabilitas | Hanya jika protokol disepakati | Standar, lintas bahasa |
| Cocok untuk | Komunikasi internal ringan | Integrasi antar layanan |

**Kesimpulan:** untuk layanan internal UMKM berskala kecil, socket cukup dan efisien; bila layanan
akan dihubungkan ke banyak sistem/bahasa lain, RPC standar lebih praktis.

## 7. Lembar Kontribusi Anggota
| Nama | NIM | Peran | Tugas yang dikerjakan | Persentase |
|------|-----|-------|-----------------------|-----------|
| [ ] | [ ] | Pemrogram server | [ ] | [ ]% |
| [ ] | [ ] | Pemrogram klien | [ ] | [ ]% |
| [ ] | [ ] | Penguji | [ ] | [ ]% |
| [ ] | [ ] | Dokumentasi | [ ] | [ ]% |

## Daftar Pustaka
Gunakan Pustaka [1], [4], [7] dari RPS/Modul Pembelajaran Sistem Operasi sesuai penomoran dosen.
