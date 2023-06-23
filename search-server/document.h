#pragma once

enum DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED
};

struct Document {
    Document();
    Document(int _id, double _relevance, int _rating);
    int id;
    double relevance;
    int rating;
};

std::ostream& operator<<(std::ostream& out, const  Document doc);

void PrintDocument(const Document& document);

