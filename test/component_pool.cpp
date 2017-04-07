#include <gtest/gtest.h>
#include <component_pool.hpp>

TEST(ComponentPool, Functionalities) {
    using pool_type = entt::ComponentPool<int, double>;

    pool_type pool{0};

    ASSERT_TRUE(pool.empty<int>());
    ASSERT_TRUE(pool.empty<double>());
    ASSERT_EQ(pool.capacity<int>(), pool_type::size_type{0});
    ASSERT_EQ(pool.capacity<double>(), pool_type::size_type{0});
    ASSERT_EQ(pool.size<int>(), pool_type::size_type{0});
    ASSERT_EQ(pool.size<double>(), pool_type::size_type{0});
    ASSERT_EQ(pool.entities<int>(), pool.entities<int>() + pool.size<int>());
    ASSERT_EQ(pool.entities<double>(), pool.entities<double>() + pool.size<double>());
    ASSERT_FALSE(pool.has<int>(0));
    ASSERT_FALSE(pool.has<double>(0));
}

TEST(ComponentPool, ConstructDestroy) {
    using pool_type = entt::ComponentPool<double, int>;

    pool_type pool{4};

    ASSERT_EQ(pool.construct<int>(0, 42), 42);
    ASSERT_FALSE(pool.empty<int>());
    ASSERT_TRUE(pool.empty<double>());
    ASSERT_EQ(pool.capacity<int>(), pool_type::size_type{4});
    ASSERT_EQ(pool.capacity<double>(), pool_type::size_type{4});
    ASSERT_EQ(pool.size<int>(), pool_type::size_type{1});
    ASSERT_EQ(pool.size<double>(), pool_type::size_type{0});
    ASSERT_TRUE(pool.has<int>(0));
    ASSERT_FALSE(pool.has<double>(0));
    ASSERT_FALSE(pool.has<int>(1));
    ASSERT_FALSE(pool.has<double>(1));

    ASSERT_EQ(pool.construct<int>(1), 0);
    ASSERT_FALSE(pool.empty<int>());
    ASSERT_TRUE(pool.empty<double>());
    ASSERT_EQ(pool.capacity<int>(), pool_type::size_type{4});
    ASSERT_EQ(pool.capacity<double>(), pool_type::size_type{4});
    ASSERT_EQ(pool.size<int>(), pool_type::size_type{2});
    ASSERT_EQ(pool.size<double>(), pool_type::size_type{0});
    ASSERT_TRUE(pool.has<int>(0));
    ASSERT_FALSE(pool.has<double>(0));
    ASSERT_TRUE(pool.has<int>(1));
    ASSERT_FALSE(pool.has<double>(1));
    ASSERT_NE(pool.get<int>(0), pool.get<int>(1));
    ASSERT_NE(&pool.get<int>(0), &pool.get<int>(1));

    ASSERT_NO_THROW(pool.destroy<int>(0));
    ASSERT_FALSE(pool.empty<int>());
    ASSERT_TRUE(pool.empty<double>());
    ASSERT_EQ(pool.capacity<int>(), pool_type::size_type{4});
    ASSERT_EQ(pool.capacity<double>(), pool_type::size_type{4});
    ASSERT_EQ(pool.size<int>(), pool_type::size_type{1});
    ASSERT_EQ(pool.size<double>(), pool_type::size_type{0});
    ASSERT_FALSE(pool.has<int>(0));
    ASSERT_FALSE(pool.has<double>(0));
    ASSERT_TRUE(pool.has<int>(1));
    ASSERT_FALSE(pool.has<double>(1));

    ASSERT_NO_THROW(pool.destroy<int>(1));
    ASSERT_TRUE(pool.empty<int>());
    ASSERT_TRUE(pool.empty<double>());
    ASSERT_EQ(pool.capacity<int>(), pool_type::size_type{4});
    ASSERT_EQ(pool.capacity<double>(), pool_type::size_type{4});
    ASSERT_EQ(pool.size<int>(), pool_type::size_type{0});
    ASSERT_EQ(pool.size<int>(), pool_type::size_type{0});
    ASSERT_FALSE(pool.has<int>(0));
    ASSERT_FALSE(pool.has<double>(0));
    ASSERT_FALSE(pool.has<int>(1));
    ASSERT_FALSE(pool.has<double>(1));

    int *comp[] = {
        &pool.construct<int>(0, 0),
        &pool.construct<int>(1, 1),
        nullptr,
        &pool.construct<int>(3, 3)
    };

    ASSERT_FALSE(pool.empty<int>());
    ASSERT_TRUE(pool.empty<double>());
    ASSERT_EQ(pool.capacity<int>(), pool_type::size_type{4});
    ASSERT_EQ(pool.capacity<double>(), pool_type::size_type{4});
    ASSERT_EQ(pool.size<int>(), pool_type::size_type{3});
    ASSERT_EQ(pool.size<double>(), pool_type::size_type{0});
    ASSERT_TRUE(pool.has<int>(0));
    ASSERT_FALSE(pool.has<double>(0));
    ASSERT_TRUE(pool.has<int>(1));
    ASSERT_FALSE(pool.has<double>(1));
    ASSERT_FALSE(pool.has<int>(2));
    ASSERT_FALSE(pool.has<double>(2));
    ASSERT_TRUE(pool.has<int>(3));
    ASSERT_FALSE(pool.has<double>(3));
    ASSERT_EQ(&pool.get<int>(0), comp[0]);
    ASSERT_EQ(&pool.get<int>(1), comp[1]);
    ASSERT_EQ(&pool.get<int>(3), comp[3]);
    ASSERT_EQ(pool.get<int>(0), 0);
    ASSERT_EQ(pool.get<int>(1), 1);
    ASSERT_EQ(pool.get<int>(3), 3);

    ASSERT_NO_THROW(pool.destroy<int>(0));
    ASSERT_NO_THROW(pool.destroy<int>(1));
    ASSERT_NO_THROW(pool.destroy<int>(3));
}

