#ifndef HAZE_UTIL_REGISTRY_H_
#define HAZE_UTIL_REGISTRY_H_

#include <unordered_map>


template<class T>
class Registry {
    std::unordered_map<std::string, T *> map_;
public:
    void put(std::string const& id, T *item) { map_[id] = item; }
    T *get(std::string const& id) { return map_[id]; }
    std::vector<T *> list();
};


#endif  // HAZE_UTIL_REGISTRY_H_
