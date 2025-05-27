echo "--MA Benchmark--"

meson setup build

ninja -C build

clear

for instance_folder in instances/*; do
	echo $instance_folder >> ./output.txt

  for instance in $instance_folder/*; do
    echo "Running $instance" 

    echo "$instance" >> ./output.txt 
    ./build/ma ${instance} | awk "{print $1}" >> ./output.txt

  done
done

echo "-" >> ./output.txt