#!/usr/bin/env python3

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

# read data from square.csv and multiply.csv
with open('square.csv', 'r') as f:
    sq = f.readlines()

with open('multiply.csv', 'r') as f:
    mul = f.readlines()

sq_data = []
mul_data = []

# remove all empty lines and convert to int
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

# draw all data into a histogram
mul_data = pd.DataFrame(mul_data, columns=['multiply'])
ax = mul_data.plot(kind='hist', bins=100, title='multiply')
sq_data = pd.DataFrame(sq_data, columns=['square'])
sq_data.plot(kind='hist', bins=100, ax=ax, title='square')
plt.show()