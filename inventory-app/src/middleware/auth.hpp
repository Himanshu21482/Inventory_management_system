#pragma once

#include "../utils/password.hpp"

#include <chrono>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <utility>

namespace inventory {
struct Principal { int id{}; std::string name; std::string email; std::string role; };

class TokenStore {
  public:
    std::string issue(Principal principal) {
        std::random_device random;
        static constexpr char alphabet[] = "0123456789abcdef";
        std::string nonce(64, '0');
        for (auto &ch : nonce) ch = alphabet[random() % 16U];
        const auto token = nonce + "." + password::hmacSha256(secret_, nonce);
        std::lock_guard<std::mutex> lock(mutex_);
        tokens_[nonce] = Entry{std::move(principal), std::chrono::system_clock::now() + std::chrono::hours(24)};
        return token;
    }

    std::optional<Principal> find(const std::string &token) {
        const auto separator = token.find('.');
        if (separator == std::string::npos) return std::nullopt;
        const auto nonce = token.substr(0, separator);
        const auto signature = token.substr(separator + 1);
        if (!constantTimeEqual(signature, password::hmacSha256(secret_, nonce))) return std::nullopt;
        std::lock_guard<std::mutex> lock(mutex_);
        const auto found = tokens_.find(nonce);
        if (found == tokens_.end()) return std::nullopt;
        if (found->second.expires <= std::chrono::system_clock::now()) {
            tokens_.erase(found);
            return std::nullopt;
        }
        return found->second.principal;
    }

  private:
    static bool constantTimeEqual(const std::string &left, const std::string &right) {
        if (left.size() != right.size()) return false;
        unsigned char difference = 0;
        for (std::size_t i = 0; i < left.size(); ++i)
            difference |= static_cast<unsigned char>(left[i] ^ right[i]);
        return difference == 0;
    }

    struct Entry { Principal principal; std::chrono::system_clock::time_point expires; };
    const std::string secret_ = password::randomSalt();
    std::mutex mutex_;
    std::unordered_map<std::string, Entry> tokens_;
};

inline std::string bearerToken(const std::string &header) {
    constexpr char prefix[] = "Bearer ";
    return header.compare(0, sizeof(prefix) - 1, prefix) == 0 ? header.substr(sizeof(prefix) - 1) : "";
}
} // namespace inventory
