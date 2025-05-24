import subprocess
import sys
import os.path
from os import listdir
from concurrent.futures import ThreadPoolExecutor, as_completed

def process_iteration(binary_path, instance_name, instance_size, iteration):

    destination_base = f"results/{instance_size}/{instance_name}/{iteration}.txt"

    os.makedirs(os.path.dirname(destination_base), exist_ok=True)

    with open(destination_base, "w") as f:
        subprocess.run([f"./{binary_path}", "-b", f"instances/{instance_size}/{instance_name}"], stdout=f)

    return destination_base

def main(binary_path):
    num_threads = 10  # Defina o número de threads no pool
    instances_sizes = ["J20M5", "J20M10", "J20M20", "J50M5", "J50M10", "J50M20", "J100M5", "J100M10", "J100M20", "J200M5", "J200M10", "J200M20", "J500M5", "J500M10", "J500M20",]
    os.makedirs("results", exist_ok=True)
    with open("results/.gitignore", "w") as gitignore:
        gitignore.write("*")
    for size in instances_sizes:
        print(f"SIZE: {size}")
        instances = [f for f in listdir(f"instances/{size}/")]
        instances.sort()
        with ThreadPoolExecutor(max_workers=num_threads) as executor:
            futures = [executor.submit(process_iteration, binary_path, instance, size, i) for instance in instances for i in range(num_threads)]             
            for future in as_completed(futures):
                result = future.result()
                print(f"Tarefa concluída:{result}")
 
if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"run with: python {sys.argv[0]} /path/to/binary")
        exit(1)
    main(sys.argv[1])

