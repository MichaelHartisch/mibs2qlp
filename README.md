# mibs2qlp

`mibs2qlp` is a small C++17 tool for transforming deterministic bilevel
instances in the [MibS file
format](https://coin-or.github.io/MibS/input.html) into instances in the
[QLP file
format](https://tm-vm-2.wiwi.uni-siegen.de/qlp-file-format.html).

The converter reads an MPS file together with its MibS auxiliary
(`.aux`) file. The `.aux` file identifies the lower-level variables and
constraints; all remaining variables and constraints are treated as
upper-level. The generated QLP uses existential variables for the upper
level (leader) and universal variables for the lower level (follower).

## Assumptions and scope

The transformation is intended for a restricted class of MibS instances.
In particular, it assumes that

-   the bilevel instance has **objective-function alignment -1**. The lower-level objective stored in the MibS `.aux` file is not used. 

-   the instance is supplied as a **plain-text MPS file** plus a **name-based MibS `.aux` file**. Compressed `.mps.gz` input and LP/GMPL input are not supported.

-   the `.aux` file uses the name-based MibS keywords `@NUMVARS`, `@NUMCONSTRS`, `@VARSBEGIN`/`@VARSEND`, and `@CONSTRSBEGIN`/`@CONSTRSEND`. The index-based MibS auxiliary format is not supported.

-   all lower-level variables listed in the `.aux` file are transformed into `ALL` variables and all remaining variables into `EXISTS` variables.

-   all upper-level variables precede all lower-level variables in the QLP `ORDER` section.

-   upper-level constraints are written below `SUBJECT TO`, while constraints identified as lower-level constraints in the `.aux` file are written below `UNCERTAINTY SUBJECT TO`.

- the model is **linear**. MPS `RANGES` are currently not supported.

-  if an MPS variable has no explicit bounds, standard MPS defaults are used: lower bound `0` and upper bound `+inf`. Note that this `+inf` might cause trouble when directly using it as input for a solver. However, it should be decided by the user, which upper bound is reasonable for such an instance.

-   if the MPS file contains no objective row, or the objective row has no coefficients, the generated QLP contains the zero objective:

        MINIMIZE
        0

-   if several `N` rows occur in the MPS file, the **first** one is treated as the upper-level objective row and all `N` rows are omitted from the constraint sections.

These restrictions are important: the program is a format/transformation
utility for the intended benchmark class, not a complete parser and
semantic converter for every model accepted by MibS.

## Build

A C++17 compiler and CMake are required. You can create the `mibs2qlp` executable as follows:

``` bash
cmake -B build
cd build
make
```


## Usage

``` bash
./mibs2qlp instance.mps instance.aux output.qlp
```
<!---
Optional arguments are:

``` text
--constraint-names
```

Keep the original MPS row names in the generated constraint sections.

``` text
--maximize
```

Write a `MAXIMIZE` objective and negate the MPS objective coefficients.

For example:

``` bash
./mibs2qlp 2AP05-10.mps 2AP05-10.aux 2AP05-10.qlp
```

or, with constraint names:

``` bash
./mibs2qlp 2AP05-10.mps 2AP05-10.aux 2AP05-10.qlp --constraint-names
```
---> 
## Acknowledgment

This tool was developed with the assistance of ChatGPT by OpenAI. ChatGPT was used to support the development process, including drafting and reviewing parts of the implementation and documentation. 
