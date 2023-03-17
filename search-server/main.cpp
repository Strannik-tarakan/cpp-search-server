#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <tuple>
#include <cassert>


using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

enum DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED
};
template <typename type>
ostream& operator<<(ostream& out, const vector<type>& container) {
    out << "[";
    bool first = true;
    for (const type& element : container) {
        if (!first) {

            out << ", " << element;
        }
        else {
            out << element;
            first = false;
        }

    }
    out << "]";
    return out;
}
template <typename type>
ostream& operator<<(ostream& out, const set<type>& container) {
    out << "{";
    bool first = true;
    for (const type& element : container) {
        if (!first) {

            out << ", " << element;
        }
        else {
            out << element;
            first = false;
        }

    }
    out << "}";
    return out;
}
template <typename key, typename value>
ostream& operator<<(ostream& out, const map<key, value>& container) {
    out << "{";
    bool first = true;
    for (const auto& [key_, value_] : container) {
        if (!first) {

            out << ", " << key_ << ": " << value_;
        }
        else {
            out << key_ << ": " << value_;
            first = false;
        }

    }
    out << "}";
    return out;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            words.push_back(word);
            word = "";
        }
        else {
            word += c;
        }
    }
    words.push_back(word);

    return words;
}

struct Document {
    int id;
    double relevance;
    int rating;
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        document_info_[document_id] = { ComputeAverageRating(ratings),status };
    }

    int GetDocumentCount() const {
        return document_info_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> plus_words_document;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word)) {
                if (word_to_document_freqs_.at(word).count(document_id)) {
                    plus_words_document.push_back(word);
                }
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word)) {
                if (word_to_document_freqs_.at(word).count(document_id)) {
                    plus_words_document.clear();
                    break;
                }
            }
        }
        sort(plus_words_document.begin(), plus_words_document.end());
        DocumentStatus status = document_info_.at(document_id).status;
        return tuple(plus_words_document, status);
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status_document = DocumentStatus::ACTUAL) const {
        return FindTopDocuments(raw_query, [status_document](int document_id, DocumentStatus status, int rating) { return  status == status_document; });
    }

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate predicat) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindDesiredStatusDocuments(query, predicat);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                const double EPSILON = 1e-6;
                return lhs.relevance > rhs.relevance || ((abs(lhs.relevance - rhs.relevance) < EPSILON) && lhs.rating > rhs.rating);
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:
    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    int document_count_ = 0;
    struct DocumentInfo {
        int rating;
        DocumentStatus status;
    };

    map<int, DocumentInfo> document_info_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {
            text,
            is_minus,
            IsStopWord(text)
        };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
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

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(document_info_.size() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    vector<Document> FindAllDocuments(const Query& query) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
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
    vector<Document> FindDesiredStatusDocuments(const Query& query, DocumentPredicate predicat) const {
        vector<Document> required_documents = FindAllDocuments(query);
        vector<Document> desired_status_documents;
        int i = 0;
        for (const auto [id, relevance, rating] : required_documents) {
            if (predicat(id, document_info_.at(id).status, rating)) {
                desired_status_documents.push_back(required_documents[i]);
            }
            ++i;
        }
        return desired_status_documents;
    }
};

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
    const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
    const string& hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))



