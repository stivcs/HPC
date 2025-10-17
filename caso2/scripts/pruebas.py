import os

for root, dirs, files in os.walk("results"):
    for file in files:
        if file.endswith(".csv"):
            print(os.path.join(root, file))