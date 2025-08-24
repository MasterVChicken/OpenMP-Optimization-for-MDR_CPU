This repository is a fork of [Multiprecision-data-refactoring](https://github.com/lxAltria/Multiprecision-data-refactoring.git), 
adapted to support progressive hybrid compression for the artifact of the SC'25 paper:  
**"HP-MDR: High-performance and Portable Data Refactoring and Progressive Retrieval with Advanced GPUs"**

To test the framework, you will need to change the following path to your own project path in /test/CMakeList
```
set(EVA_INCLUDES "/home/leonli/OpenMP-Optimization-for-MDR_CPU/external/compression_utils/include")
```
Then you can build and run the project by 
```
./build_script.sh 8
./run_script.sh
```
You will find results under /build : refactor_time.txt and retrieved_size.txt .

**How to read results**

*refactor_time.txt* shows the total refactor time for each dataset listed in run_script.sh with each result seperated by "==== Refactor time ====".

The refactor times are printed in the order specified in the run_script. Dataset names were not included in the output; apologies for the omission.






*retrieved_size.txt* follows the same sequence as *refactor_time.txt* with each result seperated by "==== Results ====".

For each dataset, we report performance under multiple tolerance levels. Each row includes:

- the tolerance value (representing the desired reconstruction accuracy),
- the retrieved size (in bytes) (incremental),
- and the retrieval time (in seconds).
The primary focus of our analysis is on the retrieved size and retrieval time, as they reflect how many bit and time taken to reconstruct to current tolerance.


### Note on Floating-point Precision
**Important**: Our current implementation assumes single-precision (float) by default.

If you are testing with a **double-precision** dataset, please manually modify the source files:

- In `test/test_refactor_omp.cpp`, change  
  ```cpp
  using T = float;
  ```
  to
  ```cpp
  using T = double;
  ```
- In `test/test_reconstructor_omp.cpp`, do the same replacement.


