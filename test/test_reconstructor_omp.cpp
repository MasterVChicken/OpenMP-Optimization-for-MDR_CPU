#include <bitset>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <vector>
#include <fstream>
#include <omp.h>

#include "utils.hpp"

#include "Reconstructor/Reconstructor.hpp"

#define NUM_CORES 16
#define NUM_BLOCKS 16

using namespace std;

template <class T, class Reconstructor>
void evaluate_reconstructor_parallel(const vector<T> &data,
                                     const vector<double> &tolerance,
                                     vector<Reconstructor> &reconstructors) {
  struct timespec start, end;
  std::vector<size_t> total_size(tolerance.size(), 0);

  std::ofstream outfile("retrieved_size.txt", std::ios::app);
  if (!outfile.is_open()) {
    std::cerr << "[ERROR] Failed to open retrieved_size.txt for writing." << std::endl;
    return;
  }

  outfile << "==== Results ====" << std::endl;

  for (int j = 0; j < tolerance.size(); j++) {
    std::vector<size_t> thread_local_sizes(NUM_BLOCKS, 0);

    clock_gettime(CLOCK_REALTIME, &start);
    #pragma omp parallel for num_threads(NUM_CORES)
    for (int i = 0; i < NUM_BLOCKS; i++) {
      auto reconstructed_data =
          reconstructors[i].progressive_reconstruct(tolerance[j], -1);
      auto size_vec = reconstructors[i].getLastRetrieveSizes();

      for (auto size : size_vec) {
        thread_local_sizes[i] += size;
      }
    }
    clock_gettime(CLOCK_REALTIME, &end);
    double elapsed =
      (double)(end.tv_sec - start.tv_sec) +
      (double)(end.tv_nsec - start.tv_nsec) / 1e9;

    for (int i = 0; i < NUM_BLOCKS; i++) {
      total_size[j] += thread_local_sizes[i];
    }

    outfile << "tolerance " << tolerance[j]
            << " -> retrieved size = " << total_size[j] << " bytes -> retrieved time = " << elapsed << std::endl;
  }

  outfile << std::endl;
  outfile.close();
}



template <class T, class Decomposer, class Interleaver, class Encoder,
          class Compressor, class ErrorEstimator, class SizeInterpreter,
          class Retriever>
void test(string filename, const vector<double> &tolerance,
          Decomposer decomposer, Interleaver interleaver, Encoder encoder,
          Compressor compressor, ErrorEstimator estimator,
          SizeInterpreter interpreter, Retriever retriever) {

  std::vector<MDR::ComposedReconstructor<T, Decomposer, Interleaver, Encoder,
                                         Compressor, SizeInterpreter,
                                         ErrorEstimator, Retriever>>
      reconstructors;

  for (int i = 0; i < NUM_BLOCKS; ++i) {
    string metadata_file = "refactored_data/metadata_" + to_string(i) + ".bin";
    int num_levels = 0;
    int num_dims = 0;

    size_t num_bytes = 0;
    auto metadata = MGARD::readfile<uint8_t>(metadata_file.c_str(), num_bytes);
    
    num_dims = metadata[0];
    num_levels = metadata[num_dims * sizeof(uint32_t) + 1];
    // assert(num_bytes > num_dims * sizeof(uint32_t) + 2);

    vector<string> files;
    for (int j = 0; j < num_levels; j++) {
      string filename =
          "refactored_data/level_" + to_string(j) + "_" + to_string(i) + ".bin";
      files.push_back(filename);
    }

    auto new_retriever = MDR::ConcatLevelFileRetriever(metadata_file, files);

    auto reconstructor =
      MDR::ComposedReconstructor<T, Decomposer, Interleaver, Encoder,
                                 Compressor, SizeInterpreter, ErrorEstimator,
                                 Retriever>(decomposer, interleaver, encoder,
                                            compressor, interpreter, new_retriever);

    reconstructors.emplace_back(decomposer, interleaver, encoder, compressor,
                                interpreter, new_retriever);
              
    reconstructors.back().load_metadata();
  }

  size_t num_elements = 0;
  auto data = MGARD::readfile<T>(filename.c_str(), num_elements);
  evaluate_reconstructor_parallel(data, tolerance, reconstructors);
}

