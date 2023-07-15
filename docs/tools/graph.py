import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("benchmarks.csv")

#df = pd.DataFrame({'lab':['A', 'B', 'C'], 'val':[10, 30, 20]})

#ax = df.plot.bar(x='languages', y='fibonacci', rot=0)

df.plot.bar(x="languages", y="fibonacci", rot=0)

df.plot.bar(x="languages", y="factors", rot=0)


plt.show()
