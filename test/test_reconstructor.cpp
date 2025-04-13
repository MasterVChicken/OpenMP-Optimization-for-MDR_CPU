#include <iostream>
#include <ctime>
#include <cstdlib>
#include <vector>
#include <iomanip>
#include <cmath>
#include <bitset>
#include "utils.hpp"
#include "Reconstructor/Reconstructor.hpp"

using namespace std;

template <class T, class Reconstructor>
void evaluate(const vector<T>& data, const vector<double>& tolerances, Reconstructor reconstructor) {
    struct timespec start, end;
    int err = 0;
    
    vector<double> aggTimes(tolerances.size(), 0);
    vector<uint64_t> aggSizes(tolerances.size(), 0);

    for (size_t i = 0; i < tolerances.size(); i++) {
        cout << "Start reconstruction for tolerance: " << tolerances[i] << endl;
        err = clock_gettime(CLOCK_REALTIME, &start);
        auto reconstructed_data = reconstructor.progressive_reconstruct(tolerances[i], -1);
        err = clock_gettime(CLOCK_REALTIME, &end);
        double elapsed = (double)(end.tv_sec - start.tv_sec) +
                         (double)(end.tv_nsec - start.tv_nsec) / 1e9;
        aggTimes[i] = elapsed;
        cout << "Reconstruct time for tolerance " << tolerances[i] << ": " << elapsed << " s" << endl;
        
        const auto &lastRetrieveSizes = reconstructor.getLastRetrieveSizes();
        uint64_t totalSize = 0;
        for (const auto &size : lastRetrieveSizes) {
            totalSize += size;
        }
        aggSizes[i] = totalSize;
    }
    
    cout << "========================================" << endl;
    cout << "Aggregate Results:" << endl;
    for (size_t i = 0; i < tolerances.size(); i++) {
        cout << "Tolerance " << tolerances[i]
             << " | Global Reconstruction Time: " << aggTimes[i] << " s, "
             << "Global Retrieval Size: " << aggSizes[i] << " bytes" << endl;
    }
}

template <class T, class Decomposer, class Interleaver, class Encoder,
          class Compressor, class ErrorEstimator, class SizeInterpreter,
          class Retriever>
void test(string filename, const vector<double>& tolerances,
          Decomposer decomposer, Interleaver interleaver, Encoder encoder,
          Compressor compressor, ErrorEstimator estimator,
          SizeInterpreter interpreter, Retriever retriever) {
    
    size_t num_elements = 0;
    auto data = MGARD::readfile<T>(filename.c_str(), num_elements);
    
    vector<double> aggTimes(tolerances.size(), 0);
    vector<uint64_t> aggSizes(tolerances.size(), 0);
    
    for (size_t i = 0; i < tolerances.size(); i++) {
        MDR::ComposedReconstructor<T, Decomposer, Interleaver, Encoder, Compressor,
                                   SizeInterpreter, ErrorEstimator, Retriever>
            reconstructor(decomposer, interleaver, encoder, compressor, interpreter, retriever);
        cout << "Loading metadata for tolerance " << tolerances[i] << endl;
        reconstructor.load_metadata();
        
        struct timespec start, end;
        int err = clock_gettime(CLOCK_REALTIME, &start);
        auto reconstructed_data = reconstructor.progressive_reconstruct(tolerances[i], -1);
        err = clock_gettime(CLOCK_REALTIME, &end);
        double elapsed = (double)(end.tv_sec - start.tv_sec) +
                         (double)(end.tv_nsec - start.tv_nsec) / 1e9;
        aggTimes[i] = elapsed;
        cout << "Reconstruct time for tolerance " << tolerances[i] << ": " << elapsed << " s" << endl;
        
        const auto &lastRetrieveSizes = reconstructor.getLastRetrieveSizes();
        uint64_t totalSize = 0;
        for (const auto &size : lastRetrieveSizes) {
            totalSize += size;
        }
        aggSizes[i] = totalSize;
    }
    
    cout << "========================================" << endl;
    cout << "Aggregate Results:" << endl;
    for (size_t i = 0; i < tolerances.size(); i++) {
        cout << "Tolerance " << tolerances[i]
             << " | Global Reconstruction Time: " << aggTimes[i] << " s, "
             << "Global Retrieval Size: " << aggSizes[i] << " bytes" << endl;
    }
}

int main(int argc, char ** argv) {
    int argv_id = 1;
    string filename = string(argv[argv_id++]);
    int error_mode = atoi(argv[argv_id++]);
    int num_tolerance = atoi(argv[argv_id++]);
    vector<double> tolerances(num_tolerance, 0);
    for (int i = 0; i < num_tolerance; i++) {
        tolerances[i] = atof(argv[argv_id++]);
    }
    double s = atof(argv[argv_id++]);

    string metadata_file = "refactored_data/metadata.bin";
    int num_levels = 0;
    int num_dims = 0;
    {
        size_t num_bytes = 0;
        auto metadata = MGARD::readfile<uint8_t>(metadata_file.c_str(), num_bytes);
        
        assert(num_bytes > num_dims * sizeof(uint32_t) + 2);
        num_dims = metadata[0];
        num_levels = metadata[num_dims * sizeof(uint32_t) + 1];
        cout << "Number of dimensions = " << num_dims 
             << ", Number of levels = " << num_levels << endl;
    }
    
    vector<string> files;
    for (int i = 0; i < num_levels; i++) {
        string fname = "refactored_data/level_" + to_string(i) + ".bin";
        files.push_back(fname);
    }

    using T = float;
    using T_stream = uint32_t;
    auto decomposer = MDR::MGARDOrthoganalDecomposer<T>();
    auto interleaver = MDR::DirectInterleaver<T>();
    auto encoder = MDR::NegaBinaryBPEncoder<T, T_stream>();
    auto compressor = MDR::AdaptiveLevelCompressor(32);
    auto retriever = MDR::ConcatLevelFileRetriever(metadata_file, files);
    
    switch(error_mode) {
        case 1: {
            auto estimator = MDR::SNormErrorEstimator<T>(num_dims, num_levels - 1, s);
            auto interpreter = MDR::NegaBinaryGreedyBasedSizeInterpreter<MDR::SNormErrorEstimator<T>>(estimator);
            test<T>(filename, tolerances, decomposer, interleaver, encoder, compressor,
                    estimator, interpreter, retriever);
            break;
        }
        default: {
            auto estimator = MDR::MaxErrorEstimatorOB<T>(num_dims);
            auto interpreter = MDR::SignExcludeGreedyBasedSizeInterpreter<MDR::MaxErrorEstimatorOB<T>>(estimator);
            test<T>(filename, tolerances, decomposer, interleaver, encoder, compressor,
                    estimator, interpreter, retriever);
        }
    }
    return 0;
}
