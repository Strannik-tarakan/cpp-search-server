
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
#include <optional>


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
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }

        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }


    return words;
}

struct Document {
    Document()
        :id(0), relevance(0), rating(0)
    {}
    Document(int _id, double _relevance, int _rating)
        :id(_id), relevance(_relevance), rating(_rating)
    {}
    int id;
    double relevance;
    int rating;
};

class SearchServer {
public:
    SearchServer() {
    }
    explicit SearchServer(const string& stop_words) :SearchServer(SplitIntoWords(stop_words))
    {}
    template <typename Сollection>
    explicit SearchServer(const Сollection& stop_words)
    {
        CheckValidWord(stop_words);
        for (const string& word : stop_words) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        if (document_id < 0) {
            throw invalid_argument("trying to add a document with a negative id"s);
        }
        if (document_info_.count(document_id)) {
            throw invalid_argument("attempt to add a document with an existing id"s);
        }
        CheckValidWord(document);
        document_count_.push_back(document_id);
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        document_info_[document_id] = { ComputeAverageRating(ratings),status };

    }

    int GetDocumentCount() const {
        return static_cast<int>(document_count_.size());
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {

        if (!document_info_.count(document_id)) {
            throw invalid_argument("There is no document with this id"s);
        }


        Query query = ParseQuery(raw_query);

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
        tuple<vector<string>, DocumentStatus> result = { plus_words_document, status };
        return result;
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status_document) const {
        return FindTopDocuments(raw_query, [status_document](int document_id, DocumentStatus status, int rating) { return  status == status_document; });
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        DocumentStatus status_document = DocumentStatus::ACTUAL;
        return FindTopDocuments(raw_query, [status_document](int document_id, DocumentStatus status, int rating) { return  status == status_document; });
    }

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const {


        Query query = ParseQuery(raw_query);

        auto matched_documents = FindDesiredStatusDocuments(query, document_predicate);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                const double EPSILON = 1e-6;
                return lhs.relevance > rhs.relevance || ((abs(lhs.relevance - rhs.relevance) < EPSILON) && lhs.rating > rhs.rating);
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        vector<Document> result = matched_documents;
        return result;
    }

    int GetDocumentId(int index) const {
        if (index < 0 || index >= static_cast<int>(document_count_.size())) {

            throw out_of_range("invalid index. Documents - "s + to_string(SearchServer::GetDocumentCount()));

        }
        return document_count_[index];
    }

private:
    inline static constexpr int INVALID_DOCUMENT_ID = -1;
    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    vector<int> document_count_;
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
        QueryWord query_word;
        bool is_minus = false;
        // Word shouldn't be empty
        if (static_cast<int>(text.size()) == 1 && text[0] == '-') {
            throw invalid_argument("expected word after '-'"s);
        }
        else if (text[1] == '-' && text[0] == '-') {
            throw invalid_argument("Two '-' characters in a row"s);
        }
        else if (text[static_cast<int>(text.size() - 1)] == '-') {
            throw invalid_argument("Invalid character '-' at the end of a word"s);
        }
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        query_word = { text,is_minus,IsStopWord(text) };
        return query_word;
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        CheckValidWord(text);
        for (const string& word : SplitIntoWords(text)) {
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

    static void CheckValidWord(const string& words) {
        if (any_of(words.begin(), words.end(), [](char c) {
            return c >= '\0' && c < ' '; })) {
            throw invalid_argument("stop_words contains invalid characters"s);
        }
    }

    template <typename Сollection>
    static void CheckValidWord(const Сollection& words) {
        for (const string& word : words) {
            if (any_of(word.begin(), word.end(), [](char c) {
                return c >= '\0' && c < ' '; })) {
                throw invalid_argument("stop_words contains invalid characters"s);
            }
        }

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
    ASSERT(examination.FindTopDocuments("fgh"s).empty());
    ASSERT_EQUAL_HINT(examination.FindTopDocuments("good"s).size(), 1, "Document not added or cannot be found"s);
}
void TestMinusWords() {
    SearchServer examination;
    DocumentStatus status = DocumentStatus::ACTUAL;
    vector<int> rating = { 5,-2 };
    examination.AddDocument(0, "good white dog"s, status, rating);
    examination.AddDocument(1, "good white black dog"s, status, rating);
    examination.AddDocument(2, "fghjk"s, status, rating);
    ASSERT_EQUAL(examination.FindTopDocuments("good"s).size(), 2);
    ASSERT_EQUAL(examination.FindTopDocuments("good -black"s).size(), 1);

}
void TestStopWords() {
    SearchServer examination("in is a"s);
    set<string> stop_w1 = { "in"s, "is"s, "a"s };
    vector<string> stop_w2 = { "in"s, "is"s, "a"s };
    SearchServer examination2(stop_w1);
    SearchServer examination3(stop_w2);
    DocumentStatus status = DocumentStatus::ACTUAL;
    vector<int> rating = { 5,-2 };
    examination.AddDocument(0, "good in white dog"s, status, rating);
    examination.AddDocument(1, "good is white a black dog"s, status, rating);
    examination2.AddDocument(0, "good in white dog"s, status, rating);
    examination2.AddDocument(1, "good is white a black dog"s, status, rating);
    examination3.AddDocument(0, "good in white dog"s, status, rating);
    examination3.AddDocument(1, "good is white a black dog"s, status, rating);
    const auto& [checked_document_1, id1] = examination.MatchDocument("good in white dog"s, 0);
    const auto& [checked_document_2, id2] = examination.MatchDocument("good in white a dog"s, 1);
    const auto& [checked_document_2_1, id21] = examination2.MatchDocument("good in white dog"s, 0);
    const auto& [checked_document_2_2, id22] = examination2.MatchDocument("good in white a dog"s, 1);
    const auto& [checked_document_3_1, id31] = examination3.MatchDocument("good in white dog"s, 0);
    const auto& [checked_document_3_2, id32] = examination3.MatchDocument("good in white a dog"s, 1);
    vector<string> words_doc = { "dog"s,"good"s,"white"s };

    ASSERT_EQUAL(checked_document_1, words_doc);
    ASSERT_EQUAL(checked_document_2, words_doc);
    ASSERT_EQUAL(checked_document_2_1, words_doc);
    ASSERT_EQUAL(checked_document_2_2, words_doc);
    ASSERT_EQUAL(checked_document_3_1, words_doc);
    ASSERT_EQUAL(checked_document_3_2, words_doc);
}
void TestMatchDocument_() {
    SearchServer examination;
    DocumentStatus status = DocumentStatus::ACTUAL;
    vector<int> rating = { 5,-2 };
    examination.AddDocument(0, "good black white dog"s, status, rating);
    const auto& [checked_document1, id1] = examination.MatchDocument(" good  white dog"s, 0);
    const auto& [checked_document2, id2] = examination.MatchDocument("good -white a dog"s, 0);
    vector<string> words_doc1 = { "dog"s,"good"s,"white"s };
    vector<string> words_doc2 = { };
    ASSERT_EQUAL(checked_document1, words_doc1);
    ASSERT_EQUAL(checked_document2, words_doc2);
}
void TestRelevanceSorting() {
    SearchServer examination;
    DocumentStatus status = DocumentStatus::ACTUAL;
    vector<int> rating = { 5,-2 };
    examination.AddDocument(0, "1 2 3 1"s, status, rating);
    examination.AddDocument(1, "3 3"s, status, rating);
    examination.AddDocument(2, "1 3"s, status, rating);
    examination.AddDocument(3, "4 5"s, status, rating);
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
    examination.AddDocument(0, "2"s, status, rating1);
    examination.AddDocument(1, "2 1"s, status, rating2);
    examination.AddDocument(2, "2 1 1"s, status, rating3);
    examination.AddDocument(3, "1"s, status, rating4);
    vector<double> rating = { 2,5,0 };
    int i = 0;
    for (const Document& document : examination.FindTopDocuments("2"s)) {
        ASSERT_EQUAL(document.rating, rating[i]);
        ++i;
    }
}
void TestPredicat() {
    SearchServer examination;
    DocumentStatus status = DocumentStatus::ACTUAL;
    vector<int> rating1 = { 5,-2 };
    vector<int> rating2 = { 6 };
    examination.AddDocument(0, "1 2 3 1"s, status, rating1);//relevanse=0.18 /idf1=0.3 idf3=0.12/tf1=0.5,tf3=0.25
    examination.AddDocument(1, "3 3"s, status, rating1);//relevanse=0,12 /tf1=0,tf3=1
    examination.AddDocument(2, "1 3"s, status, rating2);//relevanse=0,21
    examination.AddDocument(3, "4 5"s, status, rating1);//relevanse=0;
    ASSERT(examination.FindTopDocuments("1 3"s).size() == 3);
    ASSERT(examination.FindTopDocuments("1 3"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; }).size() == 2);
    ASSERT(examination.FindTopDocuments("1 3"s, [](int document_id, DocumentStatus status, int rating) { return rating > 5; }).size() == 1);
}
void TestStatusSorting() {
    SearchServer examination;
    vector<int> rating = { 5,-2 };
    string doc = "1 2"s;
    examination.AddDocument(0, doc, DocumentStatus::ACTUAL, rating);//relevanse=0.18 /idf1=0.3 idf3=0.12/tf1=0.5,tf3=0.25
    examination.AddDocument(1, doc, DocumentStatus::ACTUAL, rating);//relevanse=0,12 /tf1=0,tf3=1
    examination.AddDocument(2, doc, DocumentStatus::BANNED, rating);//relevanse=0,21
    examination.AddDocument(3, doc, DocumentStatus::IRRELEVANT, rating);//relevanse=0;
    ASSERT(examination.FindTopDocuments("1"s, DocumentStatus::ACTUAL).size() == 2);
    ASSERT(examination.FindTopDocuments("1"s, DocumentStatus::BANNED).size() == 1);
    ASSERT(examination.FindTopDocuments("1"s, DocumentStatus::REMOVED).size() == 0);
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
    RUN_TEST(TestMatchDocument_);
    RUN_TEST(TestRelevanceSorting);
    RUN_TEST(TestRankingCalculations);
    RUN_TEST(TestPredicat);
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
    try {
        SearchServer search_server("и в на"s);

        vector<string> a = { "s","in" };
        SearchServer search_server2(a);

        // Явно игнорируем результат метода AddDocument, чтобы избежать предупреждения
        // о неиспользуемом результате его вызова
        search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
        search_server.AddDocument(2, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 });
        search_server.GetDocumentId(1);
        for (const Document& document : search_server.FindTopDocuments("пушистый"s)) {
            PrintDocument(document);
        }

        search_server.MatchDocument("пушистый"s, 1);


    }
    catch (const invalid_argument& e) {
        cout << "Ошибка: "s << e.what() << endl;
    }
    catch (const out_of_range& e) {
        cout << "Ошибка: "s << e.what() << endl;
    }
}
