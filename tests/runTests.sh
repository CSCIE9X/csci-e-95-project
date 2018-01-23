#!/bin/bash

TEST_DIRS="scanner parser symbol"

rm -f error.log

for dir in $TEST_DIRS

    do
    # ---------------------------------------------------------
    # Paths related to the course compiler and input files
    # Root to where the input files live
    PROJECT_ROOT=".."
    # The input test files for grading
    INPUT_FILES="$PROJECT_ROOT/tests/$dir/input/*.txt"
    # The expected output files
    EXPECTED_FILES_PATH="$PROJECT_ROOT/tests/$dir/expected"
    # Command to invoke the compiler
    COMPILER_EXEC="$PROJECT_ROOT/src/compiler/compiler -s $dir"
    # ---------------------------------------------------------

    # ---------------------------------------------------------
    # output directory for the student's compiler output
    OUTPUT_FILES="$dir/$dir-output"
    mkdir -p $OUTPUT_FILES
    rm -f "$OUTPUT_FILES/*"


    errorcount=0
    filecount=0

    for f in $INPUT_FILES
    do
      name=${f##*/}
      #echo "$name"
      output="$OUTPUT_FILES/$name"
      expected="$EXPECTED_FILES_PATH/$name"
      #echo "scanning $name to $output "
      $COMPILER_EXEC <$f >$output
      ((filecount++))
      if diff $output $expected >/dev/null; then
        rm -f $output
      else
        ((errorcount++))
        echo "error with $f"
        echo "+++++++++++++++++++++++++++++++++++++++++++++++" >> error.log
        echo "failed test case $name" >> error.log
        echo "---------------------------" >> error.log
        echo "test case source" >> error.log
        echo "---------------------------" >> error.log
        cat $f >> error.log
        echo >> error.log
        echo "===========================" >> error.log
        echo "actual output" >> error.log
        echo "===========================" >> error.log
        cat $output >> error.log
        echo >> error.log
        echo "***************************" >> error.log
        echo "expected output" >> error.log
        echo "***************************" >> error.log
        cat $expected >> error.log
      fi
    done
    echo "$dir has $errorcount errors out of $filecount"
    if test "$errorcount" -eq "0"
    then
        rm -r $OUTPUT_FILES
    fi

done