void TestAddDocument() {
    SearchServer examination;
    int document_id = 0;
    string document = "good white dog"s;
    DocumentStatus status = DocumentStatus::ACTUAL;
    vector<int> rating = { 5,-2 };
    examination.AddDocument(document_id, document, status, rating);
    ASSERT(examination.FindTopDocuments("fgh").empty());
    ASSERT_EQUAL_HINT(examination.FindTopDocuments("good").size(), 1, "Document not added or cannot be found"s);
}
void TestMinusWords() {
    SearchServer examination;
    DocumentStatus status = DocumentStatus::ACTUAL;
    vector<int> rating = { 5,-2 };
    examination.AddDocument(0, "good white dog", status, rating);
    examination.AddDocument(1, "good white black dog", status, rating);
    examination.AddDocument(2, "fghjk", status, rating);
    ASSERT_EQUAL(examination.FindTopDocuments("good").size(), 2);
    ASSERT_EQUAL(examination.FindTopDocuments("good -black").size(), 1);

}
void TestStopWords() {
    SearchServer examination;
    DocumentStatus status = DocumentStatus::ACTUAL;
    vector<int> rating = { 5,-2 };
    examination.SetStopWords("in is a");
    examination.AddDocument(0, "good in white dog", status, rating);
    examination.AddDocument(1, "good is white a black dog", status, rating);
    const auto& [checked_document1, id1] = examination.MatchDocument("good in white dog", 0);
    const auto& [checked_document2, id2] = examination.MatchDocument("good in white a dog", 1);
    vector<string> words_doc = { "dog","good","white" };

    ASSERT_EQUAL(checked_document1, words_doc);
    ASSERT_EQUAL(checked_document2, words_doc);
}
void TestMatchDocument() {
    SearchServer examination;
    DocumentStatus status = DocumentStatus::ACTUAL;
    vector<int> rating = { 5,-2 };
    examination.AddDocument(0, "good black white dog", status, rating);
    const auto& [checked_document1, id1] = examination.MatchDocument("good  white dog", 0);
    const auto& [checked_document2, id2] = examination.MatchDocument("good -white a dog", 0);
    vector<string> words_doc1 = { "dog","good","white" };
    vector<string> words_doc2 = { };
    ASSERT_EQUAL(checked_document1, words_doc1);
    ASSERT_EQUAL(checked_document2, words_doc2);
}
void TestRelevanceSorting() {
    SearchServer examination;
    DocumentStatus status = DocumentStatus::ACTUAL;
    vector<int> rating = { 5,-2 };
    examination.AddDocument(0, "1 2 3 1", status, rating);
    examination.AddDocument(1, "3 3", status, rating);
    examination.AddDocument(2, "1 3", status, rating);
    examination.AddDocument(3, "4 5", status, rating);
    vector<int> relevanse_id = { 2,0,1 };
    vector<double> relevanse = { 0.49,0.418,0.287 };
    int i = 0;
    for (const Document& document : examination.FindTopDocuments("1 3"s)) {
        ASSERT_EQUAL_HINT(document.id, relevanse_id[i], "Relevance sorting is wrong. Document out of place"s);
        const double EPSILON = 0.001;
        ASSERT_HINT((document.relevance - relevanse[i]) < EPSILON, "Relevance is calculated incorrectly"s);
        ++i;
    }
}
void TestRankingCalculations() {
    SearchServer examination;
    DocumentStatus status = DocumentStatus::ACTUAL;
    vector<int> rating1 = { 5,-2, 3 };
    vector<int> rating2 = { 5 };
    vector<int> rating3 = { 0 };
    vector<int> rating4 = { 5 };
    examination.AddDocument(0, "2", status, rating1);
    examination.AddDocument(1, "2 1", status, rating2);
    examination.AddDocument(2, "2 1 1", status, rating3);
    examination.AddDocument(3, "1", status, rating4);
    vector<double> rating = { 2,5,0 };
    int i = 0;
    for (const Document& document : examination.FindTopDocuments("2"s)) {
        ASSERT_EQUAL(document.rating, rating[i]);
        ++i;
    }
}
void TestPredicatSorting() {
    SearchServer examination;
    DocumentStatus status = DocumentStatus::ACTUAL;
    vector<int> rating1 = { 5,-2 };
    vector<int> rating2 = { 6 };
    examination.AddDocument(0, "1 2 3 1", status, rating1);//relevanse=0.18 /idf1=0.3 idf3=0.12/tf1=0.5,tf3=0.25
    examination.AddDocument(1, "3 3", status, rating1);//relevanse=0,12 /tf1=0,tf3=1
    examination.AddDocument(2, "1 3", status, rating2);//relevanse=0,21
    examination.AddDocument(3, "4 5", status, rating1);//relevanse=0;
    ASSERT(examination.FindTopDocuments("1 3"s).size() == 3);
    ASSERT(examination.FindTopDocuments("1 3"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; }).size() == 2);
    ASSERT(examination.FindTopDocuments("1 3"s, [](int document_id, DocumentStatus status, int rating) { return rating > 5; }).size() == 1);
}
void TestStatusSorting() {
    SearchServer examination;
    vector<int> rating = { 5,-2 };
    string doc = "1 2";
    examination.AddDocument(0, doc, DocumentStatus::ACTUAL, rating);//relevanse=0.18 /idf1=0.3 idf3=0.12/tf1=0.5,tf3=0.25
    examination.AddDocument(1, doc, DocumentStatus::ACTUAL, rating);//relevanse=0,12 /tf1=0,tf3=1
    examination.AddDocument(2, doc, DocumentStatus::BANNED, rating);//relevanse=0,21
    examination.AddDocument(3, doc, DocumentStatus::IRRELEVANT, rating);//relevanse=0;
    ASSERT(examination.FindTopDocuments("1", DocumentStatus::ACTUAL).size() == 2);
    ASSERT(examination.FindTopDocuments("1", DocumentStatus::BANNED).size() == 1);
    ASSERT(examination.FindTopDocuments("1", DocumentStatus::REMOVED).size() == 0);
}

template <typename T>
void RunTestImpl(T func, const string& name) {
    func();
    cerr << name << " Test completed"s << endl;

}

#define RUN_TEST(func)  RunTestImpl(func,#func)

void TestSearchServer() {
    RUN_TEST(TestAddDocument);
    RUN_TEST(TestMinusWords);
    RUN_TEST(TestStopWords);
    RUN_TEST(TestMatchDocument);
    RUN_TEST(TestRelevanceSorting);
    RUN_TEST(TestRankingCalculations);
    RUN_TEST(TestPredicatSorting);
    RUN_TEST(TestStatusSorting);
}

void PrintDocument(const Document& document) {
    cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s << endl;
}

int main() {

    TestSearchServer();
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });
    cout << "ACTUAL by default:"s << endl;

    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s)) {
        PrintDocument(document);
    }
    cout << "BANNED:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED)) {
        PrintDocument(document);
    }
    cout << "Even ids:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }
    return 0;
}