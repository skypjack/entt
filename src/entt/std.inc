// TODO: to export or not to export, if it was c++23 that would be easier to say
export namespace std {
using std::tuple_element;
using std::tuple_size;
using std::uses_allocator;

namespace ranges {
using std::ranges::enable_borrowed_range;
using std::ranges::enable_view;
} // namespace ranges

} // namespace std