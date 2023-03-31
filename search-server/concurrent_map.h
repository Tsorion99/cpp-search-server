#pragma once

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;
    };

    explicit ConcurrentMap(size_t bucket_count)
        : maps_(bucket_count),
        mutexes_(bucket_count)
    {
    }

    Access operator[](const Key& key) {
        size_t index = static_cast<uint64_t>(key) % maps_.size();
        return { std::lock_guard(mutexes_[index]), maps_[index][key] };
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;

        /* Объединение словарей */
        for (size_t i = 0; i < maps_.size(); ++i)
        {
            std::lock_guard g(mutexes_[i]);
            for (const auto& item : maps_[i]) {
                result[item.first] = item.second;
            }
        }
        return result;
    }

    void Erase(const Key& key) {
        for (size_t i = 0; i < maps_.size(); ++i)
        {
            if (maps_[i].count(key))
            {
                std::lock_guard g(mutexes_[i]);
                maps_[i].erase(key);
            }
        }
    }

private:
    std::vector<std::map<Key, Value>> maps_;
    std::vector<std::mutex> mutexes_;
};
