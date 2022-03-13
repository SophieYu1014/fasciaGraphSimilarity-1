import os

def process_file(file_name):
    f = open(file_name, "r")
    tups = set()
    count = 0
    maxi = 0
    for line in f:
        if line.startswith("#"):
            continue
        line = line.strip()
        d_arr = line.split("\t")
        currmax = max(int(d_arr[0]), int(d_arr[1]))
        maxi = maxi if maxi > currmax else currmax
        if int(d_arr[0]) > int(d_arr[1]):
            tup = "{} {}".format(d_arr[1], d_arr[0])
        else:
            tup = ' '.join(d_arr)
        if tup in tups:
            print("Found duplicate tup: {}".format(tup))
        else:
            tups.add(tup)
    print("Got {} edges in tup".format(len(tups)))
    write_file(file_name, tups)


def write_file(file_name, tups):
    destination = file_name.replace(".txt", "_generated.graph")
    f = open(destination, "w")
    f.write("65535\n")
    f.write("{}\n".format(len(tups)))
    for tup in tups:
        f.write("{}\n".format(tup))


def does_it_all():
    for f in os.listdir("."):   
        if f.endswith(".txt"):
            process_file(f)

# process_file("oregon1_010414.txt")
does_it_all()
