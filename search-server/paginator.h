#pragma once

template <typename Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator it1, Iterator it2)
        : it1_(it1), it2_(it2)
    {}

    auto begin() const {
        return it1_;
    }

    auto end() const {
        return it2_;
    }

private:
    Iterator it1_;
    Iterator it2_;
};

template <typename Iterator>
std::ostream& operator<<(std::ostream& out, const IteratorRange<Iterator> container) {
    for (auto i = container.begin(); i != container.end(); ++i) {
        out << *i;
    }

    return out;
}

template <typename Iterator>
class Paginator {
public:
    Paginator(Iterator it1, Iterator it2, int page_size) {
        auto page_begin = it1;
        auto page_end = it2;
        if (distance(it1, it2) <= page_size) {
            page_end = it2;

        }
        else {
            page_end = it1;
            advance(page_end, page_size);
        }
        for (int i = 0; i < page_size; ++i) {
            IteratorRange<Iterator> g(page_begin, page_end);
            pages.push_back(g);
            page_begin = page_end;
            if (distance(page_begin, it2) <= page_size) {
                page_end = it2;
            }
            else {
                page_end = page_begin;
                advance(page_begin, page_size);
            }
        }
    }
    auto begin() const {
        return pages.begin();
    }
    auto end() const {
        return pages.end();
    }
    int size() {
        return pages.size();
    }
private:
    std::vector<IteratorRange<Iterator>> pages;
   
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}
