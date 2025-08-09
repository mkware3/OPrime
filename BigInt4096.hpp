#ifndef BIGINT4096_HPP
#define BIGINT4096_HPP

#include <array>
#include <cstdint>
#include <string>
#include <iostream>
#include <stdexcept>
#include <algorithm>

class BigInt4096 {
private:
    static constexpr size_t NUM_WORDS = 64;
    std::array<uint64_t, NUM_WORDS> data{};

public:
    // Constructors
    BigInt4096() { data.fill(0); }
    BigInt4096(uint64_t value) { data.fill(0); data[0] = value; }
    BigInt4096(const std::string& decimal) {
        data.fill(0);
        BigInt4096 base(10);
        for (char c : decimal) {
            if (c < '0' || c > '9') continue;
            *this *= base;
            *this += BigInt4096(c - '0');
        }
    }

    // Arithmetic operators
    BigInt4096 operator+(const BigInt4096& rhs) const {
        BigInt4096 res;
        uint64_t carry = 0;
        for (size_t i = 0; i < NUM_WORDS; ++i) {
            __uint128_t sum = (__uint128_t)data[i] + rhs.data[i] + carry;
            res.data[i] = (uint64_t)sum;
            carry = (uint64_t)(sum >> 64);
        }
        return res;
    }
    BigInt4096& operator+=(const BigInt4096& rhs) { *this = *this + rhs; return *this; }

    BigInt4096 operator-(const BigInt4096& rhs) const {
        BigInt4096 res;
        uint64_t borrow = 0;
        for (size_t i = 0; i < NUM_WORDS; ++i) {
            __uint128_t sub = (__uint128_t)data[i] - rhs.data[i] - borrow;
            res.data[i] = (uint64_t)sub;
            borrow = (sub >> 127) & 1;
        }
        return res;
    }
    BigInt4096& operator-=(const BigInt4096& rhs) { *this = *this - rhs; return *this; }

    BigInt4096 operator*(const BigInt4096& rhs) const {
        BigInt4096 res;
        for (size_t i = 0; i < NUM_WORDS; ++i) {
            if (data[i] == 0) continue;
            uint64_t carry = 0;
            for (size_t j = 0; j + i < NUM_WORDS; ++j) {
                __uint128_t mul = (__uint128_t)data[i] * rhs.data[j] + res.data[i + j] + carry;
                res.data[i + j] = (uint64_t)mul;
                carry = (uint64_t)(mul >> 64);
            }
        }
        return res;
    }
    BigInt4096& operator*=(const BigInt4096& rhs) { *this = *this * rhs; return *this; }

    BigInt4096 operator/(const BigInt4096& rhs) const {
        if (rhs == 0) throw std::runtime_error("Modulo by zero");
        BigInt4096 dividend = *this;
        BigInt4096 divisor = rhs, quotient, current;
        for (int i = NUM_WORDS * 64 - 1; i >= 0; --i) {
            current = current << 1;
            current.data[0] |= (dividend.data[i / 64] >> (i % 64)) & 1;
            if (current >= divisor) {
                current -= divisor;
                quotient.data[i / 64] |= (1ULL << (i % 64));
            }
        }
        return quotient;
    }
    BigInt4096& operator/=(const BigInt4096& rhs) { *this = *this / rhs; return *this; }

    BigInt4096 operator%(const BigInt4096& rhs) const {
        if (rhs == 0) throw std::runtime_error("Modulo by zero");
        BigInt4096 dividend = *this;
        BigInt4096 divisor = rhs, current;
        for (int i = NUM_WORDS * 64 - 1; i >= 0; --i) {
            current = current << 1;
            current.data[0] |= (dividend.data[i / 64] >> (i % 64)) & 1;
            if (current >= divisor)
                current -= divisor;
        }
        return current;
    }
    BigInt4096& operator%=(const BigInt4096& rhs) { *this = *this % rhs; return *this; }

    // Bitwise
    BigInt4096 operator&(const BigInt4096& rhs) const {
        BigInt4096 res;
        for (size_t i = 0; i < NUM_WORDS; ++i)
            res.data[i] = data[i] & rhs.data[i];
        return res;
    }
    BigInt4096 operator|(const BigInt4096& rhs) const {
        BigInt4096 res;
        for (size_t i = 0; i < NUM_WORDS; ++i)
            res.data[i] = data[i] | rhs.data[i];
        return res;
    }
    BigInt4096 operator^(const BigInt4096& rhs) const {
        BigInt4096 res;
        for (size_t i = 0; i < NUM_WORDS; ++i)
            res.data[i] = data[i] ^ rhs.data[i];
        return res;
    }
    BigInt4096 operator~() const {
        BigInt4096 res;
        for (size_t i = 0; i < NUM_WORDS; ++i)
            res.data[i] = ~data[i];
        return res;
    }

