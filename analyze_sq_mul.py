#!/usr/bin/env python3

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

with open('square.csv', 'r') as f:
    sq = f.readlines()

with open('multiply.csv', 'r') as f:
    mul = f.readlines()

sq_data = []
mul_data = []

for i in range(len(sq)):
    try:
        sq_data.append(int(sq[i].strip()))
    except:
        pass
for i in range(len(mul)):
    try:
        mul_data.append(int(mul[i].strip()))
    except:
        pass

# plot square data
mul_data = pd.DataFrame(mul_data)
mul_data.plot(kind='hist', bins=1000)
plt.show()
