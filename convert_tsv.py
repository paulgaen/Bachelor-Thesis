#used to convert datasets obtained from https://snap.stanford.edu/data/index.html into a format useable by adj_list
#the hash function is used to turn the string of source and destination into an int
#the conversion doesn't seem to be perfect but it's good enough for now
#timestamp is shortened to just year for now
#was tested with the reddit dataset

with open("C:/Users/paulg/OneDrive/Desktop/Bachelor/tempus/small-dataset.txt", "w") as writer:   #output file
    with open("C:/Users/paulg/OneDrive/Desktop/Bachelor/tempus/CollegeMsg.txt", "r") as reader:   #input file
        for line in reader.readlines():
            templine = line.replace('\t', ' ')
            source = templine.split(' ')[0]
            destination = templine.split(' ')[1]
            time = templine.split(' ')[2]
            #time = templine.split(' ')[3] + "T" + templine.split(' ')[4]
            writer.write("add " + source + " " + destination + " " + time)