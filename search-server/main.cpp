#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
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
    int id;
    double relevance;
};

struct Query {
    set<string> plus_words;
    set<string> minus_words;
};

class SearchServer
{
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document) {
        ++document_count_;
        vector<string> words = SplitIntoWordsNoStop(document);
        for (const string& word : words)
        {
            documents_[word].insert({ document_id,
                                     (count(words.begin(),words.end(),word)
                                      / (words.size() + 0.0)) });
        }
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        const Query query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                return lhs.relevance > rhs.relevance;
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:
    map<string, map<int, double>> documents_;
    set<string> stop_words_;
    int document_count_ = 0;

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
    Query ParseQueryWord(const string& text) const
    {
        Query words;
        for (string& word : SplitIntoWords(text))
        {
            if (word[0] == '-')
            {
                word.erase(word.begin());
                words.minus_words.insert(word);

            }
            else
            {
                words.plus_words.insert(word);
            }
        }
        return words;
    }
    Query ParseQuery(const string& text) const
    {
        Query words = ParseQueryWord(text);
        Query words_no_stop;
        words_no_stop.minus_words = words.minus_words;
        for (const string& word : words.plus_words) {
            if (!IsStopWord(word)) {
                words_no_stop.plus_words.insert(word);
            }
        }
        return words;
    }
    map<int, double> CalculationIDF(const Query& query_words) const
    {
        map<int, double> document_to_relevance;
        for (const string& word : query_words.plus_words)
        {
            if (documents_.count(word) == 1)
            {
                const double idf = log((document_count_ + 0.0) / documents_.at(word).size());
                for (const auto& [id, tf] : documents_.at(word))
                {
                    document_to_relevance[id] += (idf)*tf;
                }
            }
        }
        return document_to_relevance;
    }
    map<int, double> MatchDocument(const Query& query_words) const
    {
        map<int, double> document_to_relevance = CalculationIDF(query_words);

        for (const string& word : query_words.minus_words)
        {
            if (documents_.count(word) == 1)
            {
                for (const auto& [id, tf] : documents_.at(word))
                {
                    document_to_relevance.erase(id);
                }
            }
        }
        return document_to_relevance;
    }

    vector<Document> FindAllDocuments(const Query& query_words) const
    {
        map<int, double> document_to_relevance = MatchDocument(query_words);
        vector<Document> matched_documents;
        for (const auto& [id, relevance] : document_to_relevance)
        {
            Document doc;
            doc.id = id;
            doc.relevance = relevance;
            matched_documents.push_back(doc);
        }
        return matched_documents;
    }
};




SearchServer CreateSearchServer()
{
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id)
    {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main()
{
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query))
    {
        cout << "{ document_id = "s << document_id << ", "
            << "relevance = "s << relevance << " }"s << endl;
    }
}
