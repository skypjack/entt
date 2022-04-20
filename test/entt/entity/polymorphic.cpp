#include <gtest/gtest.h>
#include <entt/entity/registry.hpp>
#include "../common/polymorphic_type.hpp"


TEST(PolyGetAny, Functionalities) {
    entt::registry reg;

    entt::entity entity1 = reg.create();  // components: cat, dog
    entt::entity entity2 = reg.create();  // components: dog, sphere, cube
    entt::entity entity3 = reg.create();  // components: fat_cat
    reg.emplace<cat>(entity1);
    reg.emplace<dog>(entity1);
    reg.emplace<dog>(entity2);
    reg.emplace<cube>(entity2);
    reg.emplace<sphere>(entity2);
    reg.emplace<fat_cat>(entity3);

    ASSERT_EQ(entt::algorithm::poly_get_any<shape>(reg, entity1), nullptr);
    ASSERT_EQ(entt::algorithm::poly_get_any<cat>(reg, entity1), reg.try_get<cat>(entity1));
    ASSERT_EQ(entt::algorithm::poly_get_any<dog>(reg, entity2), reg.try_get<dog>(entity2));
    ASSERT_EQ(entt::algorithm::poly_get_any<animal>(reg, entity2), reg.try_get<dog>(entity2));
    ASSERT_EQ(entt::algorithm::poly_get_any<fat_cat>(reg, entity3), reg.try_get<fat_cat>(entity3));
    ASSERT_EQ(entt::algorithm::poly_get_any<shape>(reg, entity3)->draw(), "sphere");
    ASSERT_EQ(entt::algorithm::poly_get_any<animal>(reg, entity3)->name(), "cat");

    ASSERT_EQ(entt::algorithm::poly_get_any<cat*>(reg, entity1), nullptr);
}


TEST(PolyGetAnyPointer, Functionalities) {
    entt::registry reg;

    cat cat1;
    dog dog1;
    dog dog2;
    cube cube2;
    sphere sphere2;
    fat_cat fat_cat3;

    entt::entity entity1 = reg.create();  // components: cat, dog
    entt::entity entity2 = reg.create();  // components: dog, sphere, cube
    entt::entity entity3 = reg.create();  // components: fat_cat
    reg.emplace<cat*>(entity1, &cat1);
    reg.emplace<dog*>(entity1, &dog1);
    reg.emplace<dog*>(entity2, &dog2);
    reg.emplace<cube*>(entity2, &cube2);
    reg.emplace<sphere*>(entity2, &sphere2);
    reg.emplace<fat_cat*>(entity3, &fat_cat3);

    ASSERT_EQ(entt::algorithm::poly_get_any<shape*>(reg, entity1), nullptr);
    ASSERT_EQ(entt::algorithm::poly_get_any<cat*>(reg, entity1), reg.get<cat*>(entity1));
    ASSERT_EQ(entt::algorithm::poly_get_any<dog*>(reg, entity2), reg.get<dog*>(entity2));
    ASSERT_EQ(entt::algorithm::poly_get_any<animal*>(reg, entity2), reg.get<dog*>(entity2));
    ASSERT_EQ(entt::algorithm::poly_get_any<fat_cat*>(reg, entity3), reg.get<fat_cat*>(entity3));
    ASSERT_EQ(entt::algorithm::poly_get_any<shape*>(reg, entity3)->draw(), "sphere");
    ASSERT_EQ(entt::algorithm::poly_get_any<animal*>(reg, entity3)->name(), "cat");

    ASSERT_EQ(entt::algorithm::poly_get_any<cat>(reg, entity1), nullptr);
}


