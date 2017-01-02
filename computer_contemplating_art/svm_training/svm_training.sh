#! /usr/bin/bash

MAX_NUM_PROCESSES=6

DATA_DIR=../data/
TMP_DIR=./tmp/
LIBSVM_DIR=/d/libsvm/windows/

mkdir -p $TMP_DIR

cat $DATA_DIR/positive_train.txt > $TMP_DIR/positive_train.txt
num_positive=`wc -l $TMP_DIR/positive_train.txt | awk '{print $1}'`
num_negative=$((1 * $num_positive))
head -$num_negative $DATA_DIR/fail_ifs_features.txt \
    > $TMP_DIR/negative_train.txt
cat $TMP_DIR/positive_train.txt $TMP_DIR/negative_train.txt \
    > $TMP_DIR/train.txt
echo
echo Using $num_positive positive and $num_negative     negative samples
echo
echo

echo
echo Scaling training samples
echo
echo
$LIBSVM_DIR/svm-scale.exe -l 0 -u +1 -s $TMP_DIR/scale.txt $TMP_DIR/train.txt \
    > $TMP_DIR/scaled_train.txt

echo
echo Training libsvm machine
echo
echo
num_processes=0
for log_g in -9 -8 -7 -6 -5 -4 -3; do
    for log_c in 1 2 3 4 5 6 7 8; do
	if [ $num_processes != $MAX_NUM_PROCESSES ]; then
	    result=$TMP_DIR/result_${log_g}_${log_c}.txt
	    rm -f $result
            echo log_g $log_g log_c $log_c | tee -a $result
            $LIBSVM_DIR/svm-train.exe -s 0 -t 2 -g 1.0e$log_g -c 1.0e$log_c -v 5 \
                $TMP_DIR/scaled_train.txt $TMP_DIR/svm_model.bin \
                | grep "Cross Validation" | tee -a $result &
	    num_processes=$(($num_processes + 1))
	else
	    wait
	    num_processes=0
	fi
    done
done
