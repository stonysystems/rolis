import os.path
from os import path
import sys


if __name__ == "__main__":

    params = sys.argv

    skip_index = int(params[1]) 
    file_pattern = "./" + params[2] + "/leader-{num}-1000.log"
    search_keyword = params[3]


    for trd in range(1, 32):
        f = file_pattern.format(num=trd)
        value = ""
        if path.exists(f):
            with open(f) as handler:
                lines = handler.readlines()
                for line in lines:
                    if search_keyword in line:
                        value = line.replace("\n", "")
                        # handle the selected line
                        for i in range(4, len(params)):
                            value = value.replace(params[i], "")
                        value = value.replace(search_keyword, "")
                        break
        print(value if skip_index else "{index}\t{value}".format(index=trd, value=value))