int main(int argc, char **argv) {

  int argv_id = 1;
  string filename = string(argv[argv_id++]);
  int error_mode = atoi(argv[argv_id++]);
  int num_tolerance = atoi(argv[argv_id++]);
  vector<double> tolerance(num_tolerance, 0);
  for (int i = 0; i < num_tolerance; i++) {
    tolerance[i] = atof(argv[argv_id++]);
  }
  double s = atof(argv[argv_id++]);

  string metadata_file = "refactored_data/metadata.bin";
  int num_levels = 0;
  int num_dims = 0;
  {
    // metadata interpreter, otherwise information needs to be provided
    size_t num_bytes = 0;
    auto metadata = MGARD::readfile<uint8_t>(metadata_file.c_str(), num_bytes);
    assert(num_bytes > num_dims * sizeof(uint32_t) + 2);
    num_dims = metadata[0];
    num_levels = metadata[num_dims * sizeof(uint32_t) + 1];
    // cout << "number of dimension = " << num_dims
    //      << ", number of levels = " << num_levels << endl;
  }
  vector<string> files;
  for (int i = 0; i < num_levels; i++) {
    string filename = "refactored_data/level_" + to_string(i) + ".bin";
    files.push_back(filename);
  }

  using T = float;
  using T_stream = uint32_t;
  auto decomposer = MDR::MGARDOrthoganalDecomposer<T>();
  // auto decomposer = MDR::MGARDHierarchicalDecomposer<T>();
  auto interleaver = MDR::DirectInterleaver<T>();
  // auto interleaver = MDR::SFCInterleaver<T>();
  // auto interleaver = MDR::BlockedInterleaver<T>();
  // auto encoder = MDR::GroupedBPEncoder<T, T_stream>();
  auto encoder = MDR::NegaBinaryBPEncoder<T, T_stream>();
  // auto encoder = MDR::PerBitBPEncoder<T, T_stream>();
  // auto compressor = MDR::DefaultLevelCompressor();
  auto compressor = MDR::AdaptiveLevelCompressor(32);
  // auto compressor = MDR::NullLevelCompressor();
  auto retriever = MDR::ConcatLevelFileRetriever(metadata_file, files);
  switch (error_mode) {
  case 1: {
    auto estimator = MDR::SNormErrorEstimator<T>(num_dims, num_levels - 1, s);
    // auto interpreter =
    // MDR::SignExcludeGreedyBasedSizeInterpreter<MDR::SNormErrorEstimator<T>>(estimator);
    auto interpreter =
        MDR::NegaBinaryGreedyBasedSizeInterpreter<MDR::SNormErrorEstimator<T>>(
            estimator);
    // auto interpreter =
    // MDR::RoundRobinSizeInterpreter<MDR::SNormErrorEstimator<T>>(estimator);
    // auto interpreter =
    // MDR::InorderSizeInterpreter<MDR::SNormErrorEstimator<T>>(estimator); auto
    // estimator = MDR::L2ErrorEstimator_HB<T>(num_dims, num_levels - 1); auto
    // interpreter =
    // MDR::SignExcludeGreedyBasedSizeInterpreter<MDR::L2ErrorEstimator_HB<T>>(estimator);
    test<T>(filename, tolerance, decomposer, interleaver, encoder, compressor,
            estimator, interpreter, retriever);
    break;
  }
  default: {
    auto estimator = MDR::MaxErrorEstimatorOB<T>(num_dims);
    auto interpreter =
        MDR::SignExcludeGreedyBasedSizeInterpreter<MDR::MaxErrorEstimatorOB<T>>(
            estimator);
    // auto interpreter =
    // MDR::RoundRobinSizeInterpreter<MDR::MaxErrorEstimatorOB<T>>(estimator);
    // auto interpreter =
    // MDR::InorderSizeInterpreter<MDR::MaxErrorEstimatorOB<T>>(estimator); auto
    // estimator = MDR::MaxErrorEstimatorHB<T>(); auto interpreter =
    // MDR::SignExcludeGreedyBasedSizeInterpreter<MDR::MaxErrorEstimatorHB<T>>(estimator);
    test<T>(filename, tolerance, decomposer, interleaver, encoder, compressor,
            estimator, interpreter, retriever);
  }
  }
  return 0;
}