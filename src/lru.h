#ifndef ATELES_LRU_H
#define ATELES_LRU_H

#include <list>
#include <stdexcept>
#include <unordered_map>


namespace ateles
{
template <typename key_type, typename value_type>
class LRU {
  public:
    typedef std::list<key_type> key_list_type;
    typedef std::pair<value_type, typename key_list_type::iterator> entry_type;
    typedef std::unordered_map<key_type, entry_type> key_map_type;

    explicit LRU(size_t capacity) : _capacity(capacity) {}

    bool contains(key_type key) {}

    value_type get(key_type key)
    {
        auto iter = this->_key_map.find(key);
        if(iter != this->_key_map.end()) {
            return iter->second.first;
        } else {
            return value_type();
        }
    }

    void put(key_type key, value_type value)
    {
        auto kiter = this->_key_map.find(key);
        if(kiter == this->_key_map.end()) {
            if(this->_key_map.size() >= this->_capacity) {
                this->evict();
            }

            auto liter = this->_key_list.insert(this->_key_list.end(), key);
            auto entry = std::make_pair(key, std::make_pair(value, liter));
            this->_key_map.insert(entry);
        } else {
            this->_key_list.splice(
                this->_key_list.end(), this->_key_list, (*kiter).second.second);
        }
    }

  private:
    void evict()
    {
        assert(!this->_key_map.empty());
        assert(!this->_key_list.empty());

        auto iter = this->_key_map.find(this->_key_list.front());
        assert(iter != this->_key_map.end());

        this->_key_map.erase(iter);
        this->_key_list.pop_front();
    }

    size_t _capacity;
    key_list_type _key_list;
    key_map_type _key_map;
};

}  // namespace ateles

#endif  // included lru.h