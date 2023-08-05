%% TODO:
% Copy record count for the 'show memory size'
% Copy distributions for the 'vary memory distribution'

%% Load Data
% Load data from CSV
data = readtable('/Users/matteowohlrapp/Documents/Uni/Thesis/radixtree/results/2023-8-1-7:7:53_test_results-kemper.csv');

% Convert cell array of character arrays to string array
data.TestName = string(data.TestName);

% Convert cell array of logical values or numbers to logical array
data.Cache = strcmp(data.Cache, 'true');

data.Distribution = string(data.Distribution);


%% Vary Records Size

workloadNames = {'Workload A', 'Workload B', 'Workload C', 'Workload E', 'Workload X'};  % You should replace these with the actual names of your workloads

for i = 0:4
    figure;
    workload_cached = data(strcmp(data.TestName, 'vary record size') & data.Workload == i & data.Cache, :);
    plot(workload_cached.RecordCount, workload_cached.Throughput, 'DisplayName', 'Cached');
    hold on;
    workload_not_cached = data(strcmp(data.TestName, 'vary record size') & data.Workload == i & ~data.Cache, :);
    plot(workload_not_cached.RecordCount, workload_not_cached.Throughput, 'DisplayName', 'Not Cached');
    t = [workloadNames{i+1} ' Throughput'];
    xlabel('Number of records');
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
    set(gca, 'XTick', unique([workload_cached.RecordCount; workload_not_cached.RecordCount]));
    xticklabels_values = arrayfun(@(x) sprintf('%.1fM', x / 1e6), unique([workload_cached.RecordCount; workload_not_cached.RecordCount]), 'UniformOutput', false);
    set(gca, 'xticklabel', xticklabels_values);

matlab2tikz([t '.tex'], 'extraAxisOptions', {'scaled y ticks=false','scaled x ticks=false', 'yshift=-2ex'}, 'showInfo', false);

    hold off;
end


% Bar graph
max_record = 10000000;

operationsByWorkload = {
    {'Read', 'Update'},
    {'Read', 'Update'},
    {'Read'},
    {'Insert', 'Scan'},
    {'Read', 'Delete'},
    };
statistics = {'MeanTime', 'MedianTime', '90Percentile', '95Percentile', '99Percentile'};
statistics_groups = {'Mean Time', 'Median Time', '90th Percentile', '95th Percentile', '99th Percentile'};


% Loop over workloads
for i = 0:4
    figure;

    workload_cached = data(strcmp(data.TestName, 'vary record size') & data.Workload == i & data.Cache & max_record == data.RecordCount, :);
    workload_not_cached = data(strcmp(data.TestName, 'vary record size') & data.Workload == i & ~data.Cache & max_record == data.RecordCount, :);

    bars_data = zeros(length(statistics), length(operationsByWorkload{i+1}) * 2); % Initialize array

    for st = 1:length(statistics)
        for op = 0:(length(operationsByWorkload{i+1})-1)
            bars_data(st, op*2 + 1) = workload_cached.([operationsByWorkload{i+1}{op+1} statistics{st}]);
            bars_data(st, op*2 + 2) = workload_not_cached.([operationsByWorkload{i+1}{op+1} statistics{st}]);
        end
    end



    legends = {};
    for op = 1:length(operationsByWorkload{i+1})
        legends = [legends, {[operationsByWorkload{i+1}{op}, ', Cached' ], [operationsByWorkload{i+1}{op} ', Not Cached']}];
    end

    bar(bars_data)
    grid on;
    legend(legends, 'Location', 'southeast'); % Create legend using bar objects and labels
    set(gca, 'XTickLabel', statistics_groups);
    t = [workloadNames{i+1} ' Statistics'];
    ylabel('Time of one operation in microseconds');
    ylim([0 max(bars_data(:))*1.1]);

    % Get current yticks
    yticks = get(gca,'ytick');

    % Generate new labels
    yticklabels_values = arrayfun(@(x) sprintf('%.1f\\mu s', x * 1e6), yticks, 'UniformOutput', false);

    % Set new labels
    set(gca, 'yticklabel', yticklabels_values);

matlab2tikz([t '.tex'], 'extraAxisOptions', {'scaled y ticks=false','scaled x ticks=false'}, 'showInfo', false);
end

