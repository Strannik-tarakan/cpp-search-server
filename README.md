# cpp-search-server
The program is a method of searching for documents in a search server. The FindAllDocuments method takes as input an execution policy (ExecutionPolicy), a query (Query), and a predicate (DocumentPredicate) and returns a vector of documents that satisfy the query and predicate.

Inside the method, the relevance of each document is calculated based on the weighting coefficients of the query words and the inverse document frequency for each word. Then the documents are filtered based on the predicate and query (plus words and negative words). Finally, all matching documents are collected into a vector and returned as the result of the method execution.

Thus, the program provides the functionality of searching for documents taking into account the specified query criteria and predicate.
