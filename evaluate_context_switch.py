#!/usr/bin/env python3

def min_distance(a: list, b: int):
    # find minimum distance between a and b
    min_dist = float('inf')
    for i in range(len(a)):
        if a[i] == b:
            return 0
        if abs(a[i] - b) < min_dist:
            min_dist = abs(a[i] - b)
    return min_dist

def rem_empty_strings(a: list):
    # remove empty strings from a
    for i in range(len(a)):
        # print(i)
        if a[i] == '' or a[i] == ' ' or a[i] == '\n' or a[i] == '\t' or a[i] == '\r':
            a.pop(i)
            i -= 1
    return a

with open('thread1.csv', 'r') as f1:
    thread1 = f1.readlines()
    thread1 = rem_empty_strings(thread1)
    print(thread1)
    for i in range(len(thread1)):
        thread1[i] = int(thread1[i].strip())

with open('thread2.csv', 'r') as f2:
    thread2 = f2.readlines()
    thread2 = rem_empty_strings(thread2)
    print(thread2)
    for i in range(len(thread2)):
        thread2[i] = int(thread2[i].strip())

minimum = float('inf')
for i in range(len(thread2)):
    dist = min_distance(thread1, int(thread2[i]))
    if dist < minimum:
        minimum = dist
        print(minimum)
print(minimum)