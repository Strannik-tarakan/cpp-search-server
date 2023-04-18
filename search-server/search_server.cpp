#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>

#include "search_server.h"
#include "string_processing.h"


using std::string_literals::operator""s;

    SearchServer::SearchServer() {
    }

    SearchServer::SearchServer(const std::string& stop_words) :SearchServer(SplitIntoWords(stop_words))
    {}
    
    void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
        if (document_id < 0) {
            throw std::invalid_argument("trying to add a document with a negative id"s);
        }
        if (document_info_.count(document_id)) {
            throw std::invalid_argument("attempt to add a document with an existing id"s);
        }
        CheckValidWord(document);
        document_count_.push_back(document_id);
        const std::vector<std::string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const std::string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        document_info_[document_id] = { ComputeAverageRating(ratings),status };

    }

    int SearchServer::GetDocumentCount() const {
        return static_cast<int>(document_count_.size());
    }

    std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const {

        if (!document_info_.count(document_id)) {
            throw std::invalid_argument("There is no document with this id"s);
        }


        Query query = ParseQuery(raw_query);

        std::vector<std::string> plus_words_document;
        for (const std::string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word)) {
                if (word_to_document_freqs_.at(word).count(document_id)) {
                    plus_words_document.push_back(word);
                }
            }
        }
        for (const std::string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word)) {
                if (word_to_document_freqs_.at(word).count(document_id)) {
                    plus_words_document.clear();
                    break;
                }
            }
        }
        sort(plus_words_document.begin(), plus_words_document.end());

        DocumentStatus status = document_info_.at(document_id).status;
        std::tuple<std::vector<std::string>, DocumentStatus> result = { plus_words_document, status };
        return result;
    }

    std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status_document) const {
        return FindTopDocuments(raw_query, [status_document](int document_id, DocumentStatus status, int rating) { return  status == status_document; });
    }

    std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
        DocumentStatus status_document = DocumentStatus::ACTUAL;
        return FindTopDocuments(raw_query, [status_document](int document_id, DocumentStatus status, int rating) { return  status == status_document; });
    }

    int SearchServer::GetDocumentId(int index) const {
        if (index < 0 || index >= static_cast<int>(document_count_.size())) {

            throw std::out_of_range("invalid index. Documents - "s + std::to_string(SearchServer::GetDocumentCount()));

        }
        return document_count_[index];
    }

    bool SearchServer::IsStopWord(const std::string& word) const {
        return stop_words_.count(word) > 0;
    }

    std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
        std::vector<std::string> words;
        for (const std::string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }

    SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const {
        QueryWord query_word;
        bool is_minus = false;
        // Word shouldn't be empty
        if (static_cast<int>(text.size()) == 1 && text[0] == '-') {
            throw std::invalid_argument("expected word after '-'"s);
        }
        else if (text[1] == '-' && text[0] == '-') {
            throw std::invalid_argument("Two '-' characters in a row"s);
        }
        else if (text[static_cast<int>(text.size() - 1)] == '-') {
            throw std::invalid_argument("Invalid character '-' at the end of a word"s);
        }
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        query_word = { text,is_minus,IsStopWord(text) };
        return query_word;
    }

    SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
        Query query;
        CheckValidWord(text);
        for (const std::string& word : SplitIntoWords(text)) {
            QueryWord query_word = ParseQueryWord(word);


            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                }
                else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }

        return query;
    }
   
    double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
        return log(document_info_.size() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    std::vector<Document> SearchServer::FindAllDocuments(const Query& query) const {
        std::map<int, double> document_to_relevance;
        for (const std::string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }

        for (const std::string& word : query.minus_words) {
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
    
    void SearchServer::CheckValidWord(const std::string& words) {
        if (any_of(words.begin(), words.end(), [](char c) {
            return c >= '\0' && c < ' '; })) {
            throw std::invalid_argument("stop_words contains invalid characters"s);
        }
    }

   