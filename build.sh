project_name=tetmis

sources=$(find src -name "*.c")
headers=$(find src -name "*.h")
rebuild=false
objects=
program="build/$project_name"
do_execute=false
do_linking=false
do_debug=false
output=

common_flags="-g"
linker_libs="-lncursesw"

options=$(getopt --options=Bf:go:t:x --longoptions=clean,test:,execute,debug,trace --name "$0" -- "$@")
[ $? = 0 ] || exit 1

mkdir -p build/tests build/src || exit

eval set -- "$options"

while [ $# -ne 0 ]
do
	case "$1" in
	-B)
		rebuild=true
		shift
		;;
	--clean)
		rm -r build
		exit
		;;
	-f)
		common_flags="$common_flags -f$2"
		shift 2
		;;
	-g|--debug)
		[ ! -z "$program" ] && program=build/$project_name
		do_debug=true
		shift
		;;
	-o)
		output="$2"
		shift 2
		;;
	-t|--test)
		sources="${sources/'src/main.c'/}"
		sources="$sources tests/$2.c"
		if [ -f tests/test.c ] && [ ! "$2" = "test" ]
		then
			sources="$sources tests/test.c"
		fi
		program=build/tests/$2
		shift 2
		;;
	--trace)
		set -o xtrace
		shift
		;;
	-x|--execute)
		do_execute=true
		shift
		;;
	--)
		shift
		break
		;;
	*)
		echo "invalid argument $1"
		exit
	esac
done

# -Ibuild needs to be included so that gcc can find the .gch file
compiler_flags="$common_flags -Werror -Wall -Wextra -Ibuild"
linker_flags="$common_flags"

for h in $headers
do
	if [ src/$project_name.h -nt build/$project_name.h.gch ] ||
		[ $h -nt build/$project_name.h.gch ]
	then
		if [ -z $output ]
		then
			gcc $compiler_flags src/$project_name.h -o build/$project_name.h.gch || exit
		else
			gcc $compiler_flags src/$project_name.h -o build/$project_name.h.gch 2>$output || exit
		fi
		rebuild=true
		break
	fi
done

for s in $sources
do
	o="build/${s:0:-2}.o"
	objects="$objects $o"
	if [ $s -nt $o ] || $rebuild
	then
		if [ -z $output ]
		then
			gcc $compiler_flags -c $s -o $o || exit
		else
			gcc $compiler_flags -c $s -o $o 2>$output || exit
		fi
		do_linking=true
	fi
done

if $do_linking || [ ! -f $project_name ]
then
	if [ -z $output ]
	then
		gcc $linker_flags $objects -o $program $linker_libs || exit
	else
		gcc $linker_flags $objects -o $program $linker_libs 2>$output || exit
	fi
fi

signal_handler() {
	echo "Caught SIGINT!" >&2
}

if $do_debug
then
	if [ -z "$program" ]
	then
		program=$project_name
	fi
	gdb --args ./$program "$@"
elif $do_execute
then
	time_now=$(date "+%s %N")
	read start_seconds start_nanoseconds <<< "$time_now"
	trap signal_handler SIGINT
	./$program "$@"
	exit_code=$?
	time_now=$(date "+%s %N")
	read end_seconds end_nanoseconds <<< "$time_now"
	diff_seconds=$((10#$end_seconds - 10#$start_seconds))
	diff_nanoseconds=$((10#$end_nanoseconds - 10#$start_nanoseconds))
	if [ $diff_nanoseconds -lt 0 ]
	then
		diff_seconds=$((diff_seconds - 1))
		diff_nanoseconds=$((1000000000 + diff_nanoseconds))
	fi
	echo -e "\nexit code: \e[36m$exit_code\e[0m; elapsed time: \e[36m$diff_seconds.$diff_nanoseconds\e[0m seconds" >&2
fi
