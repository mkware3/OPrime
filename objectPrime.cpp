#include <iostream>
#include <getopt.h>
#include <cstdlib>
#include <string>
#include <sstream>
#include <chrono>
#include <vector>
#include <fstream>
#include "BigInt4096.hpp"

// -----------------------------------
// WorkTask: Computes various prime-related tasks
// -----------------------------------
class WorkTask {
public:
    enum class Mode { NthPrime, LessThan, AllUpTo };

    WorkTask(Mode mode, const BigInt4096& value, unsigned int timeout_secs, bool show_runtime)
        : mode(mode), value(value), timeout(timeout_secs), show_runtime(show_runtime) {
        buckets = {BigInt4096(1), BigInt4096(2), BigInt4096(3), BigInt4096(5), BigInt4096(7), BigInt4096(9), BigInt4096(11)};
    }

    void exec() {
        auto start = std::chrono::steady_clock::now();
        std::cout << "Starting prime task...\n";
        BigInt4096 result(0);
        if (mode == Mode::NthPrime) {
            result = compute_nth_prime(value);
            std::cout << "The " << value << "th prime is: " << result << "\n";
        } else if (mode == Mode::LessThan) {
            result = compute_prime_less_than(value);
            std::cout << "Largest prime ≤ " << value << " is: " << result << "\n";
        } else if (mode == Mode::AllUpTo) {
            compute_all_primes_up_to(value);
            std::cout << "Found " << primes.size() << " primes ≤ " << value << "\n";
            write_primes_to_file("primes.txt");
        }
        auto end = std::chrono::steady_clock::now();
        auto total_elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
        if (show_runtime) {
            std::cout << "Total elapsed time: " << total_elapsed << " seconds\n";
        }
    }

private:
    Mode mode;
    BigInt4096 value;
    unsigned int timeout;
    bool show_runtime;
    std::vector<BigInt4096> primes;
    std::vector<BigInt4096> buckets;

    bool is_prime(const BigInt4096& num) {
        return BigInt4096::isPrime(num);
    }

    BigInt4096 compute_nth_prime(const BigInt4096& n) {
        BigInt4096 count(0);
        BigInt4096 candidate(2);
        auto start = std::chrono::steady_clock::now();
        while (true) {
            if (is_prime(candidate)) {
                count += BigInt4096(1);
                if (count == n)
                    return candidate;
            }
            if (timeout > 0) {
                auto now = std::chrono::steady_clock::now();
                if (std::chrono::duration_cast<std::chrono::seconds>(now - start).count() >= timeout) {
                    std::cerr << "Timeout reached during nth-prime computation\n";
                    return BigInt4096(0);
                }
            }
            candidate += BigInt4096(1);
        }
    }

    BigInt4096 compute_prime_less_than(const BigInt4096& n) {
        if (n < BigInt4096(2)) return BigInt4096(0);
        auto start = std::chrono::steady_clock::now();
        for (BigInt4096 i = n; i >= BigInt4096(2); i -= BigInt4096(1)) {
            if (is_prime(i)) return i;
            if (timeout > 0) {
                auto now = std::chrono::steady_clock::now();
                if (std::chrono::duration_cast<std::chrono::seconds>(now - start).count() >= timeout) {
                    std::cerr << "Timeout reached during prime-less-than-n computation\n";
                    return BigInt4096(0);
                }
            }
        }
        return BigInt4096(0);
    }

    void compute_all_primes_up_to(const BigInt4096& n) {
        auto start = std::chrono::steady_clock::now();
        for (BigInt4096 i = BigInt4096(2); i <= n; i += BigInt4096(1)) {
            if (is_prime(i)) {
                primes.push_back(i);
            }
            if (timeout > 0) {
                auto now = std::chrono::steady_clock::now();
                if (std::chrono::duration_cast<std::chrono::seconds>(now - start).count() >= timeout) {
                    std::cerr << "Timeout reached during all-primes-up-to computation\n";
                    break;
                }
            }
        }
    }

    void write_primes_to_file(const std::string& filename) {
        std::ofstream file(filename);
        if (!file) {
            std::cerr << "Failed to open " << filename << " for writing.\n";
            return;
        }
        for (const auto& prime : primes) {
            file << prime << "\n";
        }
        std::cout << "Primes written to " << filename << "\n";
    }
};

// -----------------------------------
// TimedTaskApp: CLI frontend and parser
// -----------------------------------
class TimedTaskApp {
public:
    int run(int argc, char* argv[]) {
        if (!parse_arguments(argc, argv)) {
            usage(argv[0]);
            return 1; // Explicit return for clarity, though usage() calls std::exit()
        }
        WorkTask::Mode mode = has_n ? WorkTask::Mode::NthPrime :
                              has_le ? WorkTask::Mode::LessThan :
                                       WorkTask::Mode::AllUpTo;
        BigInt4096 value = has_n ? n_value :
                             has_le ? le_value :
                                       all_value;
        WorkTask task(mode, value, timeout, show_runtime);
        task.exec();
        return 0;
    }

private:
    bool has_n = false;
    bool has_le = false;
    bool has_all = false;
    bool show_runtime = false;
    BigInt4096 n_value = 0;
    BigInt4096 le_value = 0;
    BigInt4096 all_value = 0;
    unsigned int timeout = 0;

    void usage(const char* progname) {
        std::cerr << "Usage:\n";
        std::cerr << "  " << progname << " -n <N>       # Nth prime\n";
        std::cerr << "  " << progname << " --le <N>    # Prime ≤ N\n";
        std::cerr << "  " << progname << " --all <N>   # All primes ≤ N (to primes.txt)\n";
        std::cerr << "Optional:\n";
        std::cerr << "  -t <seconds>       # Limit execution time\n";
        std::cerr << "  --rt               # Print runtime\n";
        std::cerr << "Exactly one of -n, --le, or --all must be specified.\n";
        std::exit(EXIT_FAILURE);
    }

    bool parse_BigInt4096(const char* arg, BigInt4096& out) {
        std::istringstream ss(arg);
        ss >> out;
        return !ss.fail() && ss.eof();
    }

    bool parse_arguments(int argc, char* argv[]) {
        static struct option long_options[] = {
            {"le", required_argument, nullptr, 1000},
            {"rt", no_argument, nullptr, 1001},
            {"all", required_argument, nullptr, 1002},
            {0, 0, 0, 0}
        };
        optind = 1;
        int opt;
        while ((opt = getopt_long(argc, argv, "n:t:", long_options, nullptr)) != -1) {
            switch (opt) {
                case 'n':
                    if (!parse_BigInt4096(optarg, n_value)) {
                        std::cerr << "Error: -n requires a valid integer.\n";
                        return false;
                    }
                    has_n = true;
                    break;
                    try {
                        timeout = std::stoul(optarg);
                        // 0 means no timeout, so allow it
                    } catch (...) {
                        std::cerr << "Error: -t requires a non-negative integer.\n";
                        return false;
                    }
                    
                    break;
                case 1000: // --le
                    if (!parse_BigInt4096(optarg, le_value)) {
                        std::cerr << "Error: --le requires a valid integer.\n";
                        return false;
                    }
                    has_le = true;
                    break;
                case 1001: // --rt
                    show_runtime = true;
                    break;
                case 1002: // --all
                    if (!parse_BigInt4096(optarg, all_value)) {
                        std::cerr << "Error: --all requires a valid integer.\n";
                        return false;
                    }
                    has_all = true;
                    break;
                default:
                    return false;
            }
        }
        return (has_n + has_le + has_all == 1); // exactly one mode
    }
};

// -----------------------------------
// Entry Point
// -----------------------------------
int main(int argc, char* argv[]) {
    TimedTaskApp app;
    return app.run(argc, argv);
}
