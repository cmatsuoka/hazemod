#ifndef HAZE_UTIL_STATE_H_
#define HAZE_UTIL_STATE_H_

#include <vector>
#include <iterator>

using State = std::vector<uint32_t>;

template<class T>
State to_state(const T& object) {
    State state(sizeof(T));
    const uint8_t *begin = reinterpret_cast<const uint8_t*>(std::addressof(object)) ;
    const uint8_t *end = begin + sizeof(T) ;
    std::copy(begin, end, std::begin(state)) ;
    return state;
}

template<class T>
T& from_state(const State& state, T& object) {
    uint8_t *begin_object = reinterpret_cast<uint8_t *>(std::addressof(object)) ;
    std::copy(std::begin(state), std::end(state), begin_object) ;
    return object;
}

#endif  // HAZE_UTIL_STATE_H

