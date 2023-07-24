
#pragma once

#include <string>
#include "spdlog/spdlog.h"
#include <chrono>
#include <random>
#include <set>
#include "../data/data_manager.h"
#include <sys/resource.h>
#include <iostream>

/**
 * @brief General abstraction for the YCSB workload
 */
class Workload
{
private:
    uint64_t record_count;
    uint64_t operation_count;
    std::string distribution;
    double coefficient;
    double insert_proportion;
    double read_proportion;
    double update_proportion;
    double scan_proportion;
    double delete_proportion;
    bool measure_per_operation;
    int max_scan_range = 100;

    std::shared_ptr<spdlog::logger> logger;
    DataManager<Configuration::page_size> data_manager;
    std::vector<std::vector<double>> times;
    std::set<int64_t> records_set;
    std::vector<int64_t> records_vector; /// entries inserted into the DB
    std::vector<int> indice_vector;      /// indices in the records vector that are addressed
    std::random_device rd;
    std::function<int()> index_distribution;
    std::uniform_int_distribution<int64_t> value_distribution;
    std::mt19937 generator;
    int insert_index = 0; /// offset at the end of records that specifies where current insert operations draw elements from

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
    std::set<int> delete_operations;          /// indices in the record vector that are deleted
    std::set<int> update_operations;          /// indices in the record vector that are deleted

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
     * @brief Initializes the vectors for records, operations and indices
     */
    void initialize()
    {
        while (records_set.size() < record_count)
        {
            // Generate uniform random number and insert into set
            records_set.insert(value_distribution(generator));
        }

        // Function for index distribution
        if (distribution == "uniform")
        {
            std::uniform_int_distribution<int> dist(0, record_count - 1);

            // Store the lambda function
            index_distribution = [this, dist]() mutable -> int
            { return dist(rd); };
        }
        else if (distribution == "geometric")
        {
            std::geometric_distribution<int> dist(coefficient);
            ;

            index_distribution = [this, dist]() mutable -> int
            {
                int num = dist(generator);
                if (num >= this->record_count)
                    num = this->record_count - 1;
                return num;
            };
        }
        else
        {
            logger->error("Invalid distribution name");
            exit(1);
        }

        // Weights for distribution, corresponding to the operations
        std::vector<double> weights = {insert_proportion, read_proportion, update_proportion, scan_proportion, delete_proportion};

        // Use for discrete distribution
        std::discrete_distribution<> op_dist(weights.begin(), weights.end());

        for (int i = 0; i < operation_count; i++)
        {
            Operation op = static_cast<Operation>(op_dist(rd));
            operations_vector[i] = op;

            int index = index_distribution();
            indice_vector[i] = index;
            if (op == DELETE)
            {
                delete_operations.insert(index);
            }
            else if (op == UPDATE)
            {
                update_operations.insert(index);
            }

            if (op == INSERT)
            {
                while (records_set.size() < record_count)
                {
                    // Generate uniform random number and insert into set
                    records_set.insert(value_distribution(generator));
                }
            }
        }

        // Convert set to vector
        std::copy(records_set.begin(), records_set.end(), std::back_inserter(records_vector));

        records_set.clear();

        // Inserting all elements
        for (int i = 0; i < record_count; i++)
        {
            if (i % 1000000 == 0)
            {
                std::cout << "Inserted " << i << " elements" << std::endl << std::flush;
            }
            logger->debug("Inserting in workload initialization: {}", records_vector[i]);
            data_manager.insert(records_vector[i], records_vector[i]);
        }
    }

    /**
     * @brief Runs the workload
     */
    void run()
    {
        if (insert_proportion + read_proportion + update_proportion + scan_proportion + delete_proportion != 1)
        {
            logger->error("Proportions don't add up to 1");
            exit(1);
        }

        const int thread_count = 1;
        int num_op_per_thread = operation_count / thread_count;

        for (int t = 0; t < thread_count; t++)
        {
            if (measure_per_operation)
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
            else
            {
                std::chrono::time_point<std::chrono::high_resolution_clock> total_start, total_end;

                total_start = std::chrono::high_resolution_clock::now();
                for (int i = thread_count * t; i < thread_count * t + num_op_per_thread; i++)
                {
                    perform_operation(operations_vector[i], i);
                }
                total_end = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> total_elapsed = total_end - total_start;
                times[0].push_back(total_elapsed.count());
            }
        }
    }

    /**
     * @brief Validates the final state of the tree
     */
    void validate()
    {
        std::cout << "Starting validation ..." << std::endl;

        int faulty_records = 0;

        for (int i = 0; i < records_vector.size(); i++)
        {
            if (delete_operations.count(i) > 0)
            {
                if (data_manager.get_value(records_vector[i]) != INT64_MIN)
                    faulty_records++;
            }
            else if (update_operations.count(i) > 0)
            {
                if (data_manager.get_value(records_vector[i]) != records_vector[i] + 1)
                {
                    std::cout << "Expected: " << records_vector[i] + 1 << " but got: " << data_manager.get_value(records_vector[i]) << std::endl;
                    faulty_records++;
                }
            }
            else
            {
                if (data_manager.get_value(records_vector[i]) != records_vector[i])
                    faulty_records++;
            }
        }

        std::cout << "Checking for content: " << faulty_records << " out of " << records_vector.size() << " are faulty" << std::endl;

        data_manager.validate(records_vector.size() - delete_operations.size());
        std::cout << "\n";
    }