TEST(PolyGetAll, Functionalities) {
    entt::registry reg;

    entt::entity entity1 = reg.create();  // components: cat(payload=1), dog(payload=2)
    entt::entity entity2 = reg.create();  // components: dog(payload=2), cube(payload=3), sphere(payload=4)
    entt::entity entity3 = reg.create();  // components: fat_cat(both payloads = 5)
    reg.emplace<cat>(entity1).animal_payload = 1;
    reg.emplace<dog>(entity1).animal_payload = 2;
    reg.emplace<dog>(entity2).animal_payload = 2;
    reg.emplace<cube>(entity2).shape_payload = 3;
    reg.emplace<sphere>(entity2).shape_payload = 4;
    auto& c = reg.emplace<fat_cat>(entity3);
    c.animal_payload = c.shape_payload = 5;

    {
        int count = 0;
        for(animal& a: entt::algorithm::poly_get_all<animal>(reg, entity1)) {
            ASSERT_TRUE(a.animal_payload == 1 || a.animal_payload == 2);
            ASSERT_TRUE(a.animal_payload != 1 || a.name() == "cat");
            ASSERT_TRUE(a.animal_payload != 2 || a.name() == "dog");
            count++;
        }
        ASSERT_EQ(count, 2);
    }

    {
        int count = 0;
        for (cat& a: entt::algorithm::poly_get_all<cat>(reg, entity1)) {
            ASSERT_TRUE(a.animal_payload == 1 && a.name() == "cat");
            count++;
        }
        ASSERT_EQ(count, 1);
    }

    {
        for (shape& s: entt::algorithm::poly_get_all<shape>(reg, entity1)) { FAIL() << std::addressof(s); }
    }

    {
        int count = 0;
        for(shape& s: entt::algorithm::poly_get_all<shape>(reg, entity2)) {
            ASSERT_TRUE(s.shape_payload == 3 || s.shape_payload == 4);
            ASSERT_TRUE(s.shape_payload != 3 || s.draw() == "cube");
            ASSERT_TRUE(s.shape_payload != 4 || s.draw() == "sphere");
            count++;
        }
        ASSERT_EQ(count, 2);
    }

    {
        int animal_count = 0;
        for(animal& a: entt::algorithm::poly_get_all<animal>(reg, entity3)) {
            ASSERT_TRUE(a.animal_payload == 5 && a.name() == "cat");
            animal_count++;
        }
        ASSERT_EQ(animal_count, 1);
        int shape_count = 0;
        for(shape& s: entt::algorithm::poly_get_all<shape>(reg, entity3)) {
            ASSERT_TRUE(s.shape_payload == 5 && s.draw() == "sphere");
            shape_count++;
        }
        ASSERT_EQ(shape_count, 1);
    }
}


TEST(PolyGetAllPointers, Functionalities) {
    entt::registry reg;

    cat cat1;
    dog dog1;
    dog dog2;
    cube cube2;
    sphere sphere2;
    fat_cat fat_cat3;

    entt::entity entity1 = reg.create();  // components: cat(payload=1), dog(payload=2)
    entt::entity entity2 = reg.create();  // components: dog(payload=2), cube(payload=3), sphere(payload=4)
    entt::entity entity3 = reg.create();  // components: fat_cat(both payloads = 5)
    reg.emplace<cat*>(entity1, &cat1)->animal_payload = 1;
    reg.emplace<dog*>(entity1, &dog1)->animal_payload = 2;
    reg.emplace<dog*>(entity2, &dog2)->animal_payload = 2;
    reg.emplace<cube*>(entity2, &cube2)->shape_payload = 3;
    reg.emplace<sphere*>(entity2, &sphere2)->shape_payload = 4;
    auto* c = reg.emplace<fat_cat*>(entity3, &fat_cat3);
    c->animal_payload = c->shape_payload = 5;

    {
        int count = 0;
        for(animal* a: entt::algorithm::poly_get_all<animal*>(reg, entity1)) {
            ASSERT_TRUE(a->animal_payload == 1 || a->animal_payload == 2);
            ASSERT_TRUE(a->animal_payload != 1 || a->name() == "cat");
            ASSERT_TRUE(a->animal_payload != 2 || a->name() == "dog");
            count++;
        }
        ASSERT_EQ(count, 2);
    }

    {
        int count = 0;
        for (cat* a: entt::algorithm::poly_get_all<cat*>(reg, entity1)) {
            ASSERT_TRUE(a->animal_payload == 1 && a->name() == "cat");
            count++;
        }
        ASSERT_EQ(count, 1);
    }

    {
        for (shape* s: entt::algorithm::poly_get_all<shape*>(reg, entity1)) { FAIL() << s; }
    }

    {
        int count = 0;
        for(shape* s: entt::algorithm::poly_get_all<shape*>(reg, entity2)) {
            ASSERT_TRUE(s->shape_payload == 3 || s->shape_payload == 4);
            ASSERT_TRUE(s->shape_payload != 3 || s->draw() == "cube");
            ASSERT_TRUE(s->shape_payload != 4 || s->draw() == "sphere");
            count++;
        }
        ASSERT_EQ(count, 2);
    }

    {
        int animal_count = 0;
        for(animal* a: entt::algorithm::poly_get_all<animal*>(reg, entity3)) {
            ASSERT_TRUE(a->animal_payload == 5 && a->name() == "cat");
            animal_count++;
        }
        ASSERT_EQ(animal_count, 1);
        int shape_count = 0;
        for(shape* s: entt::algorithm::poly_get_all<shape*>(reg, entity3)) {
            ASSERT_TRUE(s->shape_payload == 5 && s->draw() == "sphere");
            shape_count++;
        }
        ASSERT_EQ(shape_count, 1);
    }
}


