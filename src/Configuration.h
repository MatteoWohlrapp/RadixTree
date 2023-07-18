/**
 * @file    cofiguration.h
 *
 * @author  Matteo Wohlrapp
 * @date    16.05.2023
 */

#pragma once

/**
 * @brief namespace that contains all important variables for the configuration of the database
 */
namespace Configuration
{
    /// sets the overall page_size for the pages written to memory. Subject to constraints: [((PAGE_SIZE - 32) / 2) / 8] > 2, also dividable by 16 for header alginment
    constexpr int page_size = 96;

    struct Configuration
    {
        int buffer_size = 5;                  /// size of database availabile in memory
        int record_count = 1000;              /// number of elements saved during workload
        int operation_count = 1000;           /// operation count during workload
        std::string distribution = "uniform"; /// distribution during workload
        double insert_proportion = 0;         /// proportion of inserts during workload
        double read_proportion = 0.5;         /// proportion of read during workload
        double update_proportion = 0.5;       /// proportion of update during workload
        double scan_proportion = 0;           /// proportion of scan during workload
        double delete_proportion = 0;         /// proportion of delete during workload
        bool cache = false;                   /// if caching is enabled
        int radix_tree_size = 104857600;      /// Size of the cache, default here is 100 MB
        bool measure_per_operation = false;   /// Either measure throughput or individual operations which gives the percentiles etc.
        bool benchmark = false;               /// whether benchmarking is enabled or not, only applicable for run config
        bool run_workload = false;            /// if a workload or a run config should be run
    };
}
