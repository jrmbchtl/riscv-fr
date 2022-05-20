#!/usr/bin/env python3

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

with open('montmul.csv', 'r') as f:
    montmul = f.readlines()


montmul_data = []

for i in range(len(montmul)):
    try:
        montmul_data.append(int(montmul[i].strip()))
    except:
        pass

# drop avalues < 0.5*10^8 and > 0.7^8
# print(len(montmul_data))
# montmul_data = [x for x in montmul_data if x > 15000000 and x < 70000000]
# print(len(montmul_data))

mul_data = pd.DataFrame(montmul_data, columns=['montmul'])
ax = mul_data.plot(kind='hist', bins=100, title='montmul')
# ylim=(0, 500)
plt.show()