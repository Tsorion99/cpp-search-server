#include <cmath>
#include <numeric>

#include "search_server.h"

SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))
{
}

SearchServer::SearchServer(const std::string_view stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))
{
}

std::set<int>::iterator SearchServer::begin() {
    return document_ids_.begin();
}

std::set<int>::iterator SearchServer::end() {
    return document_ids_.end();
}

void SearchServer::AddDocument(int document_id,
                               const std::string_view document,
                               DocumentStatus status,
                               const std::vector<int>& ratings)
{
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Invalid document_id");
    }

    const auto it = all_documenst_.insert(std::string(document));
    const auto words = SplitIntoWordsNoStop(*(it.first));
    const double inv_word_count = 1.0 / words.size();
    for (const auto &word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    document_ids_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query,
                                                     DocumentStatus status) const
{
    return FindTopDocuments(std::execution::seq,
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return static_cast<size_t>(documents_.size());
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(
    const std::string& raw_query,
    int document_id) const
{
    if (0 == document_ids_.count(document_id)) {
        throw std::out_of_range("There is no document with this document_id");
    }

    const Query query = ParseQuery(raw_query);

    bool minus_word_is = std::any_of(query.minus_words.begin(), query.minus_words.end(),
        [this, document_id](std::string_view word) {
            return word_to_document_freqs_.count(word) &&
                word_to_document_freqs_.at(word).count(document_id);
        });

    if (minus_word_is) {
        return {{}, documents_.at(document_id).status};
    }
    else {
        std::vector<std::string_view> matched_words;

        for (std::string_view word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }

        return {matched_words, documents_.at(document_id).status};
    }
}

/*! \fn SearchServer::MatchDocument
 *  \b Компонента  \b : Поисковой сервер \n
 *  \b Назначение  \b : Обертка над MatchDocument \n
 *                      Для вызова MatchDocument(execution::seq, query, document_id) \n
 *  \b Ограничения \b : Нет \n
 *  \param[in] raw_query "сырой" запрос \n
 *  \param[in] document_id идентификатор документа \n
 *  \return TODO \n
 */
std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(
    const std::execution::sequenced_policy&,
    const std::string& raw_query,
    int document_id) const
{
    return MatchDocument(raw_query, document_id);
}

/*! \fn SearchServer::MatchDocument
 *  \b Компонента  \b : Поисковой сервер \n
 *  \b Назначение  \b : Многопоточная версия MatchDocument \n
 *  \b Ограничения \b : Нет \n
 *  \param[in] raw_query "сырой" запрос \n
 *  \param[in] document_id идентификатор документа \n
 *  \return TODO \n
 */
std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(
    const std::execution::parallel_policy&,
    const std::string& raw_query,
    int document_id) const
{
    if (0 == document_ids_.count(document_id)) {
        throw std::out_of_range("There is no document with this document_id");
    }

    const Query query = ParseQuery(raw_query, false/* = sort_and_delete */);

    bool minus_word_is = std::any_of(std::execution::par,
        query.minus_words.begin(), query.minus_words.end(),
        [this, document_id](std::string_view word) {
            return word_to_document_freqs_.count(word) &&
                word_to_document_freqs_.at(word).count(document_id);
        });

    if (minus_word_is) {
        return {{}, documents_.at(document_id).status};
    }
    else {
        std::vector<std::string_view> matched_words(query.plus_words.size());

        auto last_copy_it = std::copy_if(std::execution::par,
            query.plus_words.begin(), query.plus_words.end(),
            matched_words.begin(),
            [this, document_id](std::string_view word) {
                return word_to_document_freqs_.count(word) &&
                    word_to_document_freqs_.at(word).count(document_id);
            });

        matched_words.erase(last_copy_it, matched_words.end());
        std::sort(matched_words.begin(), matched_words.end());
        auto last_unique_it = std::unique(matched_words.begin(), matched_words.end());
        matched_words.erase(last_unique_it, matched_words.end());
        return {matched_words, documents_.at(document_id).status};
    }
}

/*! \fn SearchServer::GetWordFrequencies
 *  \b Компонента  \b : Поисковой сервер \n
 *  \b Назначение  \b : Получение частот слов по идентификатору документа \n
 *  \b Ограничения \b : Нет \n
 *  \param[in] document_id идентификатор документа \n
 *  \return map, где ключ - слово, значение - частота \n
 *  если документа с идентификатором document_id не существует, то пустой словарь \n
 */
const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const
{
    static const std::map<std::string_view, double> empty_result;

    auto iterator = document_to_word_freqs_.find(document_id);
    return iterator != document_to_word_freqs_.end() ? document_to_word_freqs_.at(document_id) :
        empty_result;
}

/*! \fn SearchServer::RemoveDocument
 *  \b Компонента  \b : Поисковой сервер \n
 *  \b Назначение  \b : Удаление документа \n
 *  \b Ограничения \b : Нет \n
 *  \param[in] document_id идентификатор документа \n
 *  \return Нет \n
 */
void SearchServer::RemoveDocument(int document_id) {
    if (0 == document_ids_.count(document_id)) {
        return;
    }

    for (const auto& [word, frequency] : document_to_word_freqs_.at(document_id)) {
        word_to_document_freqs_.at(word).erase(document_id);
    }
    document_to_word_freqs_.erase(document_id);
    documents_.erase(document_id);
    document_ids_.erase(document_id);
}

/*! \fn SearchServer::RemoveDocument
 *  \b Компонента  \b : Поисковой сервер \n
 *  \b Назначение  \b : Обертка над RemoveDocument \n
 *                      Для вызова RemoveDocument(execution::seq, number) \n
 *  \b Ограничения \b : Нет \n
 *  \param[in] document_id идентификатор документа \n
 *  \return Нет \n
 */
void SearchServer::RemoveDocument(const std::execution::sequenced_policy&, int document_id) {
    RemoveDocument(document_id);
}

/*! \fn SearchServer::RemoveDocument
 *  \b Компонента  \b : Поисковой сервер \n
 *  \b Назначение  \b : Многопоточная версия RemoveDocument \n
 *  \b Ограничения \b : Нет \n
 *  \param[in] document_id идентификатор документа \n
 *  \return Нет \n
 */
void SearchServer::RemoveDocument(const std::execution::parallel_policy&, int document_id) {
    if (0 == document_ids_.count(document_id)) {
        return ;
    }

    {
        std::vector<std::string_view> words(document_to_word_freqs_.at(document_id).size());
        std::transform(std::execution::par,
            document_to_word_freqs_.at(document_id).cbegin(),
            document_to_word_freqs_.at(document_id).cend(),
            words.begin(),
            [](const auto &value) {return value.first;});
        std::for_each(std::execution::par,
            words.cbegin(), words.cend(),
            [this, document_id](const auto &word) {word_to_document_freqs_.at(word).erase(document_id);});
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

bool SearchServer::IsStopWord(std::string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(std::string_view word) {
    return std::none_of(word.begin(), word.end(), [](char c) {
               return c >= '\0' && c < ' ';
           });
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(std::string_view text) const {
    std::vector<std::string_view> words;

    for (const auto &word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Word " + std::string(word) + " is invalid");
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

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty");
    }

    bool is_minus = false;
    if (text[0] == '-') {
        is_minus = true;
        text.remove_prefix(1);
    }
    if (text.empty() || text[0] == '-' || !IsValidWord(text)) {
        throw std::invalid_argument("Query word " + std::string(text) + " is invalid");
    }

    return { text, is_minus, IsStopWord(text) };
}

SearchServer::Query SearchServer::ParseQuery(std::string_view text,
                                             bool sort_and_delete) const
{
    Query result;

    for (std::string_view word : SplitIntoWords(text)) {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }

    if (sort_and_delete) {
        std::sort(result.plus_words.begin(), result.plus_words.end());
        std::sort(result.minus_words.begin(), result.minus_words.end());

        auto last_pw = std::unique(result.plus_words.begin(), result.plus_words.end());
        result.plus_words.erase(last_pw, result.plus_words.end());
        auto last_mw = std::unique(result.minus_words.begin(), result.minus_words.end());
        result.minus_words.erase(last_mw, result.minus_words.end());
    }
    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(std::string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}
