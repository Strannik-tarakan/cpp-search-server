
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <deque>

#include "request_queue.h"



    RequestQueue::RequestQueue(const SearchServer& search_server)
        :search_server_(search_server)
    {
    }
    
    
    std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
        QueryResult query_result;
        query_result.result = search_server_.FindTopDocuments(raw_query, status);
        TickOfTime();
        requests_.push_back(query_result);
        if (query_result.result.empty()) {
            ++no_result_requests;
        }
        return query_result.result;
       
    }

    std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
        QueryResult query_result;
        query_result.result = search_server_.FindTopDocuments(raw_query);
        TickOfTime();
        requests_.push_back(query_result);
        if (query_result.result.empty()) {
            ++no_result_requests;
        }
        return query_result.result;
       
    }
    int RequestQueue::GetNoResultRequests() const {
        return no_result_requests;
        
    }

    void RequestQueue::TickOfTime() {
        int pop_query = 0;
        for (QueryResult& query : requests_) {
            ++query.min_of_life;
            if (query.min_of_life >= min_in_day_) {
                if (query.result.empty()) {
                    --no_result_requests;
                }
                ++pop_query;
            }
        }
        for (int i = 0; i < pop_query; ++i) {
            requests_.pop_front();
        }
    }
    
