import pandas as pd
import matplotlib.pyplot as plt
import sys

# TODO : make autosaving files work
# print(f"Arguments of the script : {sys.argv[1]=}")

is_saving = False

if len(sys.argv) > 1:
    if sys.argv[1] == "--save":
        is_saving = True

df = pd.read_csv("benchmarks.csv")

#df = pd.DataFrame({'lab':['A', 'B', 'C'], 'val':[10, 30, 20]})

#ax = df.plot.bar(x='languages', y='fibonacci', rot=0)

df.plot.bar(x="languages", y="fibonacci", rot=0)

if is_saving:
    plt.savefig("fibonacci-saved.png", pad_inches=10)

df.plot.bar(x="languages", y="factors", rot=0)

if is_saving:
    plt.savefig("factors-saved.png")


plt.show()