#!/bin/bash
set -e
set -x

./build_script.sh 8

cd build
mkdir -p refactored_data

# FINE ERROR GRAINED

# DATA_NYX=/home/leonli/SDRBENCH/single_precision/SDRBENCH-EXASKY-NYX-512x512x512/temperature.f32
# # dummy run for metadata template
# ./test/test_refactor $DATA_NYX 3 32 3 512 512 512


# ./test/test_refactor_omp $DATA_NYX 3 32 3 512 512 512
# ./test/test_reconstructor_omp $DATA_NYX 0 11 478030.252417 239015.1262085 47803.0252417 23901.51262085 4780.30252417 2390.151262085 478.030252417 239.0151262085 47.8030252417 23.9015126208 4.7803025242 inf

# rm -rf refactored_data
# mkdir -p refactored_data


# DATA_ISABELLA=/home/leonli/SDRBENCH/single_precision/SDRBENCH-Hurricane-100x500x500/100x500x500/Pf48.bin.f32
# # dummy run for metadata template
# ./test/test_refactor $DATA_ISABELLA 3 32 3 500 500 100

# ./test/test_refactor_omp $DATA_ISABELLA 3 32 3 500 500 100
# ./test/test_reconstructor_omp $DATA_ISABELLA 0 11 341.1740723 170.58703615 34.11740723 17.058703615 3.411740723 1.7058703615 0.3411740723 0.17058703615 0.03411740723 0.017058703615 0.003411740723 inf

# rm -rf refactored_data
# mkdir -p refactored_data


# DATA_SCALE_LETKF=/home/leonli/SDRBENCH/single_precision/SDRBENCH-SCALE_98x1200x1200/PRES-98x1200x1200.f32
# # dummy run for metadata template
# ./test/test_refactor $DATA_SCALE_LETKF 3 32 3 1200 1200 98

# ./test/test_refactor_omp $DATA_SCALE_LETKF 3 32 3 1200 1200 98
# ./test/test_reconstructor_omp $DATA_SCALE_LETKF 0 11 10182.021875 5091.0109375 1018.2021875 509.10109375 101.82021875 50.910109375 10.182021875 5.0910109375 1.0182021875 0.50910109375 0.10182021875 inf


# rm -rf refactored_data
# mkdir -p refactored_data


# DATA_MIRANDA=/home/leonli/SDRBENCH/double_precision/SDRBENCH-Miranda-256x384x384/velocityz.d64
# # dummy run for metadata template
# ./test/test_refactor $DATA_MIRANDA 4 32 3 384 384 256

# ./test/test_refactor_omp $DATA_MIRANDA 2 32 3 384 384 256
# ./test/test_reconstructor_omp $DATA_MIRANDA 0 11 0.899611000000 0.449805500000 0.089961100000 0.044980550000 0.008996110000 0.004498055000 0.000899611000 0.000449805500 0.000089961100 0.000044980550 0.000008996110 inf


# COARSE ERROR GRAINED

DATA_NYX=/home/leonli/SDRBENCH/single_precision/SDRBENCH-EXASKY-NYX-512x512x512/temperature.f32
# dummy run for metadata template
./test/test_refactor $DATA_NYX 3 32 3 512 512 512


./test/test_refactor_omp $DATA_NYX 3 32 3 512 512 512
./test/test_reconstructor_omp $DATA_NYX 0 6 478030.252417 47803.0252417 4780.30252417 478.030252417 47.8030252417 4.7803025242 inf

rm -rf refactored_data
mkdir -p refactored_data


DATA_ISABELLA=/home/leonli/SDRBENCH/single_precision/SDRBENCH-Hurricane-100x500x500/100x500x500/Pf48.bin.f32
# dummy run for metadata template
./test/test_refactor $DATA_ISABELLA 3 32 3 500 500 100

./test/test_refactor_omp $DATA_ISABELLA 3 32 3 500 500 100
./test/test_reconstructor_omp $DATA_ISABELLA 0 6 341.1740723 34.11740723 3.411740723 0.3411740723 0.03411740723 0.003411740723 inf

rm -rf refactored_data
mkdir -p refactored_data


DATA_SCALE_LETKF=/home/leonli/SDRBENCH/single_precision/SDRBENCH-SCALE_98x1200x1200/PRES-98x1200x1200.f32
# dummy run for metadata template
./test/test_refactor $DATA_SCALE_LETKF 3 32 3 1200 1200 98

./test/test_refactor_omp $DATA_SCALE_LETKF 3 32 3 1200 1200 98
./test/test_reconstructor_omp $DATA_SCALE_LETKF 0 6 10182.021875 1018.2021875 101.82021875 10.182021875 1.0182021875 0.10182021875 inf


rm -rf refactored_data
mkdir -p refactored_data


DATA_MIRANDA=/home/leonli/SDRBENCH/double_precision/SDRBENCH-Miranda-256x384x384/velocityz.d64
# dummy run for metadata template
./test/test_refactor $DATA_MIRANDA 4 32 3 384 384 256

./test/test_refactor_omp $DATA_MIRANDA 2 32 3 384 384 256
./test/test_reconstructor_omp $DATA_MIRANDA 0 6 0.899611000000 0.089961100000 0.008996110000 0.000899611000 0.000089961100 0.000008996110 inf