TEST(PolyCount, Functionalities) {
    entt::registry reg;

    entt::entity entity1 = reg.create();  // components: cat, dog
    entt::entity entity2 = reg.create();  // components: dog, sphere, cube
    entt::entity entity3 = reg.create();  // components: fat_cat
    reg.emplace<cat>(entity1);
    reg.emplace<dog>(entity1);
    reg.emplace<dog>(entity2);
    reg.emplace<cube>(entity2);
    reg.emplace<sphere>(entity2);
    reg.emplace<fat_cat>(entity3);

    ASSERT_EQ(entt::algorithm::poly_count<cat>(reg, entity1), 1);
    ASSERT_EQ(entt::algorithm::poly_count<dog>(reg, entity1), 1);
    ASSERT_EQ(entt::algorithm::poly_count<animal>(reg, entity1), 2);
    ASSERT_EQ(entt::algorithm::poly_count<shape>(reg, entity1), 0);
    ASSERT_EQ(entt::algorithm::poly_count<cat>(reg, entity2), 0);
    ASSERT_EQ(entt::algorithm::poly_count<dog>(reg, entity2), 1);
    ASSERT_EQ(entt::algorithm::poly_count<shape>(reg, entity2), 2);
    ASSERT_EQ(entt::algorithm::poly_count<fat_cat>(reg, entity3), 1);
    ASSERT_EQ(entt::algorithm::poly_count<animal>(reg, entity3), 1);
    ASSERT_EQ(entt::algorithm::poly_count<shape>(reg, entity3), 1);

    ASSERT_EQ(entt::algorithm::poly_count<cube>(reg), 1);
    ASSERT_EQ(entt::algorithm::poly_count<cat>(reg), 2);
    ASSERT_EQ(entt::algorithm::poly_count<dog>(reg), 2);
    ASSERT_EQ(entt::algorithm::poly_count<sphere>(reg), 2);
    ASSERT_EQ(entt::algorithm::poly_count<animal>(reg), 4);
    ASSERT_EQ(entt::algorithm::poly_count<shape>(reg), 3);
    ASSERT_EQ(entt::algorithm::poly_count<fat_cat>(reg), 1);
}


