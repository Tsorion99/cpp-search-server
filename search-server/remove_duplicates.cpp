#include <map>
#include <set>
#include <iostream>

#include "remove_duplicates.h"

/*! \fn RemoveDuplicates
 *  \b Компонента  \b : Поисковой сервер \n
 *  \b Назначение  \b : Удаление дубликатов документов из поискового сервера.
 *                      Среди двух дубликатов удаляется документ с большим id \n
 *  \b Ограничения \b : Нет \n
 *  \param[in] search_server поисковый сервер
 *  \return Нет
 */
void RemoveDuplicates(SearchServer& search_server) {
    std::set<std::set<std::string>> documents;
    std::set<int> duplicate_documents_id;

    for (const int document_id : search_server) {
        const std::map<std::string, double>& document =
            search_server.GetWordFrequencies(document_id);
        std::set<std::string> words_in_document;

        for (const auto& [word, frequency] : document) {
            words_in_document.insert(word);
        }

        if (documents.count(words_in_document)) {
            duplicate_documents_id.insert(document_id);
        }
        else {
            documents.insert(words_in_document);
        }
    }

    for (const int document_id : duplicate_documents_id) {
        search_server.RemoveDocument(document_id);
        std::cout << "Found duplicate document id " << document_id << std::endl;
    }
}
