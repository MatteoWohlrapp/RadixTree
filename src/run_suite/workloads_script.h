#pragma once

#include <string>
#include "spdlog/spdlog.h"
#include <chrono>
#include <random>
#include <set>
#include "../data/data_manager.h"
#include "../utils/time.h"
#include <sys/resource.h>
#include <iostream>

/**
 * @brief General abstraction for the YCSB workload
 */
class WorkloadScript
{
private:
    uint64_t total_records = 1250000;
    uint64_t buffer_size = 4000;
    uint64_t radix_tree_size = 69793218;
    uint64_t operation_count = 2000000;
    uint64_t record_count = 1000000;
    int max_scan_range = 100;

    std::shared_ptr<spdlog::logger> logger;
    DataManager<Configuration::page_size> data_manager;
    std::vector<std::vector<double>> times;
    std::set<int64_t> records_set;
    std::vector<int64_t> records_vector; /// entries inserted into the DB
    std::vector<int> indice_vector;      /// indices in the records vector that are addressed
    std::random_device rd;
    std::function<uint64_t()> index_distribution;
    std::uniform_int_distribution<int64_t> value_distribution;
    std::mt19937 generator;
    int insert_index = 0; /// offset at the end of records that specifies where current insert operations draw elements from
    std::string results_filename;
    std::ofstream csv_file;

    int threads[5] = {1, 5, 10, 20, 30};
    double workloads[5][5] = {
        {0, 0.5, 0.5, 0, 0}, {0, 0.95, 0.05, 0, 0}, {0, 1, 0, 0, 0}, {0.05, 0, 0, 0.95, 0}, {0, 0.90, 0, 0, 0.1}};

    /**
     * @brief enumeration for the different kinds of operation possible
     */
    enum Operation
    {
        INSERT,
        READ,
        UPDATE,
        SCAN,
        DELETE,
        NUM_OPERATIONS
    };
    std::vector<Operation> operations_vector; /// vector with operations that are executed

    /**
     * @brief Abstraction to perform an operation on the database
     * @param op The operation
     * @param index The index of the key in the indices array
     */
    void perform_operation(Operation op, int index)
    {
        switch (op)
        {
        case INSERT:
            data_manager.insert(records_vector[insert_index], records_vector[insert_index]);
            insert_index++;
            break;
        case READ:
        {
            data_manager.get_value(records_vector[indice_vector[index]]);
        }
        break;
        case UPDATE:
            data_manager.update(records_vector[indice_vector[index]], records_vector[indice_vector[index]] + 1);
            break;
        case SCAN:
            data_manager.scan(records_vector[indice_vector[index]], max_scan_range);
            break;
        case DELETE:
            data_manager.delete_value(records_vector[indice_vector[index]]);
            break;
        case NUM_OPERATIONS:
            break;
        }
    }