    /**
     * @brief Analyzes the results of the workload
     */
    void analyze()
    {
        struct rusage usage;
        getrusage(RUSAGE_SELF, &usage);
        std::cout << "Memory usage: " << usage.ru_maxrss << " KB" << std::endl;
        std::cout << "\n";

        if (!measure_per_operation)
        {
            double total_time = times[0][0];
            double throughput = operation_count / total_time;

            std::cout << "Total time for all operations: " << std::fixed << std::setprecision(2) << total_time << "s\n";
            std::cout << "Throughput: " << std::fixed << std::setprecision(2) << throughput << " operations/s\n";
        }
        else
        {
            std::vector<std::string> operation_names = {"INSERT", "READ", "UPDATE", "SCAN", "DELETE"};

            for (int i = 0; i < NUM_OPERATIONS; ++i)
            {
                std::vector<double> &operation_times = times[i];
                if (operation_times.empty())
                {
                    logger->info("No operations for {}", operation_names[i]);
                    continue;
                }

                double sum = std::accumulate(operation_times.begin(), operation_times.end(), 0.0);
                double mean = sum / operation_times.size();

                std::sort(operation_times.begin(), operation_times.end());
                double median = operation_times[operation_times.size() / 2];
                if (operation_times.size() % 2 == 0)
                {
                    median = (operation_times[operation_times.size() / 2 - 1] + median) / 2;
                }

                double percentile_90 = operation_times[int(0.9 * operation_times.size())];
                double percentile_95 = operation_times[int(0.95 * operation_times.size())];
                double percentile_99 = operation_times[int(0.99 * operation_times.size())];

                std::cout << "Analysis for " << operation_names[i] << " operations:\n";
                std::cout << "Total number of operations: " << operation_times.size() << "\n";
                std::cout << "Total time: " << std::fixed << std::setprecision(6) << sum << "s\n";
                std::cout << "Mean time: " << std::fixed << std::setprecision(6) << mean << "s\n";
                std::cout << "Median time: " << std::fixed << std::setprecision(6) << median << "s\n";
                std::cout << "90th percentile time: " << std::fixed << std::setprecision(6) << percentile_90 << "s\n";
                std::cout << "95th percentile time: " << std::fixed << std::setprecision(6) << percentile_95 << "s\n";
                std::cout << "99th percentile time: " << std::fixed << std::setprecision(6) << percentile_99 << "s\n";
                std::cout << "\n";
            }
        }
    }

public:
    /**
     * @brief Constructor for the Workload
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
     * @param measure_per_operation Decides about the type of measurements
     */
    Workload(uint64_t buffer_size_arg, uint64_t record_count_arg, uint64_t operation_count_arg, std::string distribution_arg, double coefficient_arg, double insert_proportion_arg, double read_proportion_arg, double update_proportion_arg, double scan_proportion_arg, double delete_proportion_arg, bool cache_arg, uint64_t radix_tree_size_arg, bool measure_per_operation_arg) : record_count(record_count_arg), operation_count(operation_count_arg), distribution(distribution_arg), coefficient(coefficient_arg), insert_proportion(insert_proportion_arg), read_proportion(read_proportion_arg), update_proportion(update_proportion_arg), scan_proportion(scan_proportion_arg), delete_proportion(delete_proportion_arg), measure_per_operation(measure_per_operation_arg), data_manager(buffer_size_arg, cache_arg, radix_tree_size_arg)
    {
        logger = spdlog::get("logger");
        times = std::vector<std::vector<double>>(NUM_OPERATIONS, std::vector<double>(0, 0.0));
        value_distribution = std::uniform_int_distribution<int64_t>(INT64_MIN + 1, INT64_MAX - 1);
        generator = std::mt19937(42);
        operations_vector.resize(operation_count_arg);
        indice_vector.resize(operation_count_arg);
        insert_index = record_count;
    }

    ~Workload()
    {
        data_manager.destroy();
    }

    /**
     * @brief Combines all methods important for a workload
     */
    void execute()
    {
        // set up the data
        initialize();

        // run the workload
        run();

        // validate results
        validate();

        // analyze the run
        analyze();
    }
};

/**
 * @brief Abstraction for the YCSB workload A
 */
