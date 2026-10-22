---
name: entt-development
description: >-
  Use when exploring, changing, reviewing, or documenting the EnTT C++ library
  repository. Covers repository layout, implementation conventions, subsystem
  navigation, and safe change workflow.
---

# EnTT development

Use this skill when making changes in the EnTT repository. EnTT is a
header-only modern C++ library; public headers, templates, compile-time
behavior, and source compatibility are central concerns.

## Orient in the repository

- `src/entt/` contains the library headers, grouped by subsystem: `entity`,
  `container`, `core`, `signal`, `meta`, `resource`, `graph`, `poly`,
  `process`, `locator`, `config`, `stl`, and `tools`.
- `test/entt/` mirrors most library subsystems. Find the closest existing test
  before changing behavior.
- `single_include/entt/entt.hpp` is the amalgamated distribution. Check the
  repository's generation scripts and established workflow before editing
  generated output by hand.
- `docs/` contains documentation sources; `README.md` and `CONTRIBUTING.md`
  describe project usage and contribution expectations.
- Build and packaging integrations live in `CMakeLists.txt`, `bazel/`,
  `conan/`, and `cmake/`.

## Change workflow

1. Read the implementation and tests for the affected subsystem, plus nearby
   documentation and callers.
2. Keep changes focused and consistent with existing public API naming,
   template patterns, and formatting. Avoid unrelated refactors.
3. Preserve EnTT's header-only and pay-for-what-you-use design. Avoid adding
   runtime or compile-time cost to users who do not use a feature.
4. Treat entity identifiers, storage ordering, iterator invalidation,
   signal/dispatcher ordering, and customization points as observable
   behavior. Verify assumptions in code and tests rather than relying on
   general ECS conventions.
5. Add or update regression coverage for behavior changes when asked to
   implement a fix or feature. Keep documentation aligned with public API
   changes.
6. If a public header changes, determine whether generated/amalgamated
   distributions or packaging metadata also need updating by following
   existing repository scripts and CI workflows.
7. Report what changed and what validation was performed. Do not claim unrun
   checks passed.

## C++ and API guidance

- Follow `.clang-format` and the local style in the touched files; avoid
  introducing a new style.
- Prefer compatibility with the project's configured C++ standards. Inspect
  `ENTT_CXX_STD` and the CI matrix before using newer language features.
- Keep templates generic and respect the library's customization points,
  allocator support, and user-provided STL compatibility where relevant.
- Be explicit about ownership, lifetime, invalidation, and exception behavior
  when changing interfaces.
- Preserve documented behavior unless the request explicitly changes it. When
  behavior is unclear, search tests and docs first.

## Useful navigation

- Search symbols and call sites with `rg` before editing.
- Use the subsystem directory names above to locate implementation and test
  counterparts.
- `CONTRIBUTING.md` asks bug reports to include reproduction details and
  environment/version, and recommends regression tests for fixes. New feature
  proposals are expected to be discussed before implementation.
