#include <algorithm>
#include <execution>

#include "process_queries.h"

/*! \fn ProcessQueries
 *  \b ����������  \b : ��������� ������ \n
 *  \b ����������  \b : ����������������� ��������� ���������� �������� � ���������� ������� \n
 *  \b ����������� \b : ��� \n
 *  \param[in] search_server ��������� ������ \n
 *  \param[in] queries ������� \n
 *  \return ������, ���������� ������������ ������� \n
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
 *  \b ����������  \b : ��������� ������ \n
 *  \b ����������  \b : ����������������� ��������� ���������� �������� � ���������� ������� \n
 *  \b ����������� \b : ��� \n
 *  \param[in] search_server ��������� ������ \n
 *  \param[in] queries ������� \n
 *  \return ������, ���������� ������������ ������� � ������� ���� \n
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
