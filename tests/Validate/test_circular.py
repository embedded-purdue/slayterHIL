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

#def ascent_phase(timedict):
    # test to see if changes in x or y position happens during ascent (it should not)

    # test to see if Z_velocity is consisent throughout

    # print out the average ascent phase velocity for x, y, z

# def circular_motion(timedict):
    # test to see if changes in the z position happens during the circling (it should not)

    # test something about the circle

    # print out the averge circular_motion velocity for x, y, z

# def descent_phase(timedict):
    # test to see if x and y velocity are always 0 
    # test to see consistency of Z_velocity
    # print out the average descent_phase velocity for x, y, z



run_tests()



