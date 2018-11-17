#ifndef HAZE_UTIL_REGISTRY_H_
#define HAZE_UTIL_REGISTRY_H_

#include <unordered_map>


template<class T>
class Registry {
    std::unordered_map<std::string, T *> map_;
public:
    Registry() {}

    virtual ~Registry() {}

    void put(std::string const& id, T *item) { map_[id] = item; }
    T *get(std::string const& id) { return map_[id]; }
    std::vector<T *> list() {
        std::vector<T *> v;
        for (auto const& kv : map_) {
            v.push_back(kv.second);
        }
        return v;
    }
};


#endif  // HAZE_UTIL_REGISTRY_H_
