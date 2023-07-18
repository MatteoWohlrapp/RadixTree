
#pragma once

#include <string>
#include "spdlog/spdlog.h"
#include <chrono>
#include <random>
#include <set>
#include "../data/data_manager.h"

class Workload
{
protected:
    int buffer_size;
    int record_count;
    int operation_count;
    std::string distribution;
    double coefficient; 
    double insert_proportion;
    double read_proportion;
    double update_proportion;
    double scan_proportion;
    double delete_proportion;
    bool cache;
    int radix_tree_size;
    bool measure_per_operation;
    int max_scan_range = 100;

    std::shared_ptr<spdlog::logger> logger;
    DataManager data_manager;
    std::vector<std::vector<double>> times;
    std::set<uint64_t> records_set;
    std::vector<uint64_t> records_vector;
    std::random_device rd;
    std::function<int()> index_distribution;
    std::uniform_int_distribution<int64_t> value_distribution;

    enum Operation
    {
        INSERT,
        READ,
        UPDATE,
        SCAN,
        DELETE,
        NUM_OPERATIONS
    };

    void perform_operation(Operation op, int64_t key)
    {
        switch (op)
        {
        case INSERT:
            logger->debug("Inserting: {}", key);
            data_manager.insert(key, key);
            break;
        case READ:
            logger->debug("Reading: {}", key);
            data_manager.get_value(key);
            break;
        case UPDATE:
            logger->debug("Updating: {}", key);
            data_manager.update(key, key);
            break;
        case SCAN:
            logger->debug("Scanning: {}", key);
            data_manager.scan(key, max_scan_range);
            break;
        case DELETE:
            logger->debug("Deleting: {}", key);
            data_manager.delete_value(key);
            break;
        case NUM_OPERATIONS:
            break;
        }
    }

    void initialize()
    {
        while (records_set.size() < record_count)
        {
            // Generate uniform random number and insert into set
            records_set.insert(value_distribution(rd));
        }

        // Convert set to vector
        std::copy(records_set.begin(), records_set.end(), std::back_inserter(records_vector));

        // Inserting all elements
        for (int i = 0; i < record_count; i++)
        {
            logger->debug("Inserting: {}", records_vector[i]);
            data_manager.insert(records_vector[i], records_vector[i]);
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
            std::geometric_distribution<int> dist(coefficient);;

            index_distribution = [this, dist]() mutable -> int
            {
                int num = dist(rd);
                if(num >= this->record_count)
                    num = this->record_count - 1;  
                return num;
            };
        }
        else
        {
            logger->error("Invalid distribution name");
            exit(1);
        }
    }

    void run()
    {
        if (insert_proportion + read_proportion + update_proportion + scan_proportion + delete_proportion != 1)
        {
            logger->error("Proportions don't add up to 1");
            exit(1);
        }

        // Weights for distribution, corresponding to the operations
        std::vector<double> weights = {insert_proportion, read_proportion, update_proportion, scan_proportion, delete_proportion};

        // Use for discrete distribution
        std::discrete_distribution<> dist(weights.begin(), weights.end());

        int index = -1;
        int64_t key = INT64_MIN;

        std::chrono::time_point<std::chrono::high_resolution_clock> total_start, total_end;

        if (!measure_per_operation)
        {
            total_start = std::chrono::high_resolution_clock::now();
        }

        for (int i = 0; i < operation_count; i++)
        {
            Operation op = static_cast<Operation>(dist(rd));
            if (op == INSERT)
            {
                // Generate new unique key for insert
                do
                {
                    key = value_distribution(rd);
                } while (records_set.count(key) > 0);
            }
            else
            {
                index = index_distribution();
                key = records_vector[index];
            }

            if (measure_per_operation)
            {
                std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
                start = std::chrono::high_resolution_clock::now();
                perform_operation(op, key);
                end = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> elapsed = end - start;
                times[op].push_back(elapsed.count());
            }
            else
            {
                perform_operation(op, key);
            }
        }

        if (!measure_per_operation)
        {
            total_end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> total_elapsed = total_end - total_start;
            times[0].push_back(total_elapsed.count());
        }
    }

