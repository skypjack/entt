#ifndef ENTT_META_ADL_POINTER_HPP
#define ENTT_META_ADL_POINTER_HPP


namespace entt {


/**
 * @brief ADL based lookup function for dereferencing meta pointer-like types.
 * @tparam Type Element type.
 * @param value A pointer-like object.
 * @return The value returned from the dereferenced pointer.
 */
template<typename Type>
decltype(auto) dereference_meta_pointer_like(const Type &value) {
    return *value;
}


/**
 * @brief Fake ADL based lookup function for meta pointer-like types.
 * @tparam Type Element type.
 */
template<typename Type>
struct adl_meta_pointer_like {
    /**
     * @brief Uses the default ADL based lookup method to resolve the call.
     * @param value A pointer-like object.
     * @return The value returned from the dereferenced pointer.
     */
    static decltype(auto) dereference(const Type &value) {
        return dereference_meta_pointer_like(value);
    }
};


}


#endif
