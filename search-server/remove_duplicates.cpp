#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>

#include "remove_duplicates.h"

using std::string_literals::operator""s;

void RemoveDuplicates(SearchServer& search_server) {

    std::set<int> id_remove;
    std::map<int, std::set<std::string>> id_to_words;


    for (const int id : search_server) {
        std::set<std::string> words;
        for (const auto [word, freq] : search_server.GetWordFrequencies(id)) { //<--
            words.insert(word);
        }
        id_to_words[id] = words;
    }

    for (auto it1 = search_server.begin(); it1 != search_server.end(); ++it1) {
        auto it2 = it1;
        ++it2;
        for (; it2 != search_server.end(); ++it2) {
            if (id_to_words[*it1] == id_to_words[*it2]) {

                id_remove.insert(*it2);
            }
        }
    }

    for (const int& id : id_remove) {
        std::cout << "Found duplicate document id "s << id << std::endl;
        search_server.RemoveDocument(id);
    }
}