    /**
     * @brief Runs a workload
     * @param test_name The name of the test
     * @param iteration The current iteration of the test
     * @param buffer_size_arg The amounts of pages in the buffer manager
     * @param record_count_arg The number of records in the trees
     * @param operation_count_arg The number of performed operations
     * @param distribution_arg The distribution
     * @param coefficient_arg The coefficient for the distribution
     * @param insert_proportion_arg The proportion of inserts
     * @param read_proportion_arg The proportion of reads
     * @param update_proportion_arg The proportion of updates
     * @param scan_proportion_arg The proportion of scancs
     * @param delete_proportion_arg The proportion of deletes
     * @param cache_arg If the cache is activated or not
     * @param radix_tree_size_arg The size of the cache
     * @param workload_arg The workload that is run
     * @param inverse Specifies if the elements are inserted from the front or the back of the array
     * @param threads The number of threads this is run for
     */
    void run_workload(std::string test_name, int iteration, uint64_t buffer_size_arg, uint64_t record_count_arg, uint64_t operation_count_arg, std::string distribution_arg, double coefficient_arg, double insert_proportion_arg, double read_proportion_arg, double update_proportion_arg, double scan_proportion_arg, double delete_proportion_arg, bool cache_arg, uint64_t radix_tree_size_arg, int workload_arg, bool inverse = false, int threads = 0)
    {
        std::cout << "Starting iteration " << iteration << " of test " << test_name << std::endl
                  << std::flush;

        indice_vector.clear();
        indice_vector.resize(operation_count_arg);

        operations_vector.clear();
        operations_vector.resize(operation_count_arg);

        times = std::vector<std::vector<double>>(NUM_OPERATIONS, std::vector<double>(0, 0.0));

        data_manager = DataManager<Configuration::page_size>(buffer_size_arg, cache_arg, radix_tree_size_arg);

        if (distribution_arg == "uniform")
        {
            std::uniform_int_distribution<uint64_t> dist(0, record_count_arg - 1);

            // Store the lambda function
            index_distribution = [this, dist]() mutable -> uint64_t
            { return dist(rd); };
        }
        else if (distribution_arg == "geometric")
        {
            std::geometric_distribution<uint64_t> dist(coefficient_arg);

            index_distribution = [this, dist, record_count_arg]() mutable -> uint64_t
            {
                uint64_t num = dist(generator);
                if (num >= record_count_arg)
                    num = record_count_arg - 1;
                return num;
            };
        }

        std::vector<double> weights = {insert_proportion_arg, read_proportion_arg, update_proportion_arg, scan_proportion_arg, delete_proportion_arg};

        // Use for discrete distribution
        std::discrete_distribution<> op_dist(weights.begin(), weights.end());

        for (uint64_t i = 0; i < operation_count_arg; i++)
        {
            Operation op = static_cast<Operation>(op_dist(rd));
            operations_vector[i] = op;

            int index = index_distribution();
            indice_vector[i] = index;
        }
        // Inserting all elements
        if (!inverse)
        {
            for (uint64_t i = 0; i < record_count_arg; i++)
            {
                data_manager.insert(records_vector[i], records_vector[i]);
            }
        }
        else
        {
            for (uint64_t i = 0; i < record_count_arg; i++)
            {
                data_manager.insert(records_vector[record_count_arg - i - 1], records_vector[record_count_arg - i - 1]);
            }
        }

        insert_index = record_count_arg;

        std::chrono::time_point<std::chrono::high_resolution_clock> total_start, total_end;
        total_start = std::chrono::high_resolution_clock::now();
    #pragma omp parallel for num_threads(threads)
        for (uint64_t i = 0; i < operation_count; i++)
        {
            perform_operation(operations_vector[i], i);
        }
        total_end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> total_elapsed = total_end - total_start;
        times[0].push_back(total_elapsed.count());
        

        uint64_t cache_size = data_manager.get_cache_size();
        uint64_t current_buffer_size = data_manager.get_current_buffer_size();

        analyze(test_name, iteration, buffer_size_arg, record_count_arg, operation_count_arg, distribution_arg, coefficient_arg, insert_proportion_arg, read_proportion_arg, update_proportion_arg, scan_proportion_arg, delete_proportion_arg, cache_arg, radix_tree_size_arg, cache_size, current_buffer_size, workload_arg, threads);

        data_manager.destroy();
    }
    /**
     * @brief Initializes the vectors for records, operations and indices
     */
    void initialize()
    {
        while (records_set.size() < total_records)
        {
            // Generate uniform random number and insert into set
            records_set.insert(value_distribution(generator));
        }

        // Convert set to vector
        std::copy(records_set.begin(), records_set.end(), std::back_inserter(records_vector));

        records_set.clear();
    }

    struct OperationResult
    {
        uint64_t operation_count;
        double total_time;
        double mean_time;
        double median_time;
        double percentile_90;
        double percentile_95;
        double percentile_99;
    };

