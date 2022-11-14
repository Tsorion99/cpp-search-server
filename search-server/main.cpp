template <typename T>
ostream& operator<<(ostream& out, const vector<T>& container) {
    if (!container.empty()) {
        out << "["s;
        bool is_first = true;
        for (const T& element : container) {
            if (!is_first) {
                out << ", "s;
            }
            out << element;
            is_first = false;
        }
        out << "]"s;
    }
    return out;
}

// -------- Начало модульных тестов поисковой системы ----------

void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
                    "Stop words must be excluded from documents"s);
    }
}

void TestAddDocument() {
    // Добавляем два документа
    const int doc_id_first = 1;
    const int doc_id_second = 2;
    const string content_doc_first = "cat in the city"s;
    const string content_doc_second = "dog in the city"s;
    const vector<int> ratings = {1, 2, 3};
    SearchServer server;
    server.AddDocument(doc_id_first, content_doc_first, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id_second, content_doc_second, DocumentStatus::ACTUAL, ratings);

    // Поиск несуществующего документа
    {
        const vector<Document> found_docs = server.FindTopDocuments("pig"s);
        ASSERT_EQUAL(found_docs.size(), 0);
    }
    // Поиск одного документа
    {
        const vector<Document> found_docs = server.FindTopDocuments("dog"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        ASSERT_EQUAL(found_docs[0].id, doc_id_second);
    }
    // Поиск двух документов
    {
        const vector<Document> found_docs = server.FindTopDocuments("pig the in"s);
        ASSERT_EQUAL(found_docs.size(), 2);
        ASSERT_EQUAL(found_docs[0].id, doc_id_first);
        ASSERT_EQUAL(found_docs[1].id, doc_id_second);
    }
}

void TestExcludeMinusWordsFromAddedDocumentContent() {
    // Добавляем два документа
    const int doc_id_first = 1;
    const int doc_id_second = 2;
    const string content_doc_first = "cat in the city"s;
    const string content_doc_second = "dog in the city"s;
    const vector<int> ratings = {1, 2, 3};
    SearchServer server;
    server.AddDocument(doc_id_first, content_doc_first, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id_second, content_doc_second, DocumentStatus::ACTUAL, ratings);

    // Поиск документа, в котором есть минус-слово
    {
        const vector<Document> found_docs = server.FindTopDocuments("cat -in"s);
        ASSERT_EQUAL(found_docs.size(), 0);
    }
    // Поиск документа, в котором нет минус-слова
    {
        const vector<Document> found_docs = server.FindTopDocuments("cat -dog"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        ASSERT_EQUAL(found_docs[0].id, doc_id_first);
    }
}

void TestMatchDocument() {
    // Добавляем два документа
    const int doc_id_first = 1;
    const int doc_id_second = 2;
    const string content_doc_first = "cat in the city"s;
    const string content_doc_second = "dog in the city"s;
    const vector<int> ratings = {1, 2, 3};
    SearchServer server;
    server.AddDocument(doc_id_first, content_doc_first, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id_second, content_doc_second, DocumentStatus::ACTUAL, ratings);

    // Поиск существующих слов документе
    {
        const tuple<vector<string>, DocumentStatus> matching_documents =
            server.MatchDocument("in cat dog", 1);
        const vector<string> matching_documents_expected = {"cat"s, "in"s};
        ASSERT_EQUAL(get<0>(matching_documents), matching_documents_expected);
    }
    // Поиск несуществующих слов документе
    {
        const tuple<vector<string>, DocumentStatus> matching_documents =
            server.MatchDocument("cat", 2);
        ASSERT_EQUAL(get<0>(matching_documents).size(), 0);
    }
    // Поиск существующих слов в документе с минус-словом
    {
        const tuple<vector<string>, DocumentStatus> matching_documents =
            server.MatchDocument("-the cat", 1);
        ASSERT_EQUAL(get<0>(matching_documents).size(), 0);
    }
}

void TestSortFindDocumentsToRelevance() {
    // Добавляем три документа
    const int doc_id_first = 1;
    const int doc_id_second = 2;
    const int doc_id_third = 3;
    const string content_doc_first = "cat in the city"s;
    const string content_doc_second = "walrus in the zoo"s;
    const string content_doc_third = "walrus with a ball"s;
    const vector<int> ratings = {1, 2, 3};
    SearchServer server;
    server.AddDocument(doc_id_first, content_doc_first, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id_second, content_doc_second, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id_third, content_doc_third, DocumentStatus::ACTUAL, ratings);

    {
        const vector<Document> found_docs = server.FindTopDocuments("walrus in the"s);
        ASSERT_EQUAL(found_docs.size(), 3);
        bool sorted = true;
        for (size_t i = 0; i < (found_docs.size() - 1) && sorted; ++i) {
            if (found_docs[i].relevance < found_docs[i + 1].relevance) {
                sorted = false;
            }
        }
        ASSERT_HINT(sorted, "Found documents are not sorted by relevance"s);
    }
}

void TestCalcDocumentRating() {
    const int doc_id = 1;
    const string content_doc = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3, 8, 13};
    SearchServer server;
    server.AddDocument(doc_id, content_doc, DocumentStatus::ACTUAL, ratings);

    const vector<Document> found_docs = server.FindTopDocuments("cat"s);
    ASSERT_EQUAL(found_docs[0].rating, 5);
}

void TestPredicateFilter() {
    // Добавляем три документа
    const int doc_id_first = 1;
    const int doc_id_second = 2;
    const int doc_id_third = 3;
    const string content_doc_first = "cat in the city"s;
    const string content_doc_second = "walrus in the zoo"s;
    const string content_doc_third = "walrus with a ball"s;
    SearchServer server;
    server.AddDocument(doc_id_first, content_doc_first, DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(doc_id_second, content_doc_second, DocumentStatus::IRRELEVANT, {1, 2, 6});
    server.AddDocument(doc_id_third, content_doc_third, DocumentStatus::BANNED, {1, 2, 15});

    // Фильтрация по статусу
    {
        const vector<Document> found_docs = server.FindTopDocuments("walrus in the"s,
                [](int document_id, DocumentStatus status, int rating) {
                return status == (DocumentStatus::BANNED) || status == (DocumentStatus::ACTUAL);
                });
        ASSERT_EQUAL(found_docs.size(), 2);
        ASSERT_EQUAL(found_docs[0].id, 1);
        ASSERT_EQUAL(found_docs[1].id, 3);
    }
    // Фильтрация по id
    {
        const vector<Document> found_docs = server.FindTopDocuments("walrus in the"s,
                [](int document_id, DocumentStatus status, int rating) {
                return document_id == 1 || document_id == 2;
                });
        ASSERT_EQUAL(found_docs.size(), 2);
        ASSERT_EQUAL(found_docs[0].id, 2);
        ASSERT_EQUAL(found_docs[1].id, 1);
    }
    // Фильтрация по рейтингу
    {
        const vector<Document> found_docs = server.FindTopDocuments("walrus in the"s,
                [](int document_id, DocumentStatus status, int rating) {
                return rating > 2;
                });
        ASSERT_EQUAL(found_docs.size(), 2);
        ASSERT_EQUAL(found_docs[0].id, 2);
        ASSERT_EQUAL(found_docs[1].id, 3);
    }
    // Фильтрация по статусу, id и рейтингу
    {
        const vector<Document> found_docs = server.FindTopDocuments("walrus in the"s,
                [](int document_id, DocumentStatus status, int rating) {
                return document_id == 2 && status == DocumentStatus::IRRELEVANT && rating == 3;
                });
        ASSERT_EQUAL(found_docs.size(), 1);
        ASSERT_EQUAL(found_docs[0].id, 2);
    }
    {
        const vector<Document> found_docs = server.FindTopDocuments("walrus in the"s,
                [](int document_id, DocumentStatus status, int rating) {
                return document_id == 3 && status == DocumentStatus::IRRELEVANT && rating == 3;
                });
        ASSERT_EQUAL(found_docs.size(), 0);
    }
}

void TestStatusFilter() {
    // Добавляем три документа
    const int doc_id_first = 1;
    const int doc_id_second = 2;
    const int doc_id_third = 3;
    const string content_doc_first = "cat in the city"s;
    const string content_doc_second = "walrus in the zoo"s;
    const string content_doc_third = "walrus with a ball"s;
    const vector<int> ratings = {1, 2, 3};
    SearchServer server;
    server.AddDocument(doc_id_first, content_doc_first, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id_second, content_doc_second, DocumentStatus::IRRELEVANT, ratings);
    server.AddDocument(doc_id_third, content_doc_third, DocumentStatus::IRRELEVANT, ratings);

    // Документы с заданным статусом существуют
    {
        const vector<Document> found_docs = server.FindTopDocuments("walrus in the"s,
            DocumentStatus::IRRELEVANT);
        ASSERT_EQUAL(found_docs.size(), 2);
        ASSERT_EQUAL(found_docs[0].id, 2);
        ASSERT_EQUAL(found_docs[1].id, 3);
    }
    // Документ с заданным статусом не существует
    {
        const vector<Document> found_docs = server.FindTopDocuments("walrus in the"s,
            DocumentStatus::BANNED);
        ASSERT_EQUAL(found_docs.size(), 0);
    }
}

void TestCalcRelevance() {
    // Добавляем три документа
    const double EPSILON = 1e-6;
    const int doc_id_first = 1;
    const int doc_id_second = 2;
    const int doc_id_third = 3;
    const string content_doc_first = "cat in the city"s;
    const string content_doc_second = "walrus in the zoo"s;
    const string content_doc_third = "walrus with a ball"s;
    const vector<int> ratings = {1, 2, 3};
    SearchServer server;
    server.AddDocument(doc_id_first, content_doc_first, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id_second, content_doc_second, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id_third, content_doc_third, DocumentStatus::ACTUAL, ratings);

    // Проверка найденных документов на релевантность
    {
        const vector<Document> found_docs = server.FindTopDocuments("walrus in the"s);
        ASSERT_EQUAL(found_docs.size(), 3);
        const double FIRST_DOC_RELEVANCE = 0.304099;
        const double SECOND_DOC_RELEVANCE = 0.202733;
        const double THIRD_DOC_RELEVANCE = 0.101366;
        ASSERT_HINT(abs(found_docs[0].relevance - FIRST_DOC_RELEVANCE) < EPSILON,
                    "Relevance calculated incorrectly"s);
        ASSERT_HINT(abs(found_docs[1].relevance - SECOND_DOC_RELEVANCE) < EPSILON,
                   "Relevance calculated incorrectly"s);
        ASSERT_HINT(abs(found_docs[2].relevance - THIRD_DOC_RELEVANCE) < EPSILON,
                   "Relevance calculated incorrectly"s);
    }
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestAddDocument);
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestExcludeMinusWordsFromAddedDocumentContent);
    RUN_TEST(TestMatchDocument);
    RUN_TEST(TestSortFindDocumentsToRelevance);
    RUN_TEST(TestCalcDocumentRating);
    RUN_TEST(TestPredicateFilter);
    RUN_TEST(TestStatusFilter);
    RUN_TEST(TestCalcRelevance);
}

// --------- Окончание модульных тестов поисковой системы -----------