    // Shifts
    BigInt4096 operator<<(size_t shift) const {
        BigInt4096 res;
        size_t wordShift = shift / 64;
        size_t bitShift = shift % 64;
        for (int i = NUM_WORDS - 1; i >= 0; --i) {
            if ((size_t)i < wordShift) continue;
            uint64_t lower = data[i - wordShift] << bitShift;
            uint64_t upper = 0;
            if (bitShift && i >= (int)(wordShift + 1))
                upper = data[i - wordShift - 1] >> (64 - bitShift);
            res.data[i] = lower | upper;
        }
        return res;
    }
    BigInt4096 operator>>(size_t shift) const {
        BigInt4096 res;
        size_t wordShift = shift / 64;
        size_t bitShift = shift % 64;
        for (size_t i = 0; i < NUM_WORDS; ++i) {
            if (i + wordShift >= NUM_WORDS) continue;
            uint64_t upper = data[i + wordShift] >> bitShift;
            uint64_t lower = 0;
            if (bitShift && i + wordShift + 1 < NUM_WORDS)
                lower = data[i + wordShift + 1] << (64 - bitShift);
            res.data[i] = upper | lower;
        }
        return res;
    }

    // Comparison
    bool operator==(const BigInt4096& rhs) const { return data == rhs.data; }
    bool operator!=(const BigInt4096& rhs) const { return !(*this == rhs); }
    bool operator<(const BigInt4096& rhs) const {
        for (int i = NUM_WORDS - 1; i >= 0; --i) {
            if (data[i] < rhs.data[i]) return true;
            if (data[i] > rhs.data[i]) return false;
        }
        return false;
    }
    bool operator>(const BigInt4096& rhs) const { return rhs < *this; }
    bool operator<=(const BigInt4096& rhs) const { return !(rhs < *this); }
    bool operator>=(const BigInt4096& rhs) const { return !(*this < rhs); }

    // I/O
    friend std::ostream& operator<<(std::ostream& os, const BigInt4096& bi) {
        if (bi == BigInt4096(0)) return os << "0";
        BigInt4096 temp = bi;
        BigInt4096 zero(0), ten(10);
        std::string out;
        while (temp != zero) {
            BigInt4096 digit = temp % ten;
            out += '0' + digit.data[0];
            temp /= ten;
        }
        std::reverse(out.begin(), out.end());
        return os << out;
    }
    friend std::istream& operator>>(std::istream& is, BigInt4096& bi) {
        std::string str;
        is >> str;
        bi = BigInt4096(str);
        return is;
    }

    // Cast
    explicit operator bool() const {
        for (auto w : data) if (w != 0) return true;
        return false;
    }

    // Static: Modular exponentiation
    static BigInt4096 modExp(BigInt4096 base, BigInt4096 exp, const BigInt4096& mod) {
        if (mod == BigInt4096(0)) throw std::runtime_error("modulo by zero");
        BigInt4096 result(1);
        base %= mod;
        while (exp != BigInt4096(0)) {
            if (exp.data[0] & 1) result = (result * base) % mod;
            exp = exp >> 1;
            base = (base * base) % mod;
        }
        return result;
    }

    // Static: Miller-Rabin primality test
    static bool isPrime(const BigInt4096& n, int rounds = 5) {
        if (n <= BigInt4096(1)) return false;
        if (n == BigInt4096(2) || n == BigInt4096(3)) return true;
        if ((n.data[0] & 1) == 0) return false;
        BigInt4096 d = n - BigInt4096(1);
        int r = 0;
        while ((d.data[0] & 1) == 0) {
            d = d >> 1;
            ++r;
        }
        const int basePrimes[5] = {2, 3, 5, 7, 11};
        for (int i = 0; i < rounds; ++i) {
            BigInt4096 a(basePrimes[i]);
            BigInt4096 x = modExp(a, d, n);
            if (x == BigInt4096(1) || x == n - BigInt4096(1)) continue;
            bool passed = false;
            for (int j = 1; j < r; ++j) {
                x = modExp(x, BigInt4096(2), n);
                if (x == n - BigInt4096(1)) {
                    passed = true;
                    break;
                }
            }
            if (!passed) return false;
        }
        return true;
    }
};

#endif // BIGINT4096_HPP