    void analyze()
    {
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
                std::cout << "Total time: " << std::fixed << std::setprecision(6) << sum << "s\n";
                std::cout << "Mean time: " << std::fixed << std::setprecision(6) << mean << "s\n";
                std::cout << "Median time: " << std::fixed << std::setprecision(6) << median << "s\n";
                std::cout << "90th percentile time: " << std::fixed << std::setprecision(6) << percentile_90 << "s\n";
                std::cout << "95th percentile time: " << std::fixed << std::setprecision(6) << percentile_95 << "s\n";
                std::cout << "99th percentile time: " << std::fixed << std::setprecision(6) << percentile_99 << "s\n";
            }
        }
    }

public:
    Workload(int buffer_size_arg, int record_count_arg, int operation_count_arg, std::string distribution_arg, double coefficient_arg, double insert_proportion_arg, double read_proportion_arg, double update_proportion_arg, double scan_proportion_arg, double delete_proportion_arg, bool cache_arg, int radix_tree_size_arg, bool measure_per_operation_arg) : buffer_size(buffer_size_arg), record_count(record_count_arg), operation_count(operation_count_arg), distribution(distribution_arg), coefficient(coefficient_arg), insert_proportion(insert_proportion_arg), read_proportion(read_proportion_arg), update_proportion(update_proportion_arg), scan_proportion(scan_proportion_arg), delete_proportion(delete_proportion_arg), cache(cache_arg), radix_tree_size(radix_tree_size_arg), measure_per_operation(measure_per_operation_arg), data_manager(buffer_size_arg, cache_arg, radix_tree_size_arg)
    {
        logger = spdlog::get("logger");
        times = std::vector<std::vector<double>>(NUM_OPERATIONS, std::vector<double>(0, 0.0));
        value_distribution = std::uniform_int_distribution<int64_t>(INT64_MIN + 1, INT64_MAX);
    }

    ~Workload()
    {
        data_manager.destroy();
    }

    void execute()
    {
        // set up the data
        initialize();

        // run the workload
        run();

        // analyze the run
        analyze();
    }
};

class WorkloadA : public Workload
{
public:
    WorkloadA(int buffer_size_arg, int record_count_arg, int operation_count_arg, std::string distribution_arg, double coefficient_arg, bool cache_arg, int radix_tree_size_arg, bool measure_per_operation_arg)
        : Workload(buffer_size_arg, record_count_arg, operation_count_arg, distribution_arg, coefficient_arg, 0, 0.5, 0.5, 0, 0, cache_arg, radix_tree_size_arg, measure_per_operation_arg)
    {
    }
};

class WorkloadB : public Workload
{
public:
    WorkloadB(int buffer_size_arg, int record_count_arg, int operation_count_arg, std::string distribution_arg, double coefficient_arg, bool cache_arg, int radix_tree_size_arg, bool measure_per_operation_arg)
        : Workload(buffer_size_arg, record_count_arg, operation_count_arg, distribution_arg, coefficient_arg, 0, 0.95, 0.05, 0, 0, cache_arg, radix_tree_size_arg, measure_per_operation_arg)
    {
    }
};

class WorkloadC : public Workload
{
public:
    WorkloadC(int buffer_size_arg, int record_count_arg, int operation_count_arg, std::string distribution_arg, double coefficient_arg, bool cache_arg, int radix_tree_size_arg, bool measure_per_operation_arg)
        : Workload(buffer_size_arg, record_count_arg, operation_count_arg, distribution_arg, coefficient_arg, 0, 1, 0, 0, 0, cache_arg, radix_tree_size_arg, measure_per_operation_arg)
    {
    }
};

class WorkloadE : public Workload
{
public:
    WorkloadE(int buffer_size_arg, int record_count_arg, int operation_count_arg, std::string distribution_arg, double coefficient_arg, bool cache_arg, int radix_tree_size_arg, bool measure_per_operation_arg)
        : Workload(buffer_size_arg, record_count_arg, operation_count_arg, distribution_arg, coefficient_arg, 0.05, 0, 0, 0.95, 0, cache_arg, radix_tree_size_arg, measure_per_operation_arg)
    {
    }
};

class WorkloadX : public Workload
{
public:
    WorkloadX(int buffer_size_arg, int record_count_arg, int operation_count_arg, std::string distribution_arg, double coefficient_arg, bool cache_arg, int radix_tree_size_arg, bool measure_per_operation_arg)
        : Workload(buffer_size_arg, record_count_arg, operation_count_arg, distribution_arg, coefficient_arg, 0, 0.90, 0, 0, 0.1, cache_arg, radix_tree_size_arg, measure_per_operation_arg)
    {
    }
};