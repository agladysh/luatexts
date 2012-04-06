#! /bin/bash

command -v strace >/dev/null 2>&1 || { echo >&2 "apt-get install strace"; exit 1; }

echo "`date` REPRODUCE BEGIN" | tee -a reproduce.log

iteration="0"
errors="0"

while [ $errors -lt 100 ]; do
  iteration=$[ $iteration + 1 ];

  if
    ! strace -o strace.log -e trace=open ./luajit ../../etc/replay.lua data 1 2>error.log 1>/dev/null
  then
    errors=$[ $errors + 1 ];

    echo "`date` ERROR ${errors} BEGIN (iteration $iteration)" | tee -a reproduce.log

    echo "`date` strace:" | tee -a reproduce.log

    tail -n1 strace.log | tee -a

    echo "`date` stderr:" | tee -a reproduce.log

    cat error.log | tee -a reproduce.log

    echo "`date` ERROR ${errors} END" | tee -a
  fi
done

echo "`date` REPRODUCE END" | tee -a
