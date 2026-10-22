---
name: entt-testing
description: >-
  Use when configuring, building, selecting, or interpreting tests for the EnTT
  repository, including subsystem regression tests and CTest.
---

# EnTT testing

Use this skill for validation work in the EnTT repository. Tests are optional
in the default CMake configuration and are enabled with `ENTT_BUILD_TESTING`.

## Find the relevant coverage

- Tests are under `test/entt/`, generally grouped to match `src/entt/`
  subsystems.
- Start with the narrowest existing test target covering the changed public
  behavior, then expand validation when practical.
- Read neighboring tests to match fixture patterns and to understand existing
  guarantees. Prefer a regression case that would fail before the fix.
- Check `.github/workflows/` when a change affects compiler versions,
  configured C++ standards, sanitizers, ID types, or platform-specific
  behavior.

## Configure and run

The repository README documents a CMake test build. From a build directory,
the basic flow is:

```sh
cmake -DENTT_BUILD_TESTING=ON ..
cmake --build .
ctest --output-on-failure
```

The CI also configures optional library and example targets and runs CTest
from the build directory. Follow the current workflow for broader validation;
don't assume one configuration covers every supported compiler or standard.

For a focused run, use CTest's `-R` pattern after listing available tests
with `ctest -N`. Build the corresponding target first if the generator does
not build it as part of the default target.

## Validation discipline

- Do not run a full build or test suite unless requested or needed to satisfy
  the task. If checks are run, state the exact configure/build/test commands
  and their outcomes.
- When checks cannot be run, explain the concrete limitation and identify the
  validation still needed.
- Distinguish compile failures, assertion failures, and environment/toolchain
  failures; include the useful diagnostic rather than reporting only that
  tests failed.
- Avoid weakening or deleting existing assertions to make a change pass.
  Update expectations only when the intended public behavior has changed and
  the change is justified.