    /**
     * @brief Analyzes the result of the workload
     * @param test_name The name of the test
     * @param iteration The current iteration of the test
     * @param buffer_size_arg The amounts of pages in the buffer manager
     * @param record_count_arg The number of records in the trees
     * @param operation_count_arg The number of performed operations
     * @param distribution_arg The distribution
     * @param coefficient_arg The coefficient for the distribution
     * @param insert_proportion_arg The proportion of inserts
     * @param read_proportion_arg The proportion of reads
     * @param update_proportion_arg The proportion of updates
     * @param scan_proportion_arg The proportion of scancs
     * @param delete_proportion_arg The proportion of deletes
     * @param cache_arg If the cache is activated or not
     * @param radix_tree_size_arg The size of the cache
     * @param cache_size_arg The actual size of the cache
     * @param current_buffer_size_arg The size of the buffer
     * @param workload_arg The workload that is run
     * @param threads The number of threads
     */
    void analyze(std::string test_name, int iteration, uint64_t buffer_size_arg, uint64_t record_count_arg, uint64_t operation_count_arg, std::string distribution_arg, double coefficient_arg, double insert_proportion_arg, double read_proportion_arg, double update_proportion_arg, double scan_proportion_arg, double delete_proportion_arg, bool cache_arg, uint64_t radix_tree_size_arg, uint64_t cache_size_arg, uint64_t current_buffer_size_arg, int workload_arg, int threads)
    {
        csv_file.open(results_filename, std::ios_base::app);
        csv_file << test_name << "," << iteration << "," << buffer_size_arg << "," << record_count_arg << "," << operation_count_arg << ","
                 << distribution_arg << "," << std::fixed << std::setprecision(5) << coefficient_arg << "," << (cache_arg ? "true" : "false") << radix_tree_size_arg << "," << workload_arg << ",";
        csv_file << cache_size_arg << "," << current_buffer_size_arg << "," << std::fixed << std::setprecision(2) << times[0][0] << ","
                 << operation_count_arg / times[0][0] << "," << threads <<"\n";
        csv_file.close();
    }

public:
    /**
     * @brief Constructor for the workload script
     */
    WorkloadScript() : data_manager(1, false, 1)
    {
        logger = spdlog::get("logger");
        value_distribution = std::uniform_int_distribution<int64_t>(INT64_MIN + 1, INT64_MAX - 1);
        generator = std::mt19937(42);
        times = std::vector<std::vector<double>>(NUM_OPERATIONS, std::vector<double>(0, 0.0));
        data_manager.destroy();
        std::string prefix = Time::getDateTime();
        results_filename = "../results/" + prefix + "test_results.csv";
        csv_file.open(results_filename, std::ios_base::app);
        csv_file << "TestName,Iteration,BufferSize,RecordCount,OperationCount,Distribution,Coefficient,Cache,RadixTreeSize,Workload,CacheSize,CurrentBufferSize,TotalTime,Throughput,Threads\n";
        csv_file.close();
    }

    /**
     * @brief Combines all methods important for a workload
     */
    void execute()
    {
        std::cout << "Initializing ..." << std::endl;
        initialize();
        std::cout << "Initializing done" << std::endl
                  << std::flush;

        int iteration = 1;
        
        std::cout << "Vary thread count started..." << std::endl;

        for (int i = 0; i < 3; i++)
        {
            for (auto &thread : threads)
            {                
                run_workload("vary thread count", iteration, buffer_size, record_count, operation_count, "geometric", 0.001, workloads[i][0], workloads[i][1], workloads[i][2], workloads[i][3], workloads[i][4], true, radix_tree_size, i, false, thread);
                iteration++;
                run_workload("vary thread count", iteration, buffer_size, record_count, operation_count, "geometric", 0.001, workloads[i][0], workloads[i][1], workloads[i][2], workloads[i][3], workloads[i][4], false, radix_tree_size, i, false, thread);
                iteration++;
            }
        }

        std::cout << "Vary mthread count tests completed..." << std::endl;

        std::cout << "All tests completed!" << std::endl;
    }
};