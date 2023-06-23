#pragma once

#include <set>
#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <execution>
#include <algorithm>
#include <deque>

#include "document.h"
#include "concurrent_map.h"

//using namespace std;//Иначе яндекс не принимает(можно убрать)!!!
using std::string_literals::operator""s;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer {
public:
    SearchServer();
    explicit SearchServer(std::string_view stop_words);
    explicit SearchServer(const std::string& stop_words);
    template <typename Collection> 
    explicit SearchServer(const Collection& stop_words);

    void AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings);
    void RemoveDocument(int document_id);
    void RemoveDocument(std::execution::sequenced_policy,int document_id);
    void RemoveDocument(std::execution::parallel_policy, int document_id);


    std::set<int>::const_iterator begin() const;
    std::set<int>::const_iterator end() const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::sequenced_policy, std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::parallel_policy,  std::string_view raw_query, int document_id) const;

    std::vector<Document> FindTopDocuments( std::string_view raw_query, DocumentStatus status_document) const;
    std::vector<Document> FindTopDocuments(std::string_view raw_query) const;
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments( std::string_view raw_query, DocumentPredicate document_predicate) const;
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query, DocumentStatus status_document) const;
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query) const;
    template <typename DocumentPredicate, typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query, DocumentPredicate document_predicate) const;


    int GetDocumentCount() const;
    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

private:

    struct DocumentInfo {
        int rating;
        DocumentStatus status;
    };
    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };
    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };
    
    std::deque<std::string> storage;
    std::set<std::string, std::less<>> stop_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::set<int> document_count_;
    std::map<int, DocumentInfo> document_info_;
    std::map<int, std::map<std::string_view, double>> word_frequency_;

    bool IsStopWord(std::string_view word) const;
    std::vector<std::string_view> SplitIntoWordsNoStop( std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);
    double ComputeWordInverseDocumentFreq(std::string_view word) const;

    QueryWord ParseQueryWord(std::string_view text) const;
    Query ParseQuery(const bool Need_parallel_version, std::string_view text) const;
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate& predicat) const;
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::sequenced_policy, const Query& query, DocumentPredicate& predicat) const;
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::parallel_policy, const Query& query, DocumentPredicate& predicat) const;

    static void CheckValidWord(std::string_view words);
    template <typename Collection>
    static void CheckValidWord(const Collection& words);

    void VectorEraseDuplicate(const std::execution::sequenced_policy, std::vector<std::string_view>& vec) const;
    void VectorEraseDuplicate(const std::execution::parallel_policy, std::vector<std::string_view>& vec) const;
    
};



template <typename Collection>
SearchServer::SearchServer(const Collection& stop_words)
{
    CheckValidWord(stop_words);
    for (const std::string_view& word : stop_words) {
        stop_words_.insert(std::string(word));
    }
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const {


    Query query = ParseQuery(false,raw_query);
    
    auto matched_documents = FindAllDocuments(query, document_predicate);

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

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query, DocumentStatus status_document) const {
    return FindTopDocuments(policy, raw_query, [status_document](int document_id, DocumentStatus status, int rating) { return  status == status_document; });
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query) const {
    DocumentStatus status_document = DocumentStatus::ACTUAL;
    return FindTopDocuments(policy, raw_query, [status_document](int document_id, DocumentStatus status, int rating) { return  status == status_document; });
}

template <typename DocumentPredicate, typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query, DocumentPredicate document_predicate) const {

    Query query = ParseQuery( false, raw_query);

    auto matched_documents = FindAllDocuments(policy, query, document_predicate);

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
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate& predicat) const {
    std::map<int, double> document_to_relevance;
    for (const std::string_view& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            if (predicat(document_id, document_info_.at(document_id).status, document_info_.at(document_id).rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
            
        }
    }

    for (const std::string_view& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back({
            document_id,
            relevance,
            document_info_.at(document_id).rating
            });
    }
    return matched_documents;
}
template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::sequenced_policy, const Query& query, DocumentPredicate& predicat) const {
    return FindAllDocuments(query,predicat);
}
template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::parallel_policy, const Query& query, DocumentPredicate& predicat) const {
    ConcurrentMap<int, double> document_to_relevance(100);
    for_each(std::execution::par, query.plus_words.begin(), query.plus_words.end(),
        [this, &document_to_relevance,&predicat](const std::string_view& word)
        {
            if (word_to_document_freqs_.count(word) != 0) {
                const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                    if (predicat(document_id, document_info_.at(document_id).status, document_info_.at(document_id).rating)) {
                        document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
                    }
                }
            }
        });

    for_each(std::execution::par, query.minus_words.begin(), query.minus_words.end(),
        [this, &document_to_relevance](const std::string_view& word)
        {
            if (word_to_document_freqs_.count(word) != 0) {
                for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                    document_to_relevance.erase(document_id);
                }
            }
        });

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance.BuildOrdinaryMap()) {
        matched_documents.push_back({
            document_id,
            relevance,
            document_info_.at(document_id).rating
            });
    }
    return matched_documents;
}

template <typename Collection>
void SearchServer::CheckValidWord(const Collection& words) {
    for (const std::string_view& word : words) {
        if (std::any_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' '; })) {
            throw std::invalid_argument("stop_words contains invalid characters"s);
        }
    }
}