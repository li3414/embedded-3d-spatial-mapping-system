port = "COM4";          %set the serial port according to my computer
baudrate = 115200;      %set the baud rate

%-----open serial port-----
device = serialport(port, baudrate); %open the connection between MATLAB and the microcontroller
device.Timeout = 10; %set the maximum time that MATLAB will wait for reading the data
configureTerminator(device, "CR/LF"); %it means to carriage return and create new line after each UART transmission
flush(device); %clear the old data

fprintf("Opened %s at %d baud\n", port, baudrate); %tell the user the settings
fprintf("Press reset first\n"); %guide user's action
fprintf("Then press PJ0 on the board to start the scan\n");

%-----read the UART data-----
gotHeader = false; %create a faalse flag first
data = []; %create an empty matrix

while true %create a infinite loop
    try
        rawLine = readline(device); %try to read one full line
    catch
        continue %back to the beginning of the infinite loop if timeout
    end

    line = strtrim(string(rawLine)); %convert the serial data into a MATLAB string

    if strlength(line) == 0 
        continue %back to the beginning of the infinite loop is the data is empty
    end

    disp(line) %print the data in the command window

    % wait until the actual data header appears
    if ~gotHeader
        if strcmp(line, "scan,x_axis_pos,angle_deg,distance_mm")
            gotHeader = true; %set to ture if the head is found
            fprintf("Header found. Reading scan data...\n"); %print a message to tell the user the current stage
        end
        continue
    end

    % stop when transmission is finished
    if strcmp(line, "scan_done")
        fprintf("Finished receiving data.\n"); %print a message to tell the user the current stage
        break
    end

    % read one line of numeric data
    values = sscanf(line, '%f,%f,%f,%f');

    if numel(values) == 4 %if four numbers are found add the data to the new row with all columns
        data(end+1, :) = values.'; %#ok<SAGROW>
    end
end

clear device; %clear the serial port after receiving all the data

%-----check and split the data collected-----
if isempty(data)
    error("No valid data was received."); %show an error if nothing is received
end

scan_num = data(:,1); %store the data in column 1 as the scan number
x_mm = data(:,2); %store the data in column 2 as the x displacement for each measurement in mm
angle_deg = data(:,3); %store the data in column 3 as the angles for each measurement in degreee
distance_mm = data(:,4); %store the data in column 4 as the measured distance value in mm

%-----convert the data in 3D point-----
x_positions = x_mm / 1000; %change the unit of the x displacement in m
r = distance_mm / 1000; %change the unit of the measured distance in m
theta = deg2rad(angle_deg); %change the angle data from degree to radian

Y = r .* cos(theta); %get the y coordinate using catesian formula
Z = r .* sin(theta); %get the z coordinate using catesian formula
X = x_positions; %x coordinate is equal to the x displacement

%-----prepare for 3D plot-----
scan_ids = unique(scan_num, "stable"); %get the scan number in order
num_positions = numel(scan_ids); %count the number of scans that take place

%-----create the figure-----
figure;

%-----plot the scatter diagram-----
subplot(1,2,1) %plot in the left
hold on %allow multiple scans to be drawn in same axis

for p = 1:num_positions %loop which allows to draw every scan plane
    idx = (scan_num == scan_ids(p)); %check the rows which belongs to the current scan
    scatter3(X(idx), Y(idx), Z(idx), 3, 'b', 'filled'); %plot the point
end

%label the axis and the title
xlabel('X displacement (m)')
ylabel('Y (m)')
zlabel('Z (m)')
title('scatter3')
grid on
axis equal %make the scale equal
view(35,25) %set the viewpoint angle

%-----draw the scan planes with connected lines-----
subplot(1,2,2) %plot at the right half
hold on

%draw each scan plane with closed line first
for p = 1:num_positions %go through every scan plane
    idx = find(scan_num == scan_ids(p)); %get the rows belong to this plane

    %sort points by angleto make sure the ring is drawn correctly
    [~, order] = sort(angle_deg(idx));
    idx = idx(order);

    %repeat the first point to create a closed loop
    x_loop = [X(idx); X(idx(1))];
    y_loop = [Y(idx); Y(idx(1))];
    z_loop = [Z(idx); Z(idx(1))];

    %draw the closed line
    plot3(x_loop, y_loop, z_loop, '-o', ...
        'Color', 'b', ...
        'LineWidth', 1.5, ...
        'MarkerSize', 2, ...
        'MarkerFaceColor', 'b');
end

%connect the same angle points between neighboring scans
for p = 2:num_positions %start from the second plane which connected to the previous plane
    idx1 = find(scan_num == scan_ids(p-1)); %get the index for the neighboring scans
    idx2 = find(scan_num == scan_ids(p));

    [~, order1] = sort(angle_deg(idx1)); %sort the points by angle
    [~, order2] = sort(angle_deg(idx2));

    idx1 = idx1(order1);
    idx2 = idx2(order2);

    n = min(numel(idx1), numel(idx2));

    for a = 1:n
        xc = [X(idx1(a)), X(idx2(a))]; %take one point from scan 1 and the point with the same angle data in scan 2
        yc = [Y(idx1(a)), Y(idx2(a))];
        zc = [Z(idx1(a)), Z(idx2(a))];

        plot3(xc, yc, zc, '--', 'Color', [0.7 0.7 0.7], 'LineWidth', 0.5); %plot the line between two points
    end
end

%label the axis and the title
xlabel('X displacement (m)')
ylabel('Y (m)')
zlabel('Z (m)')
title('scan planes connected with lines')
grid on
axis equal %make the scale equal
view(35,25) %set the viewpoint angle
