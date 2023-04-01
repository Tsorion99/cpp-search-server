#pragma once

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

    struct Bucket {
        std::map<Key, Value> map;
        std::mutex mutex;
    };

    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;
    };

    explicit ConcurrentMap(size_t bucket_count)
        : buckets_(bucket_count)
    {
    }

    Access operator[](const Key& key) {
        size_t index = static_cast<uint64_t>(key) % buckets_.size();
        return { std::lock_guard(buckets_[index].mutex), buckets_[index].map[key] };
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;

        /* Объединение словарей */
        for (auto& bucket : buckets_)
        {
            std::lock_guard g(bucket.mutex);
            for (const auto& item : bucket.map) {
                result[item.first] = item.second;
            }
        }
        return result;
    }

    void Erase(const Key& key) {
        for (auto& bucket : buckets_)
        {
            if (bucket.map.count(key))
            {
                std::lock_guard g(bucket.mutex);
                bucket.map.erase(key);
            }
        }
    }

private:
    std::vector<Bucket> buckets_;
};
