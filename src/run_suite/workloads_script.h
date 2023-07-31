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
    uint64_t total_records = 12500000;
    uint64_t buffer_size = 40000;         // around 0.3 gb - 0.5 gb at 15 mio entries
    uint64_t radix_tree_size = 697932185; // around 1.3 gb - 1.5 gb at 15 mio entries
    uint64_t operation_count = 20000000;
    uint64_t record_count = 10000000;
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

    uint64_t operation_counts[5] = {10000000, 20000000, 30000000, 40000000, 50000000};
    uint64_t small_operations_counts[5] = {1000, 10000, 100000, 1000000, 10000000};
    uint64_t record_counts[5] = {2000000, 4000000, 6000000, 8000000, 10000000};
    uint64_t small_record_counts[5] = {200, 2000, 20000, 200000, 2000000};
    double coefficients[10] = {0.0009, 0.009, 0.09, 0.9};
    std::vector<std::string> distributions = {"uniform", "geometric"};
    bool caches[2] = {true, false};
    double workloads[5][5] = {
        {0, 0.5, 0.5, 0, 0}, {0, 0.95, 0.05, 0, 0}, {0, 1, 0, 0, 0}, {0.05, 0, 0, 0.95, 0}, {0, 0.90, 0, 0, 0.1}};
    // uint64_t memory_distributions[5][2] = {{262144000, 448000}, {524288000, 1572864000}, {1048576000, 256000}, {1572864000, 128000}, {1835008000, 64000}};
    uint64_t memory_distributions[5][2] = {{131072000, 224000}, {262144000, 192000}, {524288000, 128000}, {786432000, 64000}, {917504000, 32000}};

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
            logger->debug("Inserting in workload: {}", records_vector[insert_index]);
            data_manager.insert(records_vector[insert_index], records_vector[insert_index]);
            insert_index++;
            break;
        case READ:
        {
            logger->debug("Reading: {}", records_vector[indice_vector[index]]);
            data_manager.get_value(records_vector[indice_vector[index]]);
        }
        break;
        case UPDATE:
            logger->debug("Updating: {}", records_vector[indice_vector[index]]);
            data_manager.update(records_vector[indice_vector[index]], records_vector[indice_vector[index]] + 1);
            break;
        case SCAN:
            logger->debug("Scanning: {}", records_vector[indice_vector[index]]);
            data_manager.scan(records_vector[indice_vector[index]], max_scan_range);
            break;
        case DELETE:
            logger->debug("Deleting: {}", records_vector[indice_vector[index]]);
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
     * @param insert_proportion The proportion of inserts
     * @param read_proportion The proportion of reads
     * @param update_proportion The proportion of updates
     * @param scan_proportion The proportion of scancs
     * @param delete_proportion The proportion of deletes
     * @param cache_arg If the cache is activated or not
     * @param radix_tree_size_arg The size of the cache
     * @param workload_arg The workload that is run
     */
    void run_workload(std::string test_name, int iteration, uint64_t buffer_size_arg, uint64_t record_count_arg, uint64_t operation_count_arg, std::string distribution_arg, double coefficient_arg, double insert_proportion_arg, double read_proportion_arg, double update_proportion_arg, double scan_proportion_arg, double delete_proportion_arg, bool cache_arg, uint64_t radix_tree_size_arg, int workload_arg, bool inverse = false)
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
            for (uint64_t i = record_count_arg - 1; i >= 0; i--)
            {
                data_manager.insert(records_vector[i], records_vector[i]);
            }
        }

        insert_index = record_count_arg;

        const int thread_count = 1;
        int num_op_per_thread = operation_count_arg / thread_count;

        for (int t = 0; t < thread_count; t++)
        {
            std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
            Operation op;
            for (int i = thread_count * t; i < thread_count * t + num_op_per_thread; i++)
            {
                op = operations_vector[i];
                start = std::chrono::high_resolution_clock::now();
                perform_operation(op, i);
                end = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> elapsed = end - start;
                times[op].push_back(elapsed.count());
            }
        }
        uint64_t cache_size = data_manager.get_cache_size();
        uint64_t current_buffer_size = data_manager.get_current_buffer_size();

        analyze(test_name, iteration, buffer_size_arg, record_count_arg, operation_count_arg, distribution_arg, coefficient_arg, insert_proportion_arg, read_proportion_arg, update_proportion_arg, scan_proportion_arg, delete_proportion_arg, cache_arg, radix_tree_size_arg, cache_size, current_buffer_size, workload_arg);

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
     * @param insert_proportion The proportion of inserts
     * @param read_proportion The proportion of reads
     * @param update_proportion The proportion of updates
     * @param scan_proportion The proportion of scancs
     * @param delete_proportion The proportion of deletes
     * @param cache_arg If the cache is activated or not
     * @param radix_tree_size_arg The size of the cache
     * @param cache_size_arg The actual size of the cache
     * @param current_buffer_size_arg The size of the buffer
     * @param workload_arg The workload that is run
     */
    void analyze(std::string test_name, int iteration, uint64_t buffer_size_arg, uint64_t record_count_arg, uint64_t operation_count_arg, std::string distribution_arg, double coefficient_arg, double insert_proportion_arg, double read_proportion_arg, double update_proportion_arg, double scan_proportion_arg, double delete_proportion_arg, bool cache_arg, uint64_t radix_tree_size_arg, uint64_t cache_size_arg, uint64_t current_buffer_size_arg, int workload_arg)
    {
        std::vector<OperationResult> operation_results(NUM_OPERATIONS);

        struct rusage usage;
        getrusage(RUSAGE_SELF, &usage);

        std::vector<std::string> operation_names = {"INSERT", "READ", "UPDATE", "SCAN", "DELETE"};
        uint64_t total_operations = 0;
        double total_time = 0;

        for (int i = 0; i < NUM_OPERATIONS; ++i)
        {
            std::vector<double> &operation_times = times[i];
            OperationResult &result = operation_results[i];

            if (operation_times.empty())
            {
                continue;
            }

            double sum = std::accumulate(operation_times.begin(), operation_times.end(), 0.0);
            result.total_time = sum;
            result.operation_count = operation_times.size();
            total_operations += operation_times.size();
            total_time += sum;

            std::sort(operation_times.begin(), operation_times.end());
            double median = operation_times[operation_times.size() / 2];
            if (operation_times.size() % 2 == 0)
            {
                median = (operation_times[operation_times.size() / 2 - 1] + median) / 2;
            }
            result.median_time = median;

            double percentile_90 = operation_times[int(0.9 * operation_times.size())];
            double percentile_95 = operation_times[int(0.95 * operation_times.size())];
            double percentile_99 = operation_times[int(0.99 * operation_times.size())];
            result.percentile_90 = percentile_90;
            result.percentile_95 = percentile_95;
            result.percentile_99 = percentile_99;

            result.mean_time = sum / operation_times.size();
        }
        // Write to CSV file
        csv_file.open(results_filename, std::ios_base::app);
        csv_file << test_name << "," << iteration << "," << buffer_size_arg << "," << record_count_arg << "," << operation_count_arg << ","
                 << distribution_arg << "," << workload_arg << "," << insert_proportion_arg << "," << read_proportion_arg << ","
                 << update_proportion_arg << "," << scan_proportion_arg << "," << delete_proportion_arg << ","
                 << (cache_arg ? "true" : "false") << "," << radix_tree_size_arg << "," << std::fixed << std::setprecision(5) << coefficient_arg << ",";

        for (const OperationResult &result : operation_results)
        {
            csv_file << result.operation_count << "," << std::fixed << std::setprecision(10) << result.total_time << ","
                     << result.mean_time << "," << result.median_time << "," << result.percentile_90 << ","
                     << result.percentile_95 << "," << result.percentile_99 << ",";
        }
        csv_file << cache_size_arg << "," << current_buffer_size_arg << "," << std::fixed << std::setprecision(2) << total_time << ","
                 << total_operations / total_time << "\n";
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
        csv_file << "TestName,Iteration,BufferSize,RecordCount,OperationCount,Distribution,Workload,InsertProportion,ReadProportion,UpdateProportion,ScanProportion,DeleteProportion,Cache,RadixTreeSize,Coefficient,InsertOperationCount,InsertTotalTime,InsertMeanTime,InsertMedianTime,Insert90Percentile,Insert95Percentile,Insert99Percentile,ReadOperationCount,ReadTotalTime,ReadMeanTime,ReadMedianTime,Read90Percentile,Read95Percentile,Read99Percentile,UpdateOperationCount,UpdateTotalTime,UpdateMeanTime,UpdateMedianTime,Update90Percentile,Update95Percentile,Update99Percentile,ScanOperationCount,ScanTotalTime,ScanMeanTime,ScanMedianTime,Scan90Percentile,Scan95Percentile,Scan99Percentile,DeleteOperationCount,DeleteTotalTime,DeleteMeanTime,DeleteMedianTime,Delete90Percentile,Delete95Percentile,Delete99Percentile,CacheSize,CurrentBufferSize,TotalTime,Throughput\n";
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
        std::cout << "Vary records-size tests started..." << std::endl;

        iteration = 1;
        for (auto &cache : caches)
        {
            for (int i = 0; i < 5; i++)
            {
                for (auto &record_count_l : record_counts)
                {
                    run_workload("vary record size", iteration, buffer_size, record_count_l, operation_count, "geometric", 0.001, workloads[i][0], workloads[i][1], workloads[i][2], workloads[i][3], workloads[i][4], cache, radix_tree_size, i);
                    iteration++;
                }
            }
        }
        std::cout << "Vary records-size tests completed..." << std::endl;

        std::cout << "Vary distribution started..." << std::endl;
        for (auto &cache : caches)
        {
            for (int i = 0; i < 5; i++)
            {
                for (auto &distribution : distributions)
                {
                    run_workload("vary distribution", iteration, 4000, 500000, 500000, distribution, 0.001, workloads[i][0], workloads[i][1], workloads[i][2], workloads[i][3], workloads[i][4], cache, 69793215, i);
                    iteration++;
                }
            }
        }
        std::cout << "Vary distribution tests completed..." << std::endl;

        iteration = 1;
        std::cout << "Vary geometric distribution coefficient tests started..." << std::endl;
        for (auto &cache : caches)
        {

            for (auto &coefficient_l : coefficients)
            {
                run_workload("vary geometric distribution", iteration, buffer_size, record_count, operation_count, "geometric", coefficient_l, workloads[0][0], workloads[0][1], workloads[0][2], workloads[0][3], workloads[0][4], cache, radix_tree_size, 0, true);
                iteration++;
            }
        }
        std::cout << "Vary geometric distribution coefficient tests completed..." << std::endl;

        iteration = 1;
        std::cout << "Show memory size tests started..." << std::endl;

        for (int j = 0; j < 5; j++)
        {
            run_workload("vary memory size", iteration, 524288, small_record_counts[j], small_operations_counts[j], "geometric", 0.001, workloads[0][0], workloads[0][1], workloads[0][2], workloads[0][3], workloads[0][4], true, 2147483648, 0);
            iteration++;
        }

        std::cout << "Show memory size tests completed..." << std::endl;

        iteration = 1;
        std::cout << "Vary memory distribution coefficient tests started..." << std::endl;

        for (int i = 0; i < 5; i++)
        {
            for (auto &memory_distribution_l : memory_distributions)
            {
                run_workload("vary memory distribution", iteration, memory_distribution_l[1], record_count, operation_count, "geometric", 0.001, workloads[i][0], workloads[i][1], workloads[i][2], workloads[i][3], workloads[i][4], true, memory_distribution_l[0], i);
                iteration++;
            }
        }

        std::cout << "Vary memory distribution coefficient tests completed..." << std::endl;

        std::cout << "Speed tests started ...";
        for (int i = 0; i < 5; i++)
        {

            run_workload("speed comparison", iteration, 104857, record_count, operation_count, "geometric", 0.01, workloads[i][0], workloads[i][1], workloads[i][2], workloads[i][3], workloads[i][4], false, 0, i);
            iteration++;
            run_workload("speed comparison", iteration, 52428, record_count, operation_count, "geometric", 0.01, workloads[i][0], workloads[i][1], workloads[i][2], workloads[i][3], workloads[i][4], true, 214748364, i);
            iteration++;
        }
        std::cout << "Speed tests completed..." << std::endl;

        std::cout << "All tests completed!" << std::endl;
    }
};