TEST(ComponentPool, HasGet) {
    using pool_type = entt::ComponentPool<int, char>;

    pool_type pool;
    const pool_type &cpool = pool;

    int &comp = pool.construct<int>(0, 42);

    ASSERT_EQ(pool.get<int>(0), comp);
    ASSERT_EQ(pool.get<int>(0), 42);
    ASSERT_TRUE(pool.has<int>(0));

    ASSERT_EQ(cpool.get<int>(0), comp);
    ASSERT_EQ(cpool.get<int>(0), 42);
    ASSERT_TRUE(cpool.has<int>(0));

    ASSERT_NO_THROW(pool.destroy<int>(0));
}

TEST(ComponentPool, EntitiesReset) {
    using pool_type = entt::ComponentPool<int, char>;

    pool_type pool{2};

    ASSERT_EQ(pool.construct<int>(0, 0), 0);
    ASSERT_EQ(pool.construct<int>(2, 2), 2);
    ASSERT_EQ(pool.construct<int>(3, 3), 3);
    ASSERT_EQ(pool.construct<int>(1, 1), 1);

    ASSERT_EQ(pool.size<int>(), decltype(pool.size<int>()){4});
    ASSERT_EQ(pool.entities<int>()[0], typename pool_type::entity_type{0});
    ASSERT_EQ(pool.entities<int>()[1], typename pool_type::entity_type{2});
    ASSERT_EQ(pool.entities<int>()[2], typename pool_type::entity_type{3});
    ASSERT_EQ(pool.entities<int>()[3], typename pool_type::entity_type{1});

    pool.destroy<int>(2);

    ASSERT_EQ(pool.size<int>(), decltype(pool.size<int>()){3});
    ASSERT_EQ(pool.entities<int>()[0], typename pool_type::entity_type{0});
    ASSERT_EQ(pool.entities<int>()[1], typename pool_type::entity_type{1});
    ASSERT_EQ(pool.entities<int>()[2], typename pool_type::entity_type{3});

    ASSERT_EQ(pool.construct<char>(0, 'c'), 'c');

    ASSERT_FALSE(pool.empty<int>());
    ASSERT_FALSE(pool.empty<char>());

    ASSERT_NO_THROW(pool.reset<char>());

    ASSERT_FALSE(pool.empty<int>());
    ASSERT_TRUE(pool.empty<char>());

    ASSERT_NO_THROW(pool.reset());

    ASSERT_TRUE(pool.empty<int>());
    ASSERT_TRUE(pool.empty<char>());
}
