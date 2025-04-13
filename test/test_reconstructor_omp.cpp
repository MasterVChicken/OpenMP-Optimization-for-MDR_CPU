#include <iostream>
#include <ctime>
#include <cstdlib>
#include <vector>
#include <iomanip>
#include <cmath>
#include <bitset>
#include "utils.hpp"
#include "Refactor/Refactor.hpp"

#include <omp.h>

#define NUM_CORES 32
#define NUM_BLOCKS 32

using namespace std;

// Parallel refactor on blocks
template <class T, class Refactor>
void evaluate_refactor_parallel(const vector<T>& data, const vector<uint32_t>& dims, int target_level, int num_bitplanes, vector<Refactor>& refactors) {
    cout << "Start parallel refactoring with " << NUM_BLOCKS << " blocks." << endl;
    
    const size_t total_size = data.size();
    const uint32_t nz = dims[0], ny = dims[1], nx = dims[2];
    const size_t chunk_z = (nz + NUM_BLOCKS - 1) / NUM_BLOCKS;

    omp_set_num_threads(NUM_CORES);

    vector<double> blockTimes(NUM_BLOCKS, 0);
    
    struct timespec tstart, tend;
    
    clock_gettime(CLOCK_REALTIME, &tstart);
    #pragma omp parallel for num_threads(NUM_CORES)
    for (int i = 0; i < NUM_BLOCKS; ++i) {
        size_t z_start = i * chunk_z;
        size_t z_end = std::min(z_start + chunk_z, static_cast<size_t>(nz));
        if (z_start >= z_end) continue;

        size_t z_len = z_end - z_start;
        vector<uint32_t> local_dims = dims;
        local_dims[0] = static_cast<uint32_t>(z_len);

        size_t offset = z_start * ny * nx;
        size_t chunk_elements = z_len * ny * nx;
        if (offset + chunk_elements > data.size()) {
            std::cerr << "Chunk " << i << " out of bounds! offset = " << offset
                      << ", chunk = " << chunk_elements
                      << ", total = " << data.size() << std::endl;
            continue;
        }
        if (local_dims[0] < 2 || local_dims[1] < 2 || local_dims[2] < 2) {
            std::cerr << "Chunk " << i << " too small! dims = ["
                      << local_dims[0] << ", " << local_dims[1] << ", " << local_dims[2]
                      << "] — skipping\n";
            continue;
        }
        
        Refactor refactor = refactors[i];
        
        // Only record time for refactor function
        struct timespec start_call, end_call;
        clock_gettime(CLOCK_REALTIME, &start_call);
        refactor.refactor(&data[offset], local_dims, target_level, num_bitplanes);
        clock_gettime(CLOCK_REALTIME, &end_call);
        
        double elapsed = (double)(end_call.tv_sec - start_call.tv_sec) +
                         (double)(end_call.tv_nsec - start_call.tv_nsec) / 1e9;
        blockTimes[i] = elapsed;
    }
    struct timespec global_end;
    clock_gettime(CLOCK_REALTIME, &global_end);
    double overallTime = (double)(global_end.tv_sec - tstart.tv_sec) +
                         (double)(global_end.tv_nsec - tstart.tv_nsec) / 1e9;

    for (int i = 0; i < NUM_BLOCKS; ++i) {
        cout << "Block " << i << " refactor call time: " << blockTimes[i] << " s" << endl;
    }
    cout << "Overall parallel refactor time (including partition overhead): " << overallTime << " s" << endl;
}

template <class T, class Decomposer, class Interleaver, class Encoder, class Compressor, class ErrorCollector, class Writer>
void test(string filename, const vector<uint32_t>& dims, int target_level, int num_bitplanes,
          Decomposer decomposer, Interleaver interleaver, Encoder encoder,
          Compressor compressor, ErrorCollector collector, Writer writer) {
    size_t num_elements = 0;
    auto data = MGARD::readfile<T>(filename.c_str(), num_elements);

    vector<MDR::ComposedRefactor<T, Decomposer, Interleaver, Encoder, Compressor, ErrorCollector, Writer>> refactors;
    for (int i = 0; i < NUM_BLOCKS; ++i) {
        string meta_file = "refactored_data/metadata_" + to_string(i) + ".bin";
        vector<string> level_files;
        for (int l = 0; l <= target_level; ++l) {
            level_files.push_back("refactored_data/level_" + to_string(l) + "_" + to_string(i) + ".bin");
        }
        auto block_writer = MDR::ConcatLevelFileWriter(meta_file, level_files);
        refactors.emplace_back(decomposer, interleaver, encoder, compressor, collector, block_writer);
    }

    evaluate_refactor_parallel(data, dims, target_level, num_bitplanes, refactors);
}

int main(int argc, char ** argv) {
    omp_set_num_threads(NUM_CORES);

    int argv_id = 1;
    string filename = string(argv[argv_id++]);
    int target_level = atoi(argv[argv_id++]);
    int num_bitplanes = atoi(argv[argv_id++]);
    if(num_bitplanes % 2 == 1) {
        num_bitplanes += 1;
        cout << "Change to " << num_bitplanes + 1 << " bitplanes for simplicity of negabinary encoding" << endl;
    }
    int num_dims = atoi(argv[argv_id++]);
    vector<uint32_t> dims(num_dims, 0);
    for (int i = 0; i < num_dims; i++) {
        dims[i] = atoi(argv[argv_id++]);
    }

    string metadata_file = "refactored_data/metadata.bin";
    vector<string> files;
    for (int i = 0; i <= target_level; i++) {
        string fname = "refactored_data/level_" + to_string(i) + ".bin";
        files.push_back(fname);
    }

    using T = float;
    using T_stream = uint32_t;
    if(num_bitplanes > 32) {
        num_bitplanes = 32;
        cout << "Only less than 32 bitplanes are supported for single-precision floating point" << endl;
    }
    auto decomposer = MDR::MGARDOrthoganalDecomposer<T>();
    auto interleaver = MDR::DirectInterleaver<T>();
    auto encoder = MDR::NegaBinaryBPEncoder<T, T_stream>();
    auto compressor = MDR::AdaptiveLevelCompressor(32);
    auto collector = MDR::SquaredErrorCollector<T>();
    auto writer = MDR::ConcatLevelFileWriter(metadata_file, files);

    test<T>(filename, dims, target_level, num_bitplanes, decomposer, interleaver, encoder,
            compressor, collector, writer);
    return 0;
}