class WorkloadA : public Workload
{
public:
    /**
     * @brief Constructor for the Workload A
     * @param buffer_size_arg The amounts of pages in the buffer manager
     * @param record_count_arg The number of records in the trees
     * @param operation_count_arg The number of performed operations
     * @param distribution_arg The distribution
     * @param coefficient_arg The coefficient for the distribution
     * @param cache_arg If the cache is activated or not
     * @param radix_tree_size_arg The size of the cache
     * @param measure_per_operation Decides about the type of measurements
     */
    WorkloadA(uint64_t buffer_size_arg, uint64_t record_count_arg, uint64_t operation_count_arg, std::string distribution_arg, double coefficient_arg, bool cache_arg, uint64_t radix_tree_size_arg, bool measure_per_operation_arg)
        : Workload(buffer_size_arg, record_count_arg, operation_count_arg, distribution_arg, coefficient_arg, 0, 0.5, 0.5, 0, 0, cache_arg, radix_tree_size_arg, measure_per_operation_arg)
    {
    }
};

/**
 * @brief Abstraction for the YCSB workload B
 */
class WorkloadB : public Workload
{
public:
    /**
     * @brief Constructor for the Workload B
     * @param buffer_size_arg The amounts of pages in the buffer manager
     * @param record_count_arg The number of records in the trees
     * @param operation_count_arg The number of performed operations
     * @param distribution_arg The distribution
     * @param coefficient_arg The coefficient for the distribution
     * @param cache_arg If the cache is activated or not
     * @param radix_tree_size_arg The size of the cache
     * @param measure_per_operation Decides about the type of measurements
     */
    WorkloadB(uint64_t buffer_size_arg, uint64_t record_count_arg, uint64_t operation_count_arg, std::string distribution_arg, double coefficient_arg, bool cache_arg, uint64_t radix_tree_size_arg, bool measure_per_operation_arg)
        : Workload(buffer_size_arg, record_count_arg, operation_count_arg, distribution_arg, coefficient_arg, 0, 0.95, 0.05, 0, 0, cache_arg, radix_tree_size_arg, measure_per_operation_arg)
    {
    }
};

/**
 * @brief Abstraction for the YCSB workload C
 */
class WorkloadC : public Workload
{
public:
    /**
     * @brief Constructor for the Workload C
     * @param buffer_size_arg The amounts of pages in the buffer manager
     * @param record_count_arg The number of records in the trees
     * @param operation_count_arg The number of performed operations
     * @param distribution_arg The distribution
     * @param coefficient_arg The coefficient for the distribution
     * @param cache_arg If the cache is activated or not
     * @param radix_tree_size_arg The size of the cache
     * @param measure_per_operation Decides about the type of measurements
     */
    WorkloadC(uint64_t buffer_size_arg, uint64_t record_count_arg, uint64_t operation_count_arg, std::string distribution_arg, double coefficient_arg, bool cache_arg, uint64_t radix_tree_size_arg, bool measure_per_operation_arg)
        : Workload(buffer_size_arg, record_count_arg, operation_count_arg, distribution_arg, coefficient_arg, 0, 1, 0, 0, 0, cache_arg, radix_tree_size_arg, measure_per_operation_arg)
    {
    }
};

/**
 * @brief Abstraction for the YCSB workload E
 */
class WorkloadE : public Workload
{
public:
    /**
     * @brief Constructor for the Workload E
     * @param buffer_size_arg The amounts of pages in the buffer manager
     * @param record_count_arg The number of records in the trees
     * @param operation_count_arg The number of performed operations
     * @param distribution_arg The distribution
     * @param coefficient_arg The coefficient for the distribution
     * @param cache_arg If the cache is activated or not
     * @param radix_tree_size_arg The size of the cache
     * @param measure_per_operation Decides about the type of measurements
     */
    WorkloadE(uint64_t buffer_size_arg, uint64_t record_count_arg, uint64_t operation_count_arg, std::string distribution_arg, double coefficient_arg, bool cache_arg, uint64_t radix_tree_size_arg, bool measure_per_operation_arg)
        : Workload(buffer_size_arg, record_count_arg, operation_count_arg, distribution_arg, coefficient_arg, 0.05, 0, 0, 0.95, 0, cache_arg, radix_tree_size_arg, measure_per_operation_arg)
    {
    }
};

/**
 * @brief Abstraction for the YCSB workload X
 */
class WorkloadX : public Workload
{
public:
    /**
     * @brief Constructor for the Workload X
     * @param buffer_size_arg The amounts of pages in the buffer manager
     * @param record_count_arg The number of records in the trees
     * @param operation_count_arg The number of performed operations
     * @param distribution_arg The distribution
     * @param coefficient_arg The coefficient for the distribution
     * @param cache_arg If the cache is activated or not
     * @param radix_tree_size_arg The size of the cache
     * @param measure_per_operation Decides about the type of measurements
     */
    WorkloadX(uint64_t buffer_size_arg, uint64_t record_count_arg, uint64_t operation_count_arg, std::string distribution_arg, double coefficient_arg, bool cache_arg, uint64_t radix_tree_size_arg, bool measure_per_operation_arg)
        : Workload(buffer_size_arg, record_count_arg, operation_count_arg, distribution_arg, coefficient_arg, 0, 0.90, 0, 0, 0.1, cache_arg, radix_tree_size_arg, measure_per_operation_arg)
    {
    }
};