#!/bin/bash

RESULT_CODE_RESTART=8

result=$RESULT_CODE_RESTART
while [ $result == $RESULT_CODE_RESTART ]; do
    awesome
    result=$?
done
exit $result
