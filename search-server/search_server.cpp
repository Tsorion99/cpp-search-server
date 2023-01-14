#include <cmath>
#include <numeric>
#include "search_server.h"

SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(
      SplitIntoWords(stop_words_text))
{
}

std::set<int>::iterator SearchServer::begin() {
    return document_ids_.begin();
}

std::set<int>::iterator SearchServer::end() {
    return document_ids_.end();
}

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status,
                const std::vector<int>& ratings) {
    using namespace std;
    
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw invalid_argument("Invalid document_id"s);
    }
    const auto words = SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / words.size();
    for (const string& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    document_ids_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
    return FindTopDocuments(
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query,
                                                   int document_id) const {
    //LOG_DURATION_STREAM("Operation time", std::cout);

    const auto query = ParseQuery(raw_query);

    std::vector<std::string> matched_words;
    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }
    return {matched_words, documents_.at(document_id).status};
}

/*! \fn SearchServer::GetWordFrequencies
 *  \b Компонента  \b : Поисковой сервер \n
 *  \b Назначение  \b : Получение частот слов по идентификатору документа \n
 *  \b Ограничения \b : Нет \n
 *  \param[in] document_id идентификатор документа
 *  \return map, где ключ - слово, значение - частота
 *  если документа с идентификатором document_id не существует, то пустой словарь
 */
const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const
{
    static const std::map<std::string, double> empty_result;

    auto iterator = document_to_word_freqs_.find(document_id);
    return iterator != document_to_word_freqs_.end() ? document_to_word_freqs_.at(document_id) :
        empty_result;
}

/*! \fn SearchServer::RemoveDocument
 *  \b Компонента  \b : Поисковой сервер \n
 *  \b Назначение  \b : Удаление документа \n
 *  \b Ограничения \b : Нет \n
 *  \param[in] document_id идентификатор документа
 *  \return Нет
 */
void SearchServer::RemoveDocument(int document_id) {
    {
        auto iterator = document_ids_.find(document_id);
        if (iterator == document_ids_.end()) {
            return ;
        }
    }

    for (const auto& [word, frequency] : document_to_word_freqs_.at(document_id)) {
        std::map<int, double>& internal_map = word_to_document_freqs_.at(word);
        internal_map.erase(document_id);
        if (internal_map.empty()) {
            word_to_document_freqs_.erase(word);
        }
    }

    {
        auto iterator = document_to_word_freqs_.find(document_id);
        document_to_word_freqs_.erase(iterator);
    }

    {
        auto iterator = documents_.find(document_id);
        documents_.erase(iterator);
    }

    {
        auto iterator = document_ids_.find(document_id);
        document_ids_.erase(iterator);
    }
}

bool SearchServer::IsStopWord(const std::string& word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const std::string& word) {
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    using namespace std;
    
    vector<string> words;
    for (const string& word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument("Word "s + word + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string& text) const {
    using namespace std;
    
    if (text.empty()) {
        throw invalid_argument("Query word is empty"s);
    }
    string word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw invalid_argument("Query word "s + text + " is invalid");
    }

    return {word, is_minus, IsStopWord(word)};
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
    Query result;

    for (const std::string& word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.insert(query_word.data);
            } else {
                result.plus_words.insert(query_word.data);
            }
        }
    }
    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}
