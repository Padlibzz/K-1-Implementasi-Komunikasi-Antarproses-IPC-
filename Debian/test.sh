#!/usr/bin/env bash
# test.sh - pengujian otomatis IPC Pesanan <-> Stok (socket dan XML-RPC)
# Hasil disimpan ke hasil_pengujian.txt (bisa langsung dilampirkan di laporan).
cd "$(dirname "$0")"
make -s all || exit 1
OUT=hasil_pengujian.txt
: > "$OUT"
LULUS=0; GAGAL=0

log() { echo "$*" | tee -a "$OUT"; }

# uji <nama> <teks yang diharapkan> <perintah...>
uji() {
  local nama="$1" harap="$2"; shift 2
  local hasil; hasil=$("$@" 2>&1)
  if [[ "$hasil" == *"$harap"* ]]; then LULUS=$((LULUS+1)); st="LULUS"
  else GAGAL=$((GAGAL+1)); st="GAGAL"; fi
  log "[$st] $nama"
  log "       perintah : $*"
  log "       keluaran : $hasil"
}

mulai_server() {   # $1 = socket|xmlrpc
  ./stok_${1}_server > "server_$1.log" 2>&1 &
  SPID=$!
  sleep 0.5
}
henti_server() { kill "$SPID" 2>/dev/null; wait "$SPID" 2>/dev/null; }

log "=== HASIL PENGUJIAN IPC PESANAN <-> STOK ==="
log "Tanggal : $(date '+%Y-%m-%d %H:%M:%S')"
log "Mesin   : $(uname -sr)"
log ""

for M in socket xmlrpc; do
  log "---------- Mode: $M ----------"
  mulai_server $M
  P="./pesanan $M"
  uji "1. Cek stok awal kopi (50)"            "stok kopi = 50"            $P cek kopi
  uji "2. Pesan kopi 5 (sisa 45)"             "sisa stok 45"              $P pesan kopi 5
  uji "3. Stok tidak cukup (kopi 100)"        "GAGAL stok tidak cukup"    $P pesan kopi 100
  uji "4. Produk tidak ada (sirup)"           "GAGAL produk tidak ditemukan" $P pesan sirup 1
  uji "5. Jumlah tidak valid (0)"             "GAGAL jumlah tidak valid"  $P pesan kopi 0
  uji "6. Tambah stok gula 10 (jadi 30)"      "menjadi 30"                $P tambah gula 10

  # Uji konkurensi: 40 pesanan serentak untuk teh (stok 30) -> tepat 30 berhasil
  log "[....] 7. Konkurensi: 40 pesanan teh x1 serentak (stok 30)"
  for i in $(seq 1 40); do $P pesan teh 1 > "/tmp/ipc_k_$i.txt" 2>&1 & done
  wait $(jobs -p | grep -v "^$SPID$") 2>/dev/null
  sleep 0.5
  OKN=$(grep -l "^OK" /tmp/ipc_k_*.txt 2>/dev/null | wc -l)
  GGL=$(grep -l "^GAGAL" /tmp/ipc_k_*.txt 2>/dev/null | wc -l)
  rm -f /tmp/ipc_k_*.txt
  if [[ "$OKN" == 30 && "$GGL" == 10 ]]; then LULUS=$((LULUS+1)); st="LULUS"; else GAGAL=$((GAGAL+1)); st="GAGAL"; fi
  log "[$st] 7. hasil: berhasil=$OKN, ditolak=$GGL (harapan 30 dan 10)"
  uji "8. Stok teh akhir harus 0 (tidak oversell)" "stok teh = 0" $P cek teh

  henti_server
  uji "9. Layanan stok mati -> pesanan gagal rapi" "GAGAL tidak bisa terhubung" $P pesan kopi 1
  log ""
done

log "RINGKASAN: $LULUS lulus, $GAGAL gagal"
[[ $GAGAL -eq 0 ]]
