pid=`pgrep dungeon_dave`
sample $pid 20 -file sample.txt

call_tree_stuff() {
	filtercalltree sample.txt -invertCallTree -chargeSystemLibraries 
}

output=`call_tree_stuff`
echo "$output" > sample.txt
