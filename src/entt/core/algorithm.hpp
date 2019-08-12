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
        if(first < last) {
            for(auto it = first+1; it < last; ++it) {
                auto value = std::move(*it);
                auto pre = it;

                for(; pre > first && compare(value, *(pre-1)); --pre) {
                    *pre = std::move(*(pre-1));
                }

                *pre = std::move(value);
            }
        }
    }
};


/*! @brief Function object for performing LSD radix sort.
 *
 * Currently works only with unsigned integer data and
 * sorts the elements in ascending order.
 *
 * @tparam bits_per_pass The value sets the number of bits processed per pass.
 * @tparam n_bits The value sets the maximum number of bits to sort.
 */
template<std::size_t bits_per_pass, std::size_t n_bits>
struct radix_sort {
    /**
     * @brief Sorts the elements in a range.
     *
     * Sorts the elements in a range using the getter function that tells
     * the algorithm which data member to use for the sorting is required.
     * The implementation was taken from an online book at
     * <a href="http://www.pbr-book.org/3ed-2018/Primitives_and_Intersection_Acceleration/Bounding_Volume_Hierarchies.html#RadixSort">link</a>.
     *
     * @tparam It Type of random access iterator.
     * @tparam Getter Type of receiving function object.
     * @param first An iterator to the first element of the range to sort.
     * @param last An iterator past the last element of the range to sort.
     * @param getter A valid receiving function object.
     */
    template<typename It, typename Getter>
    void operator()(It first, It last, Getter getter = Getter{}) const {
        if(first < last) {
            using size_type = typename std::iterator_traits<It>::value_type;

            std::vector<size_type> aux(std::distance(first, last));

            static_assert((n_bits % bits_per_pass) == 0,
                              "Radix sort bits_per_pass must evenly divide n_bits");
            constexpr uint32_t n_passes = n_bits / bits_per_pass;

            for (size_t pass = 0; pass < n_passes; ++pass) {
                // Set in and out vector iterators for radix sort pass
                if (!(pass & 1))
                    sort_internal(first, last, aux.begin(), getter, pass);
                else
                    sort_internal(aux.begin(), aux.end(), first, getter, pass);
            }

            // Move final result from _aux_ vector, if needed
            if constexpr (n_passes & 1) {
                auto it = first;
                for(auto &v : aux)
                    *(it++) = std::move(v);
            }

        }
    }

    /**
     * @brief Internal function for sorting.
     *
     *
     * @tparam It Type of random access iterator.
     * @tparam Out Type of random access iterator.
     * @tparam Getter Type of receiving function object.
     * @param first An iterator to the first element of the range to sort.
     * @param last An iterator past the last element of the range to sort.
     * @param out_begin An iterator to the first element of the vector for intermediate results.
     * @param getter A valid receiving function object.
     * @param pass A current pass to determine the position of the sortable bits.
     */
    template<typename In, typename Out, typename Getter>
    void sort_internal(In first, In last, Out out_begin, Getter getter = Getter{}, uint32_t pass = 0) const {
        uint32_t start_bit = pass * bits_per_pass;

        // Count number of zero bits in array for current radix sort bit
        constexpr int n_buckets = 1 << bits_per_pass;
        int bucket_count[n_buckets] = {0};
        constexpr int bit_mask = (1 << bits_per_pass) - 1;

        for(auto it = first; it < last; ++it) {
            int bucket = (getter(*it) >> start_bit) & bit_mask;
            ++bucket_count[bucket];
        }

        // Compute starting index in output array for each bucket
        int out_index[n_buckets];
        out_index[0] = 0;
        for (int i = 1; i < n_buckets; ++i)
            out_index[i] = out_index[i - 1] + bucket_count[i - 1];

        // Store sorted values in output array
        for(auto it = first; it < last; ++it) {
            int bucket = (getter(*it) >> start_bit) & bit_mask;
            out_begin[out_index[bucket]++] = *it;
        }
    }
};

}


#endif // ENTT_CORE_ALGORITHM_HPP
