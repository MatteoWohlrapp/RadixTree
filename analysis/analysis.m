%% TODO:
% Copy distributions labels for the 'vary memory distribution'
% Change path to csv file

%% Load Data
% Load data from CSV
data = readtable('../results/2023-8-7-10:14:34_test_results.csv');

% Convert cell array of character arrays to string array
data.TestName = string(data.TestName);

% Convert cell array of logical values or numbers to logical array
data.Cache = strcmp(data.Cache, 'true');

data.Distribution = string(data.Distribution);


workloadNames = {'Workload A', 'Workload B', 'Workload C', 'Workload E', 'Workload X'};  % You should replace these with the actual names of your workloads

%% Multi Threaded 

for i = 0:2
    figure;
    workload_cached = data(strcmp(data.TestName, 'vary thread count') & data.Workload == i & data.Cache, :);
    plot(workload_cached.Threads, workload_cached.Throughput, 'DisplayName', 'Cached');
    hold on;
    workload_not_cached = data(strcmp(data.TestName, 'vary thread count') & data.Workload == i & ~data.Cache, :);
    plot(workload_not_cached.Threads, workload_not_cached.Throughput, 'DisplayName', 'Not Cached');
    t = [workloadNames{i+1} ' Throughput multiple threads'];
    xlabel('Number of threads');
    ylabel('Throughput [Operations / s]');
    grid on;
    legend('Location', 'southeast');

    max_throughput = max([workload_cached.Throughput; workload_not_cached.Throughput]);
    step = 100000; % You can modify this step value as per your need
    yticks_values = 0:step:ceil(max_throughput/step)*step;

    % Generate new labels
    yticklabels_values = arrayfun(@(x) sprintf('%.0fK', x / 1e3), yticks_values, 'UniformOutput', false);

    % Set new labels
    set(gca, 'ytick', yticks_values);
    set(gca, 'yticklabel', yticklabels_values);

    ylim([0 ceil(max_throughput/step)*step]);
    set(gca, 'XTick', unique([workload_cached.Threads; workload_not_cached.Threads]));

matlab2tikz([t '.tex'], 'extraAxisOptions', {'scaled y ticks=false','scaled x ticks=false', 'yshift=-2ex'}, 'showInfo', false);

    hold off;
end