TEST(PolyEach, Functionalities) {
    entt::registry reg;

    entt::entity entity1 = reg.create();  // components: cat(payload=1), dog(payload=2)
    entt::entity entity2 = reg.create();  // components: dog(payload=2), cube(payload=3), sphere(payload=4)
    entt::entity entity3 = reg.create();  // components: fat_cat(both payloads = 5)
    reg.emplace<cat>(entity1).animal_payload = 1;
    reg.emplace<dog>(entity1).animal_payload = 2;
    reg.emplace<dog>(entity2).animal_payload = 2;
    reg.emplace<cube>(entity2).shape_payload = 3;
    reg.emplace<sphere>(entity2).shape_payload = 4;
    auto& c = reg.emplace<fat_cat>(entity3);
    c.animal_payload = c.shape_payload = 5;

    {
        int count = 0;
        entt::algorithm::poly_each<cube>(reg, [&] (entt::entity e, cube& s) -> void {
            ASSERT_TRUE(e == entity2);
            ASSERT_TRUE(s.shape_payload == 3);
            count++;
        });
        ASSERT_EQ(count, 1);
    }

    {
        int count = 0;
        entt::algorithm::poly_each<animal>(reg, [&] (entt::entity e, animal& a) -> void {
            ASSERT_TRUE(e == entity1 || e == entity2 || e == entity3);
            if (e == entity1) {
                ASSERT_TRUE(a.animal_payload == 1 || a.animal_payload == 2);
                ASSERT_TRUE(a.animal_payload != 1 || a.name() == "cat");
                ASSERT_TRUE(a.animal_payload != 2 || a.name() == "dog");
            } else if (e == entity2) {
                ASSERT_TRUE(a.animal_payload == 2 && a.name() == "dog");
            } else if (e == entity3) {
                ASSERT_TRUE(a.animal_payload == 5 && a.name() == "cat");
            }
            count++;
        });
        ASSERT_EQ(count, 4);
    }

    {
        int count = 0;
        entt::algorithm::poly_each<shape>(reg, [&] (entt::entity e, shape& s) -> void {
            ASSERT_TRUE(e == entity2 || e == entity3);
            if (e == entity2) {
                ASSERT_TRUE(s.shape_payload == 3 || s.shape_payload == 4);
                ASSERT_TRUE(s.shape_payload != 3 || s.draw() == "cube");
                ASSERT_TRUE(s.shape_payload != 4 || s.draw() == "sphere");
            } else if (e == entity3) {
                ASSERT_TRUE(s.shape_payload == 5 && s.draw() == "sphere");
            }
            count++;
        });
        ASSERT_EQ(count, 3);
    }

    {
        entt::algorithm::poly_each<animal*>(reg, [&] (entt::entity e, [[maybe_unused]] animal* a) -> void { FAIL(); });
    }
}


TEST(PolyEachPointer, Functionalities) {
    entt::registry reg;

    cat cat1;
    dog dog1;
    dog dog2;
    cube cube2;
    sphere sphere2;
    fat_cat fat_cat3;

    entt::entity entity1 = reg.create();  // components: cat(payload=1), dog(payload=2)
    entt::entity entity2 = reg.create();  // components: dog(payload=2), cube(payload=3), sphere(payload=4)
    entt::entity entity3 = reg.create();  // components: fat_cat(both payloads = 5)
    reg.emplace<cat*>(entity1, &cat1)->animal_payload = 1;
    reg.emplace<dog*>(entity1, &dog1)->animal_payload = 2;
    reg.emplace<dog*>(entity2, &dog2)->animal_payload = 2;
    reg.emplace<cube*>(entity2, &cube2)->shape_payload = 3;
    reg.emplace<sphere*>(entity2, &sphere2)->shape_payload = 4;
    auto* c = reg.emplace<fat_cat*>(entity3, &fat_cat3);
    c->animal_payload = c->shape_payload = 5;

    {
        int count = 0;
        entt::algorithm::poly_each<cube*>(reg, [&] (entt::entity e, cube* s) -> void {
            ASSERT_TRUE(e == entity2);
            ASSERT_TRUE(s->shape_payload == 3);
            count++;
        });
        ASSERT_EQ(count, 1);
    }

    {
        int count = 0;
        entt::algorithm::poly_each<animal*>(reg, [&] (entt::entity e, animal* a) -> void {
            ASSERT_TRUE(e == entity1 || e == entity2 || e == entity3);
            if (e == entity1) {
                ASSERT_TRUE(a->animal_payload == 1 || a->animal_payload == 2);
                ASSERT_TRUE(a->animal_payload != 1 || a->name() == "cat");
                ASSERT_TRUE(a->animal_payload != 2 || a->name() == "dog");
            } else if (e == entity2) {
                ASSERT_TRUE(a->animal_payload == 2 && a->name() == "dog");
            } else if (e == entity3) {
                ASSERT_TRUE(a->animal_payload == 5 && a->name() == "cat");
            }
            count++;
        });
        ASSERT_EQ(count, 4);
    }

    {
        int count = 0;
        entt::algorithm::poly_each<shape*>(reg, [&] (entt::entity e, shape* s) -> void {
            ASSERT_TRUE(e == entity2 || e == entity3);
            if (e == entity2) {
                ASSERT_TRUE(s->shape_payload == 3 || s->shape_payload == 4);
                ASSERT_TRUE(s->shape_payload != 3 || s->draw() == "cube");
                ASSERT_TRUE(s->shape_payload != 4 || s->draw() == "sphere");
            } else if (e == entity3) {
                ASSERT_TRUE(s->shape_payload == 5 && s->draw() == "sphere");
            }
            count++;
        });
        ASSERT_EQ(count, 3);
    }

    {
        entt::algorithm::poly_each<animal>(reg, [&] (entt::entity e, [[maybe_unused]] animal& a) -> void { FAIL(); });
    }
}


TEST(PolyRemove, Functionalities) {
    entt::registry reg;

    entt::entity entity1 = reg.create();  // components: cat, dog
    entt::entity entity2 = reg.create();  // components: dog, sphere, cube
    reg.emplace<cat>(entity1);
    reg.emplace<dog>(entity1);
    reg.emplace<dog>(entity2);
    reg.emplace<cube>(entity2);
    reg.emplace<sphere>(entity2);

    ASSERT_NE(reg.try_get<cat>(entity1), nullptr);
    ASSERT_NE(entt::algorithm::poly_get_any<cat>(reg, entity1), nullptr);
    ASSERT_NE(reg.try_get<dog>(entity1), nullptr);
    ASSERT_NE(entt::algorithm::poly_get_any<dog>(reg, entity1), nullptr);
    ASSERT_EQ(entt::algorithm::poly_count<animal>(reg, entity1), 2);
    entt::algorithm::poly_remove<animal>(reg, entity1);
    ASSERT_EQ(reg.try_get<cat>(entity1), nullptr);
    ASSERT_EQ(entt::algorithm::poly_get_any<cat>(reg, entity1), nullptr);
    ASSERT_EQ(reg.try_get<dog>(entity1), nullptr);
    ASSERT_EQ(entt::algorithm::poly_get_any<dog>(reg, entity1), nullptr);
    ASSERT_EQ(entt::algorithm::poly_count<animal>(reg, entity1), 0);

    ASSERT_NE(reg.try_get<sphere>(entity2), nullptr);
    ASSERT_NE(entt::algorithm::poly_get_any<sphere>(reg, entity2), nullptr);
    ASSERT_NE(reg.try_get<cube>(entity2), nullptr);
    ASSERT_NE(entt::algorithm::poly_get_any<cube>(reg, entity2), nullptr);
    ASSERT_EQ(entt::algorithm::poly_count<shape>(reg, entity2), 2);
    entt::algorithm::poly_remove<sphere>(reg, entity2);
    ASSERT_EQ(reg.try_get<sphere>(entity2), nullptr);
    ASSERT_EQ(entt::algorithm::poly_get_any<sphere>(reg, entity2), nullptr);
    ASSERT_NE(reg.try_get<cube>(entity2), nullptr);
    ASSERT_NE(entt::algorithm::poly_get_any<cube>(reg, entity2), nullptr);
    ASSERT_EQ(entt::algorithm::poly_count<shape>(reg, entity2), 1);

    entt::entity entity3 = reg.create();  // components: fat_cat

    reg.emplace<fat_cat>(entity3);
    ASSERT_NE(reg.try_get<fat_cat>(entity3), nullptr);
    ASSERT_NE(entt::algorithm::poly_get_any<shape>(reg, entity3), nullptr);
    ASSERT_NE(entt::algorithm::poly_get_any<animal>(reg, entity3), nullptr);
    entt::algorithm::poly_remove<animal>(reg, entity3);
    ASSERT_EQ(reg.try_get<fat_cat>(entity3), nullptr);
    ASSERT_EQ(entt::algorithm::poly_get_any<shape>(reg, entity3), nullptr);
    ASSERT_EQ(entt::algorithm::poly_get_any<animal>(reg, entity3), nullptr);

    reg.emplace<fat_cat>(entity3);
    ASSERT_NE(reg.try_get<fat_cat>(entity3), nullptr);
    ASSERT_NE(entt::algorithm::poly_get_any<shape>(reg, entity3), nullptr);
    ASSERT_NE(entt::algorithm::poly_get_any<animal>(reg, entity3), nullptr);
    entt::algorithm::poly_remove<shape>(reg, entity3);
    ASSERT_EQ(reg.try_get<fat_cat>(entity3), nullptr);
    ASSERT_EQ(entt::algorithm::poly_get_any<shape>(reg, entity3), nullptr);
    ASSERT_EQ(entt::algorithm::poly_get_any<animal>(reg, entity3), nullptr);
}