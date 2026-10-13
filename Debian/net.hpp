// net.hpp - fungsi bantu socket POSIX (TCP) yang dipakai semua program
#pragma once
#include <arpa/inet.h>
#include <algorithm>
#include <strings.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <string>

// Buat socket server yang mendengarkan di port; kembalikan fd atau -1
inline int buat_server(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_ANY);
    a.sin_port = htons(port);
    if (bind(fd, (sockaddr*)&a, sizeof(a)) < 0 || listen(fd, 64) < 0) {
        perror("bind/listen");
        close(fd);
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
    int fd = socket(r->ai_family, r->ai_socktype, r->ai_protocol);
    if (fd < 0 || connect(fd, r->ai_addr, r->ai_addrlen) < 0) {
        if (fd >= 0) close(fd);
        freeaddrinfo(r);
        return -1;
    }
    freeaddrinfo(r);
    return fd;
}

inline bool kirim_semua(int fd, const std::string& s) {
    size_t sent = 0;
    while (sent < s.size()) {
        ssize_t n = send(fd, s.data() + sent, s.size() - sent, 0);
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
        ssize_t n = recv(fd, &c, 1, 0);
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
        ssize_t k = recv(fd, buf, std::min(sizeof(buf), n - out.size()), 0);
        if (k <= 0) return false;
        out.append(buf, k);
    }
    return true;
}
