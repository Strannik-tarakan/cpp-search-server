#include <vector>
#include <string>
#include <string_view>
#include <algorithm>
#include <execution>

#include "process_queries.h"

std::vector<std::vector<Document>> ProcessQueries(const SearchServer& search_server, const std::vector<std::string>& queries) {
	std::vector<std::vector<Document>> result(queries.size());
	std::transform(std::execution::par, queries.begin(), queries.end(), result.begin(),
		[&search_server](std::string queri)
		{
			return search_server.FindTopDocuments(queri);
		});


	return result;
}

std::vector<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries)
{
	std::vector<Document> documents;
	for (std::vector<Document> document : ProcessQueries(search_server, queries)) {
		documents.insert(documents.end(), document.begin(), document.end());
	}


	return documents;
}
