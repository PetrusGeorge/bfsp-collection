import os
import json

results_dir = "results"

with open('best_ribas.json') as f:
    taillard_best_solutions = json.load(f)

differences = {}

for jxmy_dir in os.listdir(results_dir):
    jxmy_path = os.path.join(results_dir, jxmy_dir)
    if not os.path.isdir(jxmy_path):
        continue

    means = []
    for instance_dir in os.listdir(jxmy_path):
        instance_path = os.path.join(jxmy_path, instance_dir)
        if not os.path.isdir(instance_path):
            continue

        try:
            instance_index = int(instance_dir[instance_dir.find("N")+1:]) - 1
        except ValueError:
            continue

        values = []
        for file in os.listdir(instance_path):
            if file.endswith(".txt"):
                with open(os.path.join(instance_path, file), "r") as f:
                    ro_values = []
                    for value in f.read().strip().split('\n'):
                        ro_values.append(float(value))
                    values.append(ro_values)

        if len(ro_values) != 4:
            print(f"There was an error in instance {instance_path}")

        mean_values = [0,0,0,0]
        for ro_values in values:
            for i,value in enumerate(ro_values):
                mean_values[i] += value / len(values)

        try:
            best_value = taillard_best_solutions[jxmy_dir][instance_index]
        except (KeyError, IndexError):
            continue
        arpd = [((i - best_value)/best_value)*100 for i in mean_values]
        try:
            differences[f"{jxmy_dir}"] = [i + j for i,j in zip(arpd, differences[f"{jxmy_dir}"])]
        except KeyError:
            differences[f"{jxmy_dir}"] = arpd
    num_instances = len(os.listdir(jxmy_path))
    try:
        differences[f"{jxmy_dir}"] = [float(f"{i / num_instances:.2f}") if float(f"{i / num_instances:.2f}") != -0.0 else 0.0 for i in differences[f"{jxmy_dir}"]]
    except KeyError:
        continue
for key in sorted(differences):
    print(f"{key} = {differences[key]}")
