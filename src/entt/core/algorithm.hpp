#ifndef ENTT_CORE_ALGORITHM_HPP
#define ENTT_CORE_ALGORITHM_HPP


#include <functional>
#include <algorithm>
#include <utility>


namespace entt {


/**
 * @brief Function object to wrap `std::sort` in a class type.
 *
 * Unfortunately, `std::sort` cannot be passed as template argument to a class
 * template or a function template.<br/>
 * This class fills the gap by wrapping some flavors of `std::sort` in a
 * function object.
 */
struct std_sort {
    /**
     * @brief Sorts the elements in a range.
     *
     * Sorts the elements in a range using the given binary comparison function.
     *
     * @tparam It Type of random access iterator.
     * @tparam Compare Type of comparison function object.
     * @tparam Args Types of arguments to forward to the sort function.
     * @param first An iterator to the first element of the range to sort.
     * @param last An iterator past the last element of the range to sort.
     * @param compare A valid comparison function object.
     * @param args Arguments to forward to the sort function, if any.
     */
    template<typename It, typename Compare = std::less<>, typename... Args>
    void operator()(It first, It last, Compare compare = Compare{}, Args &&... args) const {
        std::sort(std::forward<Args>(args)..., std::move(first), std::move(last), std::move(compare));
    }
};


/*! @brief Function object for performing insertion sort. */
struct insertion_sort {
    /**
     * @brief Sorts the elements in a range.
     *
     * Sorts the elements in a range using the given binary comparison function.
     *
     * @tparam It Type of random access iterator.
     * @tparam Compare Type of comparison function object.
     * @param first An iterator to the first element of the range to sort.
     * @param last An iterator past the last element of the range to sort.
     * @param compare A valid comparison function object.
     */
    template<typename It, typename Compare = std::less<>>
    void operator()(It first, It last, Compare compare = Compare{}) const {
        if(first != last) {
            auto it = first + 1;

            while(it != last) {
                auto pre = it++;
                auto value = *pre;

                while(pre-- != first && compare(value, *pre)) {
                    *(pre+1) = *pre;
                }

                *(pre+1) = value;
            }
        }
    }
};


}


#endif // ENTT_CORE_ALGORITHM_HPP
