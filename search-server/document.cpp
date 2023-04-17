#include <iostream>

#include "document.h"

using std::string_literals::operator""s;

Document::Document()
    :id(0), relevance(0), rating(0)
{}
Document::Document(int _id, double _relevance, int _rating)
    :id(_id), relevance(_relevance), rating(_rating)
{}

std::ostream& operator<<(std::ostream& out, const  Document doc) {
    out << "{ document_id = " << doc.id << ", relevance = " << doc.relevance << ", rating = " << doc.rating << " }";
    return out;
}

void PrintDocument(const Document& document) {
    std::cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s << std::endl;
}

