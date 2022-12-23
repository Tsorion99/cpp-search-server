#pragma once

#include <iterator>
#include <vector>

template <typename Iterator>
class IteratorRange {
public:
    explicit IteratorRange(const Iterator range_begin,
                           const Iterator range_end,
                           const size_t size)
        : begin_(range_begin),
          end_(range_end),
          size_(size)
    {
    }

    Iterator begin() const {
        return begin_;
    }

    Iterator end() const {
        return end_;
    }

    Iterator getSize() const {
        return size_;
    }

private:
    Iterator begin_;
    Iterator end_;
    size_t size_;
};

template <typename Iterator>
std::ostream& operator<<(std::ostream &os, const IteratorRange<Iterator> &i_r) {
    auto it = i_r.begin();

    while (it != i_r.end()) {
        os << *it;
        ++it;
    }
    return os;
}

template <typename Iterator>
class Paginator {
public:
    explicit Paginator(Iterator begin, Iterator end, const size_t page_size)
    {
        size_t i = 0;
        size_t distance_range = distance(begin, end);
        while (i < distance_range) {
            Iterator begin_it_page = begin;
            size_t shift = 0;
            if (i + page_size <= distance_range) {
                shift = page_size;
            }
            else {
                shift = i + page_size - distance_range;
            }
            advance(begin, shift);
            Iterator end_it_page = begin;
            pages_.push_back(IteratorRange<Iterator>(begin_it_page, end_it_page, shift));
            i += shift;
        }
    }
    
    auto begin() const {
        return pages_.begin();
    }
 
    auto end() const {
        return pages_.end();
    }
 
    auto size() const {
        return pages_.size();
    }

private:
    std::vector<IteratorRange<Iterator>> pages_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}