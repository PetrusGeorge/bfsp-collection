echo "--RAIS Benchmark--"

meson setup build

ninja -C build

for instance_folder in instances/*; do
	echo $instance_folder >> ./output2.txt

  for instance in $instance_folder/*; do
    echo "Running $instance" 

    echo "$instance" >> ./output2.txt 
    ./build/rais ${instance} | awk "{print $1}" >> ./output2.txt

  done
done

echo "-" >> ./output2.txt