%% Vary Distribution

distributions = {'uniform', 'geometric'};
cache_text = {'not cached', 'cached'};

% Loop over workloads
for c = 0:1
    figure;
    bars_data = zeros(length(workloadNames), 2); % Initialize array
    for i = 0:4
        for di = 1:length(distributions)
            workload = data(strcmp(data.TestName, 'vary distribution') & data.Workload == i & data.Cache == c & data.Distribution == distributions{di}, :);
            bars_data(i+1,di) = workload.("Throughput");
        end
    end

    bar(bars_data);
    grid on;
    legend({'Uniform', 'Geometric'}, 'Location', 'southeast'); % Create legend using the distribution names
    set(gca, 'XTickLabel', workloadNames);
    t = ['Varying distribution ' cache_text{c+1}];
    ylabel('Throughput [Operations / s]');


    % Get current yticks
    yticks = get(gca,'ytick');

    % Generate new labels
    yticklabels_values = arrayfun(@(x) sprintf('%.0fK', x / 1e3), yticks, 'UniformOutput', false);

    % Set new labels
    set(gca, 'yticklabel', yticklabels_values);

matlab2tikz([t '.tex'], 'extraAxisOptions', {'scaled y ticks=false','scaled x ticks=false'}, 'showInfo', false);

    hold off
end


%% Vary Geometric Distribution

coefficients_table = data(strcmp(data.TestName, 'vary geometric distribution'), :);
coefficients = coefficients_table.Coefficient;
coefficients = sort(unique(coefficients)); 
for c = 0:1
    figure;
    bars_data = zeros(length(coefficients), length(workloadNames)); % Initialize array
    for p = 1:length(coefficients)
        workload = data(strcmp(data.TestName, 'vary geometric distribution') & data.Coefficient == coefficients(p) & data.Cache == c, :);
        bars_data(p, :) = workload.Throughput;
    end

    bar(bars_data);
    grid on;

    xlabel('p');
    grid on;
    legend(workloadNames ,'Location', 'southeast'); % Create legend using the distribution names
    t = ['Varying coefficient of geometric distribution ' cache_text{c+1}];
    ylabel('Throughput [Operations / s]');

    workload = data(strcmp(data.TestName, 'vary geometric distribution'), :);
    max_throughput = max(workload.Throughput);
    step = 100000; % You can modify this step value as per your need
    ylim([0 ceil(max_throughput/step)*step]);

    % Get current yticks
    yticks = get(gca,'ytick');

    % Generate new labels
    yticklabels_values = arrayfun(@(x) sprintf('%.0fK', x / 1e3), yticks, 'UniformOutput', false);

    % Set new labels
    set(gca, 'yticklabel', yticklabels_values);

    set(gca, 'XTickLabel', num2str(coefficients));

matlab2tikz([t '.tex'], 'extraAxisOptions', {'scaled y ticks=false','scaled x ticks=false'}, 'showInfo', false);

    hold off
end

% %% Show Memory Size
% 
% record_sizes_tab = unique(data(strcmp(data.TestName, 'vary memory size'), "RecordCount"));
% record_sizes = table2array(record_sizes_tab);
% record_sizes = record_sizes(:);
% memory_instances = {'Cache', 'Buffer'};
% bars_data = zeros(length(record_sizes), 2);
% figure;
% for i = 1:length(record_sizes)
%     workload = data(strcmp(data.TestName, 'vary memory size') & data.RecordCount == record_sizes(i), :);
%     bars_data(i, 1) = workload.CacheSize;
%     bars_data(i, 2) = workload.CurrentBufferSize * 4096;
%     hold on;
% end
% 
% bar(bars_data);
% grid on;
% set(gca, 'XTickLabel', {'0.0002M', '','0.002M', '','0.02M', '','0.2M', '','2M'});
% t = 'Memory Size for different numbers of records';
% xlabel('Number of records');
% legend(memory_instances, 'Location', 'southeast'); % Create legend using the distribution names
% ylabel('Size [Bytes]');
% % Get current yticks
% yticks = get(gca,'ytick');
% 
% % Generate new labels
% yticklabels_values = arrayfun(@(x) sprintf('%.0fM', x / 1e6), yticks, 'UniformOutput', false);
% 
% % Set new labels
% set(gca, 'yticklabel', yticklabels_values);
% 
% matlab2tikz([t '.tex'], 'extraAxisOptions','scaled y ticks=false', 'showInfo', false);
% 
% hold off


