#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>

#include "remove_duplicates.h"

using std::string_literals::operator""s;

void RemoveDuplicates(SearchServer& search_server) {

    std::set<int> id_remove;
    std::set<std::set<std::string>> words_doc;

    for (const int id : search_server) {
        std::set<std::string> words;
        for (const auto [word, freq] : search_server.GetWordFrequencies(id)) { //<--
            words.insert(word);
        }
        if (words_doc.count(words)) {
            id_remove.insert(id);
            continue;
        }
        words_doc.insert(words);
    }

    for (const int& id : id_remove) {
        std::cout << "Found duplicate document id "s << id << std::endl;
        search_server.RemoveDocument(id);
    }
}
