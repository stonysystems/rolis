import sys
import os


def mk_csv_file(filename_to_write, file_to_read):
    print '----- START -----'
    lines_to_write = []
    with open(file_to_read) as fs:
        for each in fs.readlines():
            if 'XXX,' in each:
                lines_to_write.append(each)

    with open(filename_to_write, "w") as fz:
        fz.writelines(lines_to_write)
    print '----- END -------'


def main(file_path, prefix):
    if not os.path.exists(prefix):
        os.makedirs(prefix)
    for each in os.listdir(file_path):
        if '.txt' in each:
            target_file_name = os.sep.join([prefix, each+".csv"])
            mk_csv_file(target_file_name, each)


if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
    # main(".", "/Users/mrityunjaykumar/USA/SBU/Summer2019/CSE 523/py/Out/")
