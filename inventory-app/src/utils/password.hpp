#pragma once

#include <array>
#include <cstdint>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace inventory::password {
inline std::uint32_t rotateRight(std::uint32_t value, unsigned count) {
    return (value >> count) | (value << (32U - count));
}

inline std::string randomSalt() {
    std::random_device random;
    constexpr char alphabet[] = "0123456789abcdef";
    std::string salt(32, '0');
    for (auto &ch : salt) ch = alphabet[random() % 16U];
    return salt;
}

inline std::string sha256(const std::string &input) {
    constexpr std::array<std::uint32_t, 64> k = {
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
    std::vector<std::uint8_t> bytes(input.begin(), input.end());
    const auto bitLength = static_cast<std::uint64_t>(bytes.size()) * 8U;
    bytes.push_back(0x80);
    while (bytes.size() % 64 != 56) bytes.push_back(0);
    for (int shift = 56; shift >= 0; shift -= 8) bytes.push_back(static_cast<std::uint8_t>(bitLength >> shift));
    std::array<std::uint32_t, 8> h = {0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,
                                      0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    for (std::size_t offset = 0; offset < bytes.size(); offset += 64) {
        std::array<std::uint32_t, 64> w{};
        for (std::size_t i = 0; i < 16; ++i) {
            const auto pos = offset + i * 4;
            w[i] = (static_cast<std::uint32_t>(bytes[pos]) << 24U) |
                   (static_cast<std::uint32_t>(bytes[pos + 1]) << 16U) |
                   (static_cast<std::uint32_t>(bytes[pos + 2]) << 8U) | bytes[pos + 3];
        }
        for (std::size_t i = 16; i < 64; ++i) {
            const auto s0 = rotateRight(w[i-15], 7) ^ rotateRight(w[i-15], 18) ^ (w[i-15] >> 3U);
            const auto s1 = rotateRight(w[i-2], 17) ^ rotateRight(w[i-2], 19) ^ (w[i-2] >> 10U);
            w[i] = w[i-16] + s0 + w[i-7] + s1;
        }
        auto a=h[0], b=h[1], c=h[2], d=h[3], e=h[4], f=h[5], g=h[6], x=h[7];
        for (std::size_t i = 0; i < 64; ++i) {
            const auto s1 = rotateRight(e,6) ^ rotateRight(e,11) ^ rotateRight(e,25);
            const auto ch = (e & f) ^ (~e & g);
            const auto t1 = x + s1 + ch + k[i] + w[i];
            const auto s0 = rotateRight(a,2) ^ rotateRight(a,13) ^ rotateRight(a,22);
            const auto maj = (a & b) ^ (a & c) ^ (b & c);
            const auto t2 = s0 + maj;
            x=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
        }
        h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d; h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=x;
    }
    std::ostringstream output;
    output << std::hex << std::setfill('0');
    for (const auto word : h) output << std::setw(8) << word;
    return output.str();
}

inline std::string hmacSha256(const std::string &key, const std::string &message) {
    std::string keyBlock = key;
    if (keyBlock.size() > 64) keyBlock = sha256(keyBlock);
    keyBlock.resize(64, '\0');
    std::string innerPad(64, '\0');
    std::string outerPad(64, '\0');
    for (std::size_t i = 0; i < 64; ++i) {
        innerPad[i] = static_cast<char>(static_cast<unsigned char>(keyBlock[i]) ^ 0x36U);
        outerPad[i] = static_cast<char>(static_cast<unsigned char>(keyBlock[i]) ^ 0x5cU);
    }
    const auto innerHex = sha256(innerPad + message);
    std::string innerDigest;
    innerDigest.reserve(32);
    for (std::size_t i = 0; i < innerHex.size(); i += 2) {
        const auto byte = static_cast<unsigned char>(std::stoul(innerHex.substr(i, 2), nullptr, 16));
        innerDigest.push_back(static_cast<char>(byte));
    }
    return sha256(outerPad + innerDigest);
}

// Salted SHA-256 keeps this small project dependency-free. Production passwords should use Argon2 or bcrypt.
inline std::string hash(const std::string &salt, const std::string &plain) { return sha256(salt + plain); }
} // namespace inventory::password
