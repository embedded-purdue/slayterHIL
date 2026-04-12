# file that checks the accuracy of the circle test with ../flightpaths/circular_test.json

# info for whoever reads this: the initialization is 1) checking valid file, 2) uploading info to dictionaries.
# then I will make more specific tests for the phases of the descent (for this one, it is ascent, circle, descent)

def run_tests():
    file_path = '../generateTests/JSONtests/circular_test.json'
    with open(file_path, 'r') as file:
        lines = file.readlines()

    #TODO add testing that checks for accuracy of file format ... implement after we see the actual file

# first line is [ an d last line is ]
# then, you have one "{", and then after that is 8 lines of:
    #"Message_id": 1,
    #"Timestamp": 0.0,
    #"X_pos": 10.0,
    #"Y_pos": 0,
    #"Z_pos": 0.0,
    #"X_vel_ext": 0,
    #"Y_vel_ext": 0,
    #"Z_vel_ext": 25.0
# then, you have a line of "},"

    timestampdict = {}

    # for a key in timestampdict corresponding to the timestamp, the values is a list of:
    # [message_id, timestamp, x_pos, y_pos, x_vel_ext, y_vel_ext, z_vel_ext]

    index = 0
    for i in lines:
        if index % 10 == 2:
            # use timestamp as a key to make a dictionary entry for the new block of info
            timing = lines[index+1].split(": ")[1]
            timestamp = timing.split(",")[0]
            timestampdict[timestamp] = []
            count = 0
            llist = []
            # add everything to the dictionary that corresponds to the block of info
            for count in range(0, 8):
                newindex = index + count
                infoval = lines[newindex].split(": ")[1].split(",")[0].strip()
                llist.append(infoval)
            timestampdict[timestamp] = llist 
        index = index + 1
    
    # call the tests for the ascent_phase(), descent_phase(), timestamp()
    ascent_phase(timestampdict)

def ascent_phase(timedict):

    total_x_velocity = 0
    total_y_velocity = 0
    total_z_velocity = 0

    # test to see if changes in x or y position happens during ascent (it should not)
    # currently, i have the variable for the time to ascend being constant (3). But if i want, i could easily change that.
    i = 0.5
    ascent_time = 3.0
    curr_x = timedict['0.0'][2]
    curr_y = timedict['0.0'][3]
    while i < ascent_time:
        next_x = timedict[str(i)][2]
        next_y = timedict[str(i)][3]
        changed = 0
        if curr_x != next_x:
            print(f"TEST FAILED: change in x position detected during ascent phase at timestamp {i}.")
            print(f"x distance changed to {next_x}, was supposed to be {curr_x}")
            changed = 1
        if curr_y != next_y:
            print(f"TEST FAILED: change in y position detected during ascent phase at timestamp {i}.")
            print(f"x distance changed to {next_y}, was supposed to be {curr_y}")
            changed = 1
        if changed == 1:
            break
        i = i + 0.5
    if changed == 0:
        print("Test passed! No changed in x and y direction detected during ascent.")

    # test to see if Z_velocity is consisent throughout
    i = 0.5
    ascent_time = 3.0
    curr_z_velocity = timedict['0.0'][7]
    while i < ascent_time:
        next_z_velocity = timedict[str(i)][7]
        changed = 0
        if curr_x != next_x:
            print(f"TEST FAILED: change in z position detected during ascent phase at timestamp {i}.")
            print(f"z velocity was supposed to be {curr_z_velocity}, was supposed to be {next_z_velocity}")
            changed = 1
        if changed == 1:
            break
        i = i + 0.5
    if changed == 0:
        print("Test passed! No changed in z velocity detected during ascent.")

    # print out the average ascent phase velocity for x, y, z
    j = 0.0
    timestampct = 0
    while j < ascent_time:
        total_x_velocity = total_x_velocity + float(timedict[str(j)][5])
        total_y_velocity = total_y_velocity + float(timedict[str(j)][6])
        total_z_velocity = total_z_velocity + float(timedict[str(j)][7])
        j = j + 0.5
        timestampct = timestampct + 1

    print("Ascent tests done.")
    print(f"Average x velocity: {total_x_velocity / timestampct}")
    print(f"Average y velocity: {total_y_velocity / timestampct}")
    print(f"Average z velocity: {total_z_velocity / timestampct}")

# def circular_motion(timedict):
    # test to see if changes in the z position happens during the circling (it should not)

    # test something about the circle

    # print out the averge circular_motion velocity for x, y, z

# def descent_phase(timedict):
    # test to see if x and y velocity are always 0 
    # test to see consistency of Z_velocity
    # print out the average descent_phase velocity for x, y, z



run_tests()



