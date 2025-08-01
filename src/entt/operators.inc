export namespace entt {
using ::entt::operator==;
using ::entt::operator!=;
using ::entt::operator<;
using ::entt::operator<=;
using ::entt::operator>;
using ::entt::operator>=;

using ::entt::operator+;
} // namespace entt

// global operators
export {
    // from entt/core/enum.h
    using ::operator|;
    using ::operator&;
    using ::operator^;
    using ::operator~;
    using ::operator!;
    using ::operator|=;
    using ::operator&=;
    using ::operator^=;
}
