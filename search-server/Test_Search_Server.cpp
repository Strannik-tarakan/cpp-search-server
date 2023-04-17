#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <tuple>
#include <cassert>
#include <optional>
#include <deque>

#include "Test_Search_Server.h"
#include "search_server.h"

using std::string_literals::operator""s;


#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line,
    const std::string& hint) {
    if (!value) {
        std::cout << file << "("s << line << "): "s << func << ": "s;
        std::cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            std::cout << " Hint: "s << hint;
        }
        std::cout << std::endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

void TestAddDocument() {
    SearchServer examination;
    int document_id = 0;
    std::string document = "good white dog"s;
    DocumentStatus status = DocumentStatus::ACTUAL;
    std::vector<int> rating = { 5,-2 };
    examination.AddDocument(document_id, document, status, rating);
    ASSERT(examination.FindTopDocuments("fgh"s).empty());
    ASSERT_EQUAL_HINT(examination.FindTopDocuments("good"s).size(), 1, "Document not added or cannot be found"s);
}
void TestMinusWords() {
    SearchServer examination;
    DocumentStatus status = DocumentStatus::ACTUAL;
    std::vector<int> rating = { 5,-2 };
    examination.AddDocument(0, "good white dog"s, status, rating);
    examination.AddDocument(1, "good white black dog"s, status, rating);
    examination.AddDocument(2, "fghjk"s, status, rating);
    ASSERT_EQUAL(examination.FindTopDocuments("good"s).size(), 2);
    ASSERT_EQUAL(examination.FindTopDocuments("good -black"s).size(), 1);

}
void TestStopWords() {
    SearchServer examination("in is a"s);
    std::set<std::string> stop_w1 = { "in"s, "is"s, "a"s };
    std::vector<std::string> stop_w2 = { "in"s, "is"s, "a"s };
    SearchServer examination2(stop_w1);
    SearchServer examination3(stop_w2);
    DocumentStatus status = DocumentStatus::ACTUAL;
    std::vector<int> rating = { 5,-2 };
    examination.AddDocument(0, "good in white dog"s, status, rating);
    examination.AddDocument(1, "good is white a black dog"s, status, rating);
    examination2.AddDocument(0, "good in white dog"s, status, rating);
    examination2.AddDocument(1, "good is white a black dog"s, status, rating);
    examination3.AddDocument(0, "good in white dog"s, status, rating);
    examination3.AddDocument(1, "good is white a black dog"s, status, rating);
    const auto& [checked_document_1, id1] = examination.MatchDocument("good in white dog"s, 0);
    const auto& [checked_document_2, id2] = examination.MatchDocument("good in white a dog"s, 1);
    const auto& [checked_document_2_1, id21] = examination2.MatchDocument("good in white dog"s, 0);
    const auto& [checked_document_2_2, id22] = examination2.MatchDocument("good in white a dog"s, 1);
    const auto& [checked_document_3_1, id31] = examination3.MatchDocument("good in white dog"s, 0);
    const auto& [checked_document_3_2, id32] = examination3.MatchDocument("good in white a dog"s, 1);
    std::vector<std::string> words_doc = { "dog"s,"good"s,"white"s };

    ASSERT_EQUAL(checked_document_1, words_doc);
    ASSERT_EQUAL(checked_document_2, words_doc);
    ASSERT_EQUAL(checked_document_2_1, words_doc);
    ASSERT_EQUAL(checked_document_2_2, words_doc);
    ASSERT_EQUAL(checked_document_3_1, words_doc);
    ASSERT_EQUAL(checked_document_3_2, words_doc);
}
void TestMatchDocument_() {
    SearchServer examination;
    DocumentStatus status = DocumentStatus::ACTUAL;
    std::vector<int> rating = { 5,-2 };
    examination.AddDocument(0, "good black white dog"s, status, rating);
    const auto& [checked_document1, id1] = examination.MatchDocument(" good  white dog"s, 0);
    const auto& [checked_document2, id2] = examination.MatchDocument("good -white a dog"s, 0);
    std::vector<std::string> words_doc1 = { "dog"s,"good"s,"white"s };
    std::vector<std::string> words_doc2 = { };
    ASSERT_EQUAL(checked_document1, words_doc1);
    ASSERT_EQUAL(checked_document2, words_doc2);
}
void TestRelevanceSorting() {
    SearchServer examination;
    DocumentStatus status = DocumentStatus::ACTUAL;
    std::vector<int> rating = { 5,-2 };
    examination.AddDocument(0, "1 2 3 1"s, status, rating);
    examination.AddDocument(1, "3 3"s, status, rating);
    examination.AddDocument(2, "1 3"s, status, rating);
    examination.AddDocument(3, "4 5"s, status, rating);
    std::vector<int> relevanse_id = { 2,0,1 };
    std::vector<double> relevanse = { 0.49,0.418,0.287 };
    int i = 0;
    for (const Document& document : examination.FindTopDocuments("1 3"s)) {
        ASSERT_EQUAL_HINT(document.id, relevanse_id[i], "Relevance sorting is wrong. Document out of place"s);
        const double EPSILON = 0.001;
        ASSERT_HINT((document.relevance - relevanse[i]) < EPSILON, "Relevance is calculated incorrectly"s);
        ++i;
    }
}
void TestRankingCalculations() {
    SearchServer examination;
    DocumentStatus status = DocumentStatus::ACTUAL;
    std::vector<int> rating1 = { 5,-2, 3 };
    std::vector<int> rating2 = { 5 };
    std::vector<int> rating3 = { 0 };
    std::vector<int> rating4 = { 5 };
    examination.AddDocument(0, "2"s, status, rating1);
    examination.AddDocument(1, "2 1"s, status, rating2);
    examination.AddDocument(2, "2 1 1"s, status, rating3);
    examination.AddDocument(3, "1"s, status, rating4);
    std::vector<double> rating = { 2,5,0 };
    int i = 0;
    for (const Document& document : examination.FindTopDocuments("2"s)) {
        ASSERT_EQUAL(document.rating, rating[i]);
        ++i;
    }
}
void TestPredicat() {
    SearchServer examination;
    DocumentStatus status = DocumentStatus::ACTUAL;
    std::vector<int> rating1 = { 5,-2 };
    std::vector<int> rating2 = { 6 };
    examination.AddDocument(0, "1 2 3 1"s, status, rating1);//relevanse=0.18 /idf1=0.3 idf3=0.12/tf1=0.5,tf3=0.25
    examination.AddDocument(1, "3 3"s, status, rating1);//relevanse=0,12 /tf1=0,tf3=1
    examination.AddDocument(2, "1 3"s, status, rating2);//relevanse=0,21
    examination.AddDocument(3, "4 5"s, status, rating1);//relevanse=0;
    ASSERT(examination.FindTopDocuments("1 3"s).size() == 3);
    ASSERT(examination.FindTopDocuments("1 3"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; }).size() == 2);
    ASSERT(examination.FindTopDocuments("1 3"s, [](int document_id, DocumentStatus status, int rating) { return rating > 5; }).size() == 1);
}
void TestStatusSorting() {
    SearchServer examination;
    std::vector<int> rating = { 5,-2 };
    std::string doc = "1 2"s;
    examination.AddDocument(0, doc, DocumentStatus::ACTUAL, rating);//relevanse=0.18 /idf1=0.3 idf3=0.12/tf1=0.5,tf3=0.25
    examination.AddDocument(1, doc, DocumentStatus::ACTUAL, rating);//relevanse=0,12 /tf1=0,tf3=1
    examination.AddDocument(2, doc, DocumentStatus::BANNED, rating);//relevanse=0,21
    examination.AddDocument(3, doc, DocumentStatus::IRRELEVANT, rating);//relevanse=0;
    ASSERT(examination.FindTopDocuments("1"s, DocumentStatus::ACTUAL).size() == 2);
    ASSERT(examination.FindTopDocuments("1"s, DocumentStatus::BANNED).size() == 1);
    ASSERT(examination.FindTopDocuments("1"s, DocumentStatus::REMOVED).size() == 0);
}



#define RUN_TEST(func)  RunTestImpl(func,#func)

void TestSearchServer() {
    RUN_TEST(TestAddDocument);
    RUN_TEST(TestMinusWords);
    RUN_TEST(TestStopWords);
    RUN_TEST(TestMatchDocument_);
    RUN_TEST(TestRelevanceSorting);
    RUN_TEST(TestRankingCalculations);
    RUN_TEST(TestPredicat);
    RUN_TEST(TestStatusSorting);
}
