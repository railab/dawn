# Contributing to Dawn

We welcome and appreciate all contributions to the Dawn project! Whether you're
fixing bugs, adding new features, improving documentation, or suggesting
enhancements, your efforts help make Dawn a better tool for everyone.

## How to Contribute

There are several ways to contribute to Dawn:

* **Bug Reports:** If you find a bug, please open an issue on our GitHub
  repository.

* **Feature Requests:** Have an idea for a new feature? Open an issue to
  discuss it.

* **Documentation Improvements:** Found a typo or something unclear in
  the documentation? We'd love your help to make it better.

* **Code Contributions:** Submit pull requests for bug fixes, new features,
  or improvements.

## Coding Standards

The Dawn project adheres to the following coding standards:

* Language: Primarily C++. The porting layer, external libraries, and
  tests may use C.

* C++ Features: Only "light" C++ features are allowed. C++ exceptions
  are **not** permitted.

* Build System: CMake is the exclusive build system.

* C++ style: use custom style (based on Mozilla).
  Enforce with ``clang-format`` (configured in ``.clang-format``).
  Run ``tools/scripts/check-format.sh fix`` to auto-format all files.

* Line Length: Aim for 80-100 characters per line in both code and documentation
  for readability.

* Headers: use ``#pragma once`` instead of manual include guards.

* Memory Management: Dynamic allocations must be strictly limited to the
  initialization stage. No run-time dynamic allocation.

* Portability: Code and configuration should be generic. Port-specific code
  should be isolated within the `dawn/src/porting` and `dawn/include/porting`
  directories.

## Testing

* Unit tests are crucial for maintaining code quality. Please write tests for
  new features and bug fixes.
  
* Unit tests are required for new features; NTFC tests are highly recommended 
  for new features.
  
* ``dawnpy-tests`` is the main quality assurance tool for this project and all
  its steps must be completed successfully for the change to be considered as 
  meeting the standards requirements.

## Documentation

* Please keep the documentation up-to-date with your changes.

* Documentation is written in reStructuredText (`.rst`) and built using Sphinx.

### Building Documentation

To build and preview documentation:

```bash
cd Documentation
make html
# View the output in _build/html/index.html
```

## Pull Request Guidelines

Before submitting a pull request, please ensure:

1. Your code adheres to the [Coding Standards](#coding-standards).

2. New features or bug fixes are accompanied by appropriate unit tests.

3. Documentation is updated to reflect your changes, especially when adding
   new components (IOs, Programs, Protocols). See [Documentation](#documentation)
   for details.

4. If you added or modified C++ components (IOs, Programs, Protocols), the
   Python tooling (`tools/dawnpy/`) has been updated accordingly.

5. New upstream object classes are represented in Capabilities IO.

6. Your commit messages are clear and descriptive.
