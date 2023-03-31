#include <algorithm>
#include <execution>

#include "process_queries.h"

/*! \fn ProcessQueries
 *  \b Компонента  \b : Поисковой сервер \n
 *  \b Назначение  \b : Распараллеливание обработки нескольких запросов к поисковому серверу \n
 *  \b Ограничения \b : Нет \n
 *  \param[in] search_server поисковый сервер \n
 *  \param[in] queries запросы \n
 *  \return вектор, содержащий обработанные запросы \n
 */
std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries)
{
    std::vector<std::vector<Document>> result(queries.size());

    std::transform(std::execution::par,
        queries.begin(), queries.end(),
        result.begin(),
        [&search_server](const auto& query) {return search_server.FindTopDocuments(query);});
    return result;
}

/*! \fn ProcessQueriesJoined
 *  \b Компонента  \b : Поисковой сервер \n
 *  \b Назначение  \b : Распараллеливание обработки нескольких запросов к поисковому серверу \n
 *  \b Ограничения \b : Нет \n
 *  \param[in] search_server поисковый сервер \n
 *  \param[in] queries запросы \n
 *  \return вектор, содержащий обработанные запросы в плоском виде \n
 */
std::list<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries)
{
    std::list<Document> result;

    std::vector<std::vector<Document>> process_queries = ProcessQueries(search_server, queries);
    for (const auto& documents : process_queries) {
        for (const auto& document : documents) {
            result.push_back(document);
        }
    }
    return result;
}
