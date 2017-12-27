

with open("../TestPipelinePlannerCLRCPP/TestPipelinePlannerCLRCPP/testOut/testOut__00001.txt", "w+") as f:

    b = 0

    f.write("W\t" + str(b) + "\t3\t0\n")
    b += 100
    for i in range(100):
        f.write("W\t" + str(b) + "\t2\t1\n")
        f.write("W\t" + str(b+1) + "\t2\t0\n")
        b += 100

    f.write("W\t" + str(b) + "\t5\t0\n")
    for i in range(100):
        f.write("W\t" + str(b) + "\t4\t1\n")
        f.write("W\t" + str(b+1) + "\t4\t0\n")
        b += 100

    f.write("W\t" + str(b) + "\t3\t1\n")
    for i in range(100):
        f.write("W\t" + str(b) + "\t2\t1\n")
        f.write("W\t" + str(b+1) + "\t2\t0\n")
        b += 100

    f.write("W\t" + str(b) + "\t5\t1\n")
    for i in range(100):
        f.write("W\t" + str(b) + "\t4\t1\n")
        f.write("W\t" + str(b+1) + "\t4\t0\n")
        b += 100
