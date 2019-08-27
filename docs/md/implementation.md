## Intro

The below is a breakdown of how [EnTT](https://github.com/skypjack/entt) works under the hood.

# Part 1 - Essentials: Pools and Views

## Pools

A pool is a "sparse set" data structure, containing the following for each component type:

### EntityIndices

* A sparse array
* Contains integers which are the indices in EntityList. 
* The index (not the value) of this sparse array is itself the entity id.

### EntityList

* A packed array
* Contains integers - which are the entity ids themselves
* The index doesn't have inherent meaning, other than it must be correct from EntityIndices

### ComponentList

* A packed array
* Contains component data (of this pool type)
* It is aligned with EntityList such that the element at EntityList[N] has component data of ComponentList[N]

When entities are removed from the world, or they have this component type removed, all the above are affected:

1. The index of EntityIndices, equal to the value of the entity, is removed (leaving a hole)
2. EntityList which contains the entity is removed (easy to find via EntityIndices) - and the Array is repacked
3. The ComponentList which is at that same index is also removed and repacked the same way

When entities have a component type added, all the above are similarly affected:

1. The index of EntityIndices, equal to the value of the entity, is filled (with the value of EntityList.length)
2. EntityList is appended with the entityId
3. ComponentList is appended with the component data

Due to the structure, these operations are all very quick! (for example - repacking is just a swap+pop or a push)

And it is _extremely_ quick/cache-efficient to iterate over all the entities or components of a given component type.

A Pool can also be sorted efficiently since it is built out of 3 simple inter-related arrays.

## Views

A view is essentially an answer to the query: "what are all the entities that have all these components? - And give me their component data too!".

In other words, it can return something like [(Entity, [ComponentDataForType1, ComponentDataForTypeN..])] for a given list of ComponentTypes

This happens by first getting the smallest pool requested, and then iterating over this list to get each entity and component.

Using these entities - it must first check that the other pool(s) indeed have a valid component for the given entity. If so, it's added to the list.

This walking and checking is often not terrible performance, since they are direct array lookups, but the indirection is a penalty and there might be extra allocations to group things together. However, the penalties here can be optimized away completely via owned groups and mitigated via other group types too...

# Part 2 - Optimizations: Groups, Pages, and Recycling

## Owned Groups

Owned groups are an optimization strategy that allows different pools to be aligned and culled such that views of those pools, for a given group, require no jumping or even checks for an component. It's as fast as can be from every perspective!

It does this by moving all the elements of the each pool in the group to the front of their arrays (i.e. starting at 0) and maintaining a marker for where it ends.

Whenever an element enters the group, it is moved to the left of the split and the split is moved right. Leaving the group is similar (element is moved to the end of the arrays and split is moved left).

Because groups change the their owned pools such that all their elements begin at 0 and follow the same order of entity indexes, until the end of the group - a pool cannot be owned by multiple groups. 

Note that groups can be sorted manually, as will - it simply means that all the pools must be sorted the same way for the group portion.

## Partially owned groups

Partially owned groups are groups that only own some of their component types. They take the same approach as above, but only affect the ordering of their owned groups. This is still a win for two reasons: 1 those owned groups will be iterated linearly when taking a view of this group, and 2) the checks for component existance can be avoided since the group already ensures its existance.

## Non-owned groups

There is a possibility for grouping components that aren't owned at all. The direct performance benefit is only avoiding the check for component existance during iteration (but at the cost of the group overhead). The bigger benefit of this approach is that the non-owned groups can be ordered in a way that is totally independant of re-ordering each pool. It's implemented by maintaining a its own sparse array, with only one dense array (the entity ids - no component data).

In other words, the group can be iterated like this (pseudocode) and the entity/component results will be in the order of the group:

```
for entityId in nonOwnedGroup.denseArray {
  for targetPool in targetPools {
    index = targetPool.sparseArray[entityId];
    componentData = targetPool.denseComponentArray[index];
  }
}
```

## Organizing Sparse Arrays into paging

