// net.hpp - fungsi bantu socket TCP, portabel: Linux/Debian (POSIX) dan Windows (Winsock)
#pragma once
#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>

#ifdef _WIN32
  #ifndef _WIN32_WINNT
  #define _WIN32_WINNT 0x0601
  #endif
  #include <winsock2.h>
  #include <ws2tcpip.h>
  // Winsock harus diinisialisasi sekali sebelum socket dipakai
  struct WinsockInit {
      WinsockInit()  { WSADATA w; WSAStartup(MAKEWORD(2, 2), &w); }
      ~WinsockInit() { WSACleanup(); }
  };
  inline WinsockInit winsock_init_;
#else
  #include <arpa/inet.h>
  #include <netdb.h>
  #include <netinet/in.h>
  #include <strings.h>
  #include <sys/socket.h>
  #include <unistd.h>
#endif

// Tutup socket (close di Linux, closesocket di Windows)
inline void tutup(int fd) {
#ifdef _WIN32
    closesocket((SOCKET)fd);
#else
    close(fd);
#endif
}

// Terima paling banyak n byte; <=0 berarti koneksi putus/galat
inline int terima(int fd, char* buf, int n) {
    return (int)recv(fd, buf, n, 0);
}

// Buat socket server yang mendengarkan di port; kembalikan fd atau -1
inline int buat_server(int port) {
    int fd = (int)socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_ANY);
    a.sin_port = htons(port);
    if (bind(fd, (sockaddr*)&a, sizeof(a)) < 0 || listen(fd, 64) < 0) {
        perror("bind/listen");
        tutup(fd);
        return -1;
    }
    return fd;
}

// Sambung ke server (host berupa IP atau nama); kembalikan fd atau -1
inline int sambung(const std::string& host, int port) {
    addrinfo h{}, *r = nullptr;
    h.ai_family = AF_INET;
    h.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &h, &r) != 0) return -1;
    int fd = (int)socket(r->ai_family, r->ai_socktype, r->ai_protocol);
    if (fd < 0 || connect(fd, r->ai_addr, (int)r->ai_addrlen) < 0) {
        if (fd >= 0) tutup(fd);
        freeaddrinfo(r);
        return -1;
    }
    freeaddrinfo(r);
    return fd;
}

inline bool kirim_semua(int fd, const std::string& s) {
    size_t sent = 0;
    while (sent < s.size()) {
        int n = (int)send(fd, s.data() + sent, (int)(s.size() - sent), 0);
        if (n <= 0) return false;
        sent += n;
    }
    return true;
}

// Baca satu baris (sampai '\n'); false jika koneksi ditutup
inline bool baca_baris(int fd, std::string& out) {
    out.clear();
    char c;
    while (true) {
        int n = terima(fd, &c, 1);
        if (n <= 0) return false;
        if (c == '\n') break;
        if (c != '\r') out += c;
    }
    return true;
}

// Baca tepat n byte
inline bool baca_n(int fd, std::string& out, size_t n) {
    out.clear();
    char buf[1024];
    while (out.size() < n) {
        int k = terima(fd, buf, (int)std::min(sizeof(buf), n - out.size()));
        if (k <= 0) return false;
        out.append(buf, k);
    }
    return true;
}
