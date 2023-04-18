#pragma once

#include "document.h"

using namespace std;//Иначе яндекс не принимает(можно убрать)!!!
using std::string_literals::operator""s;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer {
public:
    SearchServer();
    explicit SearchServer(const std::string& stop_words);
    template <typename Collection> //Извеняюсь что так много раз не знаю от куда эта русская С везде зачасалась) Доброй ночи или утра или лня)
    explicit SearchServer(const Collection& stop_words);

    void AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings);

    int GetDocumentCount() const;

    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query, int document_id) const;

    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus status_document) const;

    std::vector<Document> FindTopDocuments(const std::string& raw_query) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentPredicate document_predicate) const;

    int GetDocumentId(int index) const;

private:

    struct DocumentInfo {
        int rating;
        DocumentStatus status;
    };
    struct QueryWord {
        std::string data;
        bool is_minus;
        bool is_stop;
    };
    struct Query {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };

    std::set<std::string> stop_words_;
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    std::vector<int> document_count_;
    std::map<int, DocumentInfo> document_info_;

    bool IsStopWord(const std::string& word) const;

    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    QueryWord ParseQueryWord(std::string text) const;

    Query ParseQuery(const std::string& text) const;

    double ComputeWordInverseDocumentFreq(const std::string& word) const;

    std::vector<Document> FindAllDocuments(const Query& query) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindDesiredStatusDocuments(const Query& query, DocumentPredicate predicat) const;

    static void CheckValidWord(const std::string& words);

    template <typename Collection>
    static void CheckValidWord(const Collection& words);
};


template <typename Collection>
SearchServer::SearchServer(const Collection& stop_words)
{
    CheckValidWord(stop_words);
    for (const std::string& word : stop_words) {
        stop_words_.insert(word);
    }
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentPredicate document_predicate) const {


    Query query = ParseQuery(raw_query);

    auto matched_documents = FindDesiredStatusDocuments(query, document_predicate);

    sort(matched_documents.begin(), matched_documents.end(),
        [](const Document& lhs, const Document& rhs) {
            const double EPSILON = 1e-6;
            return lhs.relevance > rhs.relevance || ((std::abs(lhs.relevance - rhs.relevance) < EPSILON) && lhs.rating > rhs.rating);
        });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    std::vector<Document> result = matched_documents;
    return result;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindDesiredStatusDocuments(const Query& query, DocumentPredicate predicat) const {
    std::vector<Document> required_documents = FindAllDocuments(query);
    std::vector<Document> desired_status_documents;
    int i = 0;
    for (const auto [id, relevance, rating] : required_documents) {
        if (predicat(id, document_info_.at(id).status, rating)) {
            desired_status_documents.push_back(required_documents[i]);
        }
        ++i;
    }
    return desired_status_documents;
}

template <typename Collection>
void SearchServer::CheckValidWord(const Collection& words) {
    for (const std::string& word : words) {
        if (any_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' '; })) {
            throw std::invalid_argument("stop_words contains invalid characters"s);
        }
    }
}