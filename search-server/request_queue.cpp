#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer& search_server) :
    search_server_(search_server),
    count_requests_(0),
    count_empty_requests_(0)
{
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    std::vector<Document> result = search_server_.FindTopDocuments(raw_query, status);
    AddRequest(result.empty());
    return result;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    std::vector<Document> result = search_server_.FindTopDocuments(raw_query);
    AddRequest(result.empty());
    return result;
}

int RequestQueue::GetNoResultRequests() const {
    return count_empty_requests_;
}

void RequestQueue::AddRequest(bool response_is_empty) {
    if (count_requests_ == min_in_day_) {
        if (requests_.front().isEmpty) {
            --count_empty_requests_;
        }
        requests_.pop_front();
    }
    else {
        ++count_requests_;
    }
    if (response_is_empty) {
        requests_.push_back({ true });
        ++count_empty_requests_;
    }
    else {
        requests_.push_back({ false });
    }
}
