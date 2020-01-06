# wrLib

Whimsical Raps libraries optimized for embedded processors.

Focus on minimal RAM usage, and block processing for DSP.

A test suite (incomplete) is provided for ensuring equivalence of the block processing versions of DSP functions.

## Layout

| Makefile
| README.md
| math/
    | Previously 'wrLib'
| dsp/
    | Signal processing library
| test/
    | math/
        | Tests for math library
    | dsp/
        | Tests for DSP library
| unity/
    | The 'unity' testing library

## Tests

Run all tests with `make` from the root directory.

## TODO

* Greater test coverage for block-processing equivalence
* Profiling tests for block-processing functions
* Add tests for non-signal based functions
* Consistancy of style should be improved

## Contributing

* 4-space indentation!
