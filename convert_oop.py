from collections import namedtuple
import random

# Creating a pair using namedtuple
Pair = namedtuple('Pair', ['first', 'secondt'])

delayPerc = 40 # percentage of delaed data
maxDelay = 40 # maximal lines of delay

with open("C:/Users/paulg/OneDrive/Desktop/Bachelor/tempus/big_40", "w") as writer:  # output file
    with open("C:/Users/paulg/OneDrive/Desktop/Bachelor/tempus/big-dataset", "r") as reader:  # input file
        buffer = []
        for line in reader.readlines():
            if random.randint(1, 100) > (100 - delayPerc):
                buffer.append(Pair(line, random.randint(2, maxDelay)))
            else:
                buffer.append(Pair(line, 1))
            print("before:")
            print(buffer)
            buffer = [(lineBuff, intBuff - 1) for lineBuff, intBuff in buffer]
            print("after:")
            print(buffer)
            for output, count in buffer:
                if count <= 0:
                    writer.write(output)
                    buffer.remove(Pair(output, count))
        while buffer:
            buffer = [(lineBuff, intBuff - 1) for lineBuff, intBuff in buffer]
            for output, count in buffer:
                if count <= 0:
                    writer.write(output)
                    buffer.remove(Pair(output, count))