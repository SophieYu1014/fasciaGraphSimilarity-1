import os


def get_pair(line):
    line = line.strip()
    d_arr = line.split("\t")
    return list(map(lambda x: int(x), d_arr))


def populate_normalization_dict(filenames):
    all_values = set()
    for fname in filenames:
        f = open(fname, "r")
        for line in f:
            if line.startswith("#"):
                continue
            pair = get_pair(line)
            assert(len(pair) == 2)
            all_values.add(pair[0])
            all_values.add(pair[1])
        f.close()
    print("Found {} unique BGP identifiers".format(len(all_values)))
    vlist = list(all_values)
    vlist.sort()
    return { v: k for k, v in enumerate(vlist) }


def transform_file(fname, ndict):
    f = open(fname, "r")
    print("Transforming {}".format(fname))
    transformed_pairs = []
    for line in f:
        if line.startswith("#"):
            continue
        pair = get_pair(line)
        tp = [ndict[pair[0]], ndict[pair[1]]]
        if tp[0] > tp[1]:
            tp = [tp[1], tp[0]]
        transformed_pairs.append(tp)
    transformed_pairs.sort(key=lambda p: p[0])
    print("Found total of {} edges".format(len(transformed_pairs)))
    tpstr = list(map(lambda p: ' '.join(map(str, p)), transformed_pairs))
    tpstr = list(dict.fromkeys(tpstr))
    print("Total of {} edges remain after removing duplicates".format(len(tpstr)))
    write_file(fname, tpstr, len(ndict))


def write_file(fname, pairs, ncount):
    destination = fname.replace(".txt", "_transformed.graph")
    f = open(destination, "w")
    f.write("{}\n".format(ncount))
    f.write("{}\n".format(len(pairs)))
    for pair in pairs:
        f.write("{}\n".format(pair))


def main():
    filenames = []
    for f in os.listdir("."):
        if f.endswith(".txt"):
            filenames.append(f)
    normalized_dict = populate_normalization_dict(filenames)
    for f in filenames:
        transform_file(f, normalized_dict)


main()
