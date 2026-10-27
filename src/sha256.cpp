#include "tradeup/catalog_io.hpp"

#include <array>
#include <bit>
#include <limits>

namespace tradeup {
std::string sha256(const std::string_view bytes) {
    if (bytes.size() > std::numeric_limits<std::uint64_t>::max() / 8) {
        throw std::overflow_error("SHA-256 input bit length exceeds uint64");
    }
    constexpr std::array<std::uint32_t, 64> k{
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
    std::array<std::uint32_t, 8> h{0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    const auto block = [&](const unsigned char* data) {
        std::array<std::uint32_t, 64> w{};
        for (std::size_t i = 0; i < 16; ++i) {
            const auto j = 4*i;
            w[i] = (static_cast<std::uint32_t>(data[j]) << 24) | (static_cast<std::uint32_t>(data[j+1]) << 16)
                | (static_cast<std::uint32_t>(data[j+2]) << 8) | data[j+3];
        }
        for (std::size_t i = 16; i < 64; ++i) {
            const auto s0 = std::rotr(w[i-15],7) ^ std::rotr(w[i-15],18) ^ (w[i-15] >> 3);
            const auto s1 = std::rotr(w[i-2],17) ^ std::rotr(w[i-2],19) ^ (w[i-2] >> 10);
            w[i] = w[i-16] + s0 + w[i-7] + s1;
        }
        auto a=h[0], b=h[1], c=h[2], d=h[3], e=h[4], f=h[5], g=h[6], z=h[7];
        for (std::size_t i = 0; i < 64; ++i) {
            const auto s1 = std::rotr(e,6) ^ std::rotr(e,11) ^ std::rotr(e,25);
            const auto ch = (e & f) ^ (~e & g);
            const auto t1 = z + s1 + ch + k[i] + w[i];
            const auto s0 = std::rotr(a,2) ^ std::rotr(a,13) ^ std::rotr(a,22);
            const auto maj = (a & b) ^ (a & c) ^ (b & c);
            const auto t2 = s0 + maj;
            z=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
        }
        h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d; h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=z;
    };
    const auto full = bytes.size() / 64;
    for (std::size_t i = 0; i < full; ++i) { block(reinterpret_cast<const unsigned char*>(bytes.data()+i*64)); }
    std::array<unsigned char, 128> tail{};
    const auto remaining = bytes.size() % 64;
    for (std::size_t i = 0; i < remaining; ++i) { tail[i] = static_cast<unsigned char>(bytes[full*64+i]); }
    tail[remaining] = 0x80;
    const std::size_t length = remaining < 56 ? 64 : 128;
    const auto bits = static_cast<std::uint64_t>(bytes.size()) * 8;
    for (std::size_t i = 0; i < 8; ++i) { tail[length-1-i] = static_cast<unsigned char>(bits >> (8*i)); }
    block(tail.data()); if (length == 128) { block(tail.data()+64); }
    constexpr char hex[] = "0123456789abcdef";
    std::string result;
    for (const auto word : h) {
        for (int shift = 28; shift >= 0; shift -= 4) { result += hex[(word >> shift) & 15U]; }
    }
    return result;
}
} // namespace tradeup