Imagine the following scenario: an entity at index 100,000 contains a certain component.
That pool's sparse array will need to allocate the entire amount - even though there are holes all the way to that index. This is a huge waste!

Instead, the sparse array can be split into Pages of N bytes, and the page will only be allocated as-needed. If only the 10th page is needed, that means 9 pages worth of data are never even allocated.

Major savings!

## Entity recycling

All the above could work with every new entity being the next value of some internal counter. This would be incredibly wasteful, however, if entities are not only created, but destroyed too!

Rather - we want to re-use the same space that was already allocated for the existing entities. This is not only true in general, but it's especially true in this architecture since sparse arrays are always as big as the largest entity id / index (paging helps here- but not entirely).

If we simply re-use the entity id's as-is, we have a major bug, unfortunately. Since entities that are not _actually_ equal (say - entity `42` which was destroyed, and the new entity `42` which was created), will be equal in game logic since there's no way to tell if 42 != 42. Big problem!

A straightforward way to work around this is by splitting the number in two. Some of the bits are used to represent the straight index, and the other bits are used to represent the "generation" or "version" (we'll use "version" for the terminology here). Whenever one of these numbers is recycled, the `version` is bumped. Comparisons must check both the `id` and `version` parts of the number in order to ensure equality. 

(note - splitting a number isn't a strict requirement, it could also be a tuple,struct/array of its own... but usually the version part doesn't need to be so large and simply using a few bits of a number is a nice convenience)

## Finding an available entity

This version or generational approach works for re-using space and still having strict equality checks... but how do we find an entity which is actually available for recycling?

Or in other words - how do we find an index, which has no components in any pool (i.e. it's a hole in every pool's sparse array), so that we can bump the version (to invalidate it) when it's killed, and re-assign it to a pool when it's resurrected - without actually checking through each pool's sparse array?

To use common terminology, let's say that the `id` portion of an entity is `entityId` and the generation or version is `version`. When they're combined into one number it's simply `entity`. Also, the max possible value is prefixed with INVALID, e.g. `INVALID_ID` and `INVALID_VERSION`

The concept is essentially a linked list, done efficiently with bitmasks and re-purposing the meaning of the values. 

When an entity is alive, the `entityId` at a given index matches the index. When an entity is recyclable, the `entityId` points elsewhere (to the next available entity for recycling, or if there is none- anywhere else but the index)

### Step-by-step walkthrough 

We need: 

1. `entities`: an array containing all the entities (whether dead or alive doesn't matter - it's a complete list)
2. `destroyed`: an integer which points at the next available `entityId` that can be recycled
3. Some way of knowing if `destroyed` exists, as well as knowing if `destroyed` points to a valid index. In EnTT, `null` is used for both (`null` turned into an integer is `INVALID_ID`). If an `Option/Maybe` wrapper were used then these are two distinct checks (In other words, `Some(INVALID_ID) != None`).

When adding an `entity` and `destroyed` does not exist
* a new `entity` is appended where:
  * `entityId` is the length of `entities`
  * `version` is 0
* `destroyed` is unchanged 
* this entity is returned

When deleting an `entity`:

* the `version` at `entities[entityId]` is bumped
* the `entityId` at `entities[entityId]` is set to `destroyed`. If `destroyed` does not exist then it is instead set to `INVALID_ID` (note: these are equivilent in EnTT since `null` as an integer is `INVALID_ID`)
* `destroyed` is set to `entityId` 

When adding an `entity` and `destroyed` exists 

* `destroyed` is stashed as `index`
* if the `entityId` at `entities[index]` is `INVALID_ID` then `destroyed` is removed (e.g. set to `null` or `None`. In EnTT this is not an extra step since `null` as an integer is `INVALID_ID`)
* otherwise, the `entityId` is the next valid free slot, and `destroyed` is set to it.
* the `entityId` at `entities[index]` is set to `index` (e.g. what was previously `destroyed`)
* the entity is returned

# Part 3 - Bonus mechanisms

## Events / Signals

Events or signals may be emitted whenever the internal structure is changed. This can be used as an internal implementation detail-  for example, groups can work by listening for "add/remove" component signals, and then doing its work on a pool as needed.

It can also be used to tie the ECS to an external system, or to trigger systems to run, etc.

## Serialization

Since everything exists in simple plain objects - it's actually possible to serialize and deserialize an entire game state!

Of course this depends on the component data itself being serializable, but the ECS is not a limiting factor

# Footnotes

* Many more things can be built on top of ECS or at least without contradiction to it. For example, EnTT includes a whole [signals/event/observable architecture](https://github.com/skypjack/entt/wiki/Crash-Course:-events,-signals-and-everything-in-between)

* Careful grouping is important to get the biggest benefit. This requires knowing your data and shaping the structures at compile-time, according to the needs at runtime. In real-world scenarios it's often impossible to have 100% owned groups - but it's often possible to have some owned, and some partially owned, and to also have very little shifting in unowned pools. Overall, the architecture outlined here can be incredibly fast, but it does benefit greatly from good planning.

* EnTT's approach is starting to be implemented by other authors in other languages, with some small differences. For example, [Shipyard](https://github.com/leudz/shipyard).

* Archetype approaches are inherently faster for iterating over different groups of components on an ad-hoc basis. The tradeoffs here are in other areas- specifically a heavy runtime cost when components are added or removed from an entity, as well as fragmentation when there are lots and lots of mixtures of components overall, and the need to iterate over the archetypes one at a time (but each iteration gives you the entire set). These restrictions makes it problematic to use transient components, which are great for things like an event system, and can be a significant limitation when solving problems by creating new component types. However the benefit of fast ad-hoc iteration is wonderful and a archetypes are an excellent solution in many scenarios, such as empowering built-in hierarchical sorting in [Flecs](https://github.com/SanderMertens/flecs).

# References: 

* https://skypjack.github.io/2019-02-14-ecs-baf-part-1/ (and all the following parts)
* https://skypjack.github.io/pdf/ecs_back_and_forth.pdf and https://skypjack.github.io/pdf/the_cpp_of_entt.pdf (slides)
* https://imgur.com/a/quiJDB4 - add/remove explanation from the author of Shipyard
* https://github.com/SanderMertens/flecs/issues/3#issuecomment-521354387 (explanation from the author of flecs)


# Addendum

## Example - adding/removing entities

Code / live demo using JS: https://codesandbox.io/s/beautiful-blackburn-ckzhf?fontsize=14

   Add 3 entities:                                       
    destroyed is NONE                                    
    [(E0|V0), (E1|V0), (E2|V0)]                          
                                                         
   Remove E1                                             
    destroyed is 1                                       
    [(E0|V0), (INVALID|V1), (E2|V0)]                     
                                                         
   Add an entity: (E1|V1)                                
    destroyed is NONE                                    
    [(E0|V0), (E1|V1), (E2|V0)]                          
                                                         
   Remove: (E1|V1)                                       
    destroyed is 1                                       
    [(E0|V0), (INVALID|V2), (E2|V0)]                     
                                                         
   Remove E0                                             
    destroyed is 0                                       
    [(PTR1|V1), (INVALID|V2), (E2|V0)]                   
                                                         
   Remove E2                                             
    destroyed is 2                                       
    [(PTR1|V1), (INVALID|V2), (PTR0|V1)]                 
                                                         
   Add an entity: (E2|V1)                                
    destroyed is 0                                       
    [(PTR1|V1), (INVALID|V2), (E2|V1)]                   
                                                         
   Add an entity: (E0|V1)                                
    destroyed is 1                                       
    [(E0|V1), (INVALID|V2), (E2|V1)]                     
                                                         
   Add an entity: (E1|V2)                                
    destroyed is NONE                                    
    [(E0|V1), (E1|V2), (E2|V1)]                          
                                                         
   Add an entity: (E3|V0)                                
    destroyed is NONE                                    
    [(E0|V1), (E1|V2), (E2|V1), (E3|V0)]                 
