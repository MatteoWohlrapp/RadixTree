% Load data from CSV
data = readtable('/Users/matteowohlrapp/Documents/Uni/Thesis/radixtree/results/2023-7-26-17:52:54_test_results_test.csv');

% Convert cell array of character arrays to string array
data.TestName = string(data.TestName);

% Convert cell array of logical values or numbers to logical array
data.Cache = strcmp(data.Cache, 'true');

% Select rows for each workload and cache condition
workload_a_cached = data(strcmp(data.TestName, 'vary record size') & data.Workload == 0 & data.Cache, :);
workload_a_not_cached = data(strcmp(data.TestName, 'vary record size') & data.Workload == 0 & ~data.Cache, :);

% Other workloads go here...

% Plot Workload A
figure;
plot(workload_a_cached.RecordCount, workload_a_cached.Throughput, 'DisplayName', 'Cached');
hold on;
plot(workload_a_not_cached.RecordCount, workload_a_not_cached.Throughput, 'DisplayName', 'Not Cached');
hold off;
title('Workload A');
xlabel('Number of records in millions');
ylabel('Throughput/s');
legend('Location', 'southeast');
grid on;

% Other plots go here...
p = 0.001; % Geometric distribution parameter, change as necessary
x = 0:20000000; % Values
y = geopdf(x,p); % Geometric Probability Distribution Function

figure
semilogy(x,y) % Plot on a semilogarithmic scale to better visualize the data
title('Geometric Distribution')
xlabel('x')
ylabel('Probability')