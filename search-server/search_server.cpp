
#include <cmath>
#include <string_view>


#include "search_server.h"
#include "string_processing.h"



using std::string_literals::operator""s;

    SearchServer::SearchServer() {
    }

    SearchServer::SearchServer(std::string_view stop_words) :SearchServer(SplitIntoWords(stop_words))
    {}
    SearchServer::SearchServer(const std::string& stop_words) :SearchServer(SplitIntoWords(std::string_view(stop_words)))
    {}
    
    void SearchServer::AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings) {
        if (document_id < 0) {
            throw std::invalid_argument("trying to add a document with a negative id"s);
        }
        if (document_info_.count(document_id)) {
            throw std::invalid_argument("attempt to add a document with an existing id"s);
        }
        CheckValidWord(document);

        storage.emplace_back(document);
        document_count_.insert(document_id);
        const std::vector<std::string_view> words = SplitIntoWordsNoStop(storage.back());
        const double inv_word_count = 1.0 / words.size();
        for (const std::string_view& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
            word_frequency_[document_id][word] += inv_word_count;
        }
        document_info_[document_id] = { ComputeAverageRating(ratings),status };
    }

    void SearchServer::RemoveDocument(int document_id) {
        if (!document_count_.count(document_id)) {
            return;
        }
       
        for (auto [word, TF] : word_frequency_[document_id]) {
            word_to_document_freqs_[word].erase(document_id);
        }

        document_count_.erase(document_id);
        document_info_.erase(document_id);
        word_frequency_.erase(document_id);
    }
    void SearchServer::RemoveDocument(std::execution::sequenced_policy, int document_id) {
        SearchServer::RemoveDocument(document_id);
    }
    void SearchServer::RemoveDocument(std::execution::parallel_policy, int document_id) {
        if (!document_count_.count(document_id)) {
            return;
        }
        std::vector<std::string_view> word_erase(word_frequency_[document_id].size());
        std::transform(std::execution::par, word_frequency_[document_id].begin(), word_frequency_[document_id].end(), word_erase.begin(),
            [](auto word_tf)
            {
                return word_tf.first;
            });
        std::for_each(std::execution::par, word_erase.begin(), word_erase.end(), [this,document_id](auto word)
            {
                word_to_document_freqs_.at(word).erase(document_id);
            });

        document_count_.erase(document_id);
        document_info_.erase(document_id);
        word_frequency_.erase(document_id);
    }


    int SearchServer::GetDocumentCount() const {
        return static_cast<int>(document_count_.size());
    }

    std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::string_view raw_query, int document_id) const {


        Query query = ParseQuery(false,raw_query);

        std::vector<std::string_view> plus_words_document;
        for (const std::string_view& word : query.minus_words) {
            if (word_to_document_freqs_.count(word)) {
                if (word_to_document_freqs_.at(word).count(document_id)) {
                    DocumentStatus status = document_info_.at(document_id).status;
                    std::tuple<std::vector<std::string_view>, DocumentStatus> result = { plus_words_document, status };
                    return result;
                    
                }
            }
        }
        for (const std::string_view& word : query.plus_words) {
            if (word_to_document_freqs_.count(word)) {
                if (word_to_document_freqs_.at(word).count(document_id)) {
                    plus_words_document.push_back(word);
                }
            }
        }
        
        sort(plus_words_document.begin(), plus_words_document.end());

        DocumentStatus status = document_info_.at(document_id).status;
        std::tuple<std::vector<std::string_view>, DocumentStatus> result = { plus_words_document, status };
        return result;
    }
    std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy,  std::string_view raw_query, int document_id) const {

        return MatchDocument(raw_query, document_id);
    }
    std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy, std::string_view raw_query, int document_id) const {

        Query query = ParseQuery(true,raw_query);
        
        std::vector<std::string_view> plus_words_document;

        if (!std::any_of(std::execution::par, query.minus_words.begin(), query.minus_words.end(), 
            [this,document_id](const std::string_view& word)
            {
                return word_to_document_freqs_.count(word) && word_to_document_freqs_.at(word).count(document_id);
            })) 
        {
            plus_words_document.resize(query.plus_words.size());
            auto it = std::copy_if(std::execution::par, query.plus_words.begin(), query.plus_words.end(), plus_words_document.begin(), 
                [this,document_id](const std::string_view& word)
                {
                    return word_to_document_freqs_.count(word) && word_to_document_freqs_.at(word).count(document_id);
                });
            plus_words_document.erase(it, plus_words_document.end());
            VectorEraseDuplicate(std::execution::par, plus_words_document);
        }

        DocumentStatus status = document_info_.at(document_id).status;
        std::tuple<std::vector<std::string_view>, DocumentStatus> result = { plus_words_document, status };
        return result;
    }

    std::vector<Document> SearchServer::FindTopDocuments( std::string_view raw_query, DocumentStatus status_document) const {
        return FindTopDocuments(raw_query, [status_document](int document_id, DocumentStatus status, int rating) { return  status == status_document; });
    }

    std::vector<Document> SearchServer::FindTopDocuments( std::string_view raw_query) const {
        DocumentStatus status_document = DocumentStatus::ACTUAL;
        return FindTopDocuments(raw_query, [status_document](int document_id, DocumentStatus status, int rating) { return  status == status_document; });
    }


    std::set<int>::const_iterator SearchServer::begin() const {
        return document_count_.begin();
    }

    std::set<int>::const_iterator SearchServer::end() const {
        return document_count_.end();
    }

    bool SearchServer::IsStopWord(std::string_view word) const {
        return stop_words_.count(word) > 0;
    }

    std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(std::string_view text) const {
        std::vector<std::string_view> words;
        for (const std::string_view& word : SplitIntoWords(text)) {
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

    SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
        QueryWord query_word;
        bool is_minus = false;
        // Word shouldn't be empty
        if (static_cast<int>(text.size()) == 1 ) {
            if (text[0] == '-') {
                throw std::invalid_argument("expected word after '-'"s);
            }
            else {
                query_word = { text,is_minus,IsStopWord(text) };
                return query_word;
            }
        }
        if (text[1] == '-' && text[0] == '-') {
            throw std::invalid_argument("Two '-' characters in a row"s);
        }
        if (text.back() == '-') {
            throw std::invalid_argument("Invalid character '-' at the end of a word"s);
        }
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        query_word = { text,is_minus,IsStopWord(text) };
        return query_word;
    }

    SearchServer::Query SearchServer::ParseQuery(const bool Need_parallel_version,  std::string_view text) const {
        Query query;
        CheckValidWord(text);
        for (const std::string_view& word : SplitIntoWords(text)) {
            QueryWord query_word = ParseQueryWord(word);

            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.push_back(query_word.data);
                }
                else {
                    query.plus_words.push_back(query_word.data);
                }
            }
        }
        if (!Need_parallel_version) {
            VectorEraseDuplicate(std::execution::seq, query.minus_words);
            VectorEraseDuplicate(std::execution::seq, query.plus_words);
        }
       

        return query;
    }
   
    double SearchServer::ComputeWordInverseDocumentFreq(std::string_view word) const {
        return log(document_info_.size() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    
    
    void SearchServer::CheckValidWord(std::string_view words) {
        if (std::any_of(words.begin(), words.end(), [](char c) {
            return c >= '\0' && c < ' '; })) {
            throw std::invalid_argument("stop_words contains invalid characters"s);
        }
    }

    const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
        static const std::map<std::string_view, double> empty_map = {};

        if (!word_frequency_.count(document_id)) {
            return empty_map;
        }
        return word_frequency_.at(document_id);
    }

    void SearchServer::VectorEraseDuplicate(const std::execution::sequenced_policy, std::vector<std::string_view>& vec) const {
        std::sort(vec.begin(), vec.end());
        auto last = std::unique(vec.begin(), vec.end());
        vec.erase(last, vec.end());
    }
    void SearchServer::VectorEraseDuplicate(const std::execution::parallel_policy, std::vector<std::string_view>& vec) const {
        std::sort(std::execution::par,vec.begin(), vec.end());
        auto last = std::unique(std::execution::par, vec.begin(), vec.end());
        vec.erase(last, vec.end());
    }