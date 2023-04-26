#pragma once

#include "string_processing.h"

using std::string_literals::operator""s;

template <typename type>
std::ostream& operator<<(std::ostream& out, const std::vector<type>& container) {
    out << "[";
    bool first = true;
    for (const type& element : container) {
        if (!first) {

            out << ", " << element;
        }
        else {
            out << element;
            first = false;
        }

    }
    out << "]";
    return out;
}

template <typename type>
std::ostream& operator<<(std::ostream& out, const std::set<type>& container) {
    out << "{";
    bool first = true;
    for (const type& element : container) {
        if (!first) {

            out << ", " << element;
        }
        else {
            out << element;
            first = false;
        }

    }
    out << "}";
    return out;
}

template <typename key, typename value>
std::ostream& operator<<(std::ostream& out, const std::map<key, value>& container) {
    out << "{";
    bool first = true;
    for (const auto& [key_, value_] : container) {
        if (!first) {

            out << ", " << key_ << ": " << value_;
        }
        else {
            out << key_ << ": " << value_;
            first = false;
        }

    }
    out << "}";
    return out;
}

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file,
    const std::string& func, unsigned line, const std::string& hint) {
    if (t != u) {
        std::cout << std::boolalpha;
        std::cout << file << "("s << line << "): "s << func << ": "s;
        std::cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        std::cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            std::cout << " Hint: "s << hint;
        }
        std::cout << std::endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line,
    const std::string& hint);

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

void TestAddDocument();
void TestMinusWords();
void TestStopWords();
void TestMatchDocument_();
void TestRelevanceSorting();
void TestRankingCalculations();
void TestPredicat();
void TestStatusSorting();

template <typename T>
void RunTestImpl(T func, const std::string& name) {
    func();
    std::cerr << name << " Test completed"s << std::endl;

}

#define RUN_TEST(func)  RunTestImpl(func,#func)

void TestSearchServer();