%% Vary memory distribution


%memory_distribution = [131072000, 224000; 262144000, 192000; 524288000, 128000; 786432000, 64000; 917504000, 32000];
test = data(strcmp(data.TestName, 'vary memory distribution'), :);
r_tree = unique(test.RadixTreeSize);
b_tree = unique(test.BufferSize);
memory_distribution = [r_tree, b_tree];
memory_distribution_label = {'0:1', '1:7', '2:6', '1:1', '6:2', '7:1'};
bars_data = zeros(length(memory_distribution), length(workloadNames));
figure;
set(gca,'YTickLabelMode','manual')  % Add this line
for j = 1:length(memory_distribution)
    workload = data(strcmp(data.TestName, 'vary memory distribution') & data.RadixTreeSize == memory_distribution(j, 1), :);

    bars_data(j, :) = workload.Throughput;
end

bar(bars_data);
grid on;
ylim([0 max(bars_data(:))*1.1]);
set(gca, 'XTickLabel', memory_distribution_label);
t = 'Memory distribution between cache and buffer';
xlabel('Distribution ratio');
legend(workloadNames, 'Location', 'southeast'); % Create legend using the distribution names
ylabel('Throughput [Operations / s]');
% Get current yticks
yticks = get(gca,'ytick');

% Generate new labels
yticklabels_values = arrayfun(@(x) sprintf('%.0fK', x / 1e3), yticks, 'UniformOutput', false);

% Set new labels
set(gca, 'yticklabel', yticklabels_values);
hold off
matlab2tikz([t '.tex'], 'extraAxisOptions', {'scaled y ticks=false','scaled x ticks=false'}, 'showInfo', false);

% %% Speed comparison 
% 
% coefficients_table = data(strcmp(data.TestName, 'speed comparison'), :);
% coefficients = coefficients_table.Coefficient;
% coefficients = sort(unique(coefficients)); 
% 
% bars_data = zeros(length(workloadNames), 2);
% figure;
% set(gca,'YTickLabelMode','manual')  % Add this line
% for i = 1:length(workloadNames)
%     for c = 0:1
%         workload = data(strcmp(data.TestName, 'speed comparison') & data.Workload == i & data.Cache == c, :);
%         disp(workload)
%         bars_data(i, c+1) = workload.Throughput;
%     end
% end
% 
% bar(bars_data);
% grid on;
% set(gca, 'XTickLabel', num2str(coefficients));
% t = 'Speed comparison for varying p';
% xlabel('p');
% legend({'Not Cached', 'Cached'}, 'Location', 'southeast'); % Create legend using the distribution names
% ylabel('Throughput [Operations / s]');
% % Get current yticks
% yticks = get(gca,'ytick');
% 
% % Generate new labels
% yticklabels_values = arrayfun(@(x) sprintf('%.0fK', x / 1e3), yticks, 'UniformOutput', false);
% 
% % Set new labels
% set(gca, 'yticklabel', yticklabels_values);
% hold off
% matlab2tikz([t '.tex'], 'extraAxisOptions','scaled y ticks=false', 'showInfo', false);



%% Geometric 
p_values = [0.0009, 0.009, 0.09, 0.9];
x = 0:50;  % range of x values

figure; hold on;  % create a figure and hold the plot

% loop over each value of p
for i = 1:length(p_values)
    p = p_values(i);
    y = pdf('Geometric', x, p);
    plot(x, y, 'LineWidth', 2);
end

xlabel('k');
ylabel('P(X=k)');
legend('p = 0.0009', 'p = 0.009', 'p = 0.09', 'p = 0.9');
title('Geometric Distribution for Different p Values');
grid on;
t = ['Geometric Distribution for Different p Values' '.tex'];
matlab2tikz([t '.tex'], 'extraAxisOptions', {'scaled y ticks=false','scaled x ticks=false'}, 'showInfo', false);
