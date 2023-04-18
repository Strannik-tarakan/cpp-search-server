#pragma once

#include "search_server.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);
   
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);
    
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;
private:

    void TickOfTime();

    struct QueryResult {
        bool result_is_empty;
        int min_of_life = 0;
       
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& search_server_;
    int no_result_requests = 0;
    
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    QueryResult query_result;
    vector<Document> result;
    result = search_server_.FindTopDocuments(raw_query, document_predicate);
    query_result.result_is_empty = result.empty();
    TickOfTime();
    requests_.push_back(query_result);
    if (query_result.result_is_empty) {
        ++no_result_requests;
    }
    return result;
}


