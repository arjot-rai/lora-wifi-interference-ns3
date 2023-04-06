import numpy as np

data = []
i = 0
with open("temp", "r") as file:
    for line in file:
        if 'client=' in line:
            i=i+1
            sent_str, delivered_str = line.split(";")[0], line.split(";")[1]
            sent_str = sent_str.split(" ")[1:]
            sent_str = ''.join(sent_str)
            sent = float(sent_str.strip().split(":")[1])
            delivered = float(delivered_str.strip().split(":")[1])
            data.append(100*(sent-delivered)/sent)

# create a dataset of random numbers

# calculate the quartiles and median
q1, median, q3 = np.percentile(data, [25, 50, 75])

# calculate the interquartile range (IQR)
iqr = q3 - q1

# calculate the upper and lower bounds for outliers
upper_bound = q3 + 1.5 * iqr
lower_bound = q1 - 1.5 * iqr
outliers = []
# find the outliers in the dataset
for d in data:
    if (d > upper_bound) or (d < lower_bound):
        outliers.append(d)

# print the results
# print("Data: ", data)
print("Quartiles: ", q1, median, q3)
print("IQR: ", iqr)
print("Upper bound: ", upper_bound)
print("Lower bound: ", lower_bound)
print("Outliers: ", outliers)