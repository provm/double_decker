if mkdir ../collected/$1 ; then
        cp ../*.* ../collected/$1
        rm ../collected/$1/collector.py
        cp ../../run_experiment.sh ../collected/$1/
else
        echo "ERROR: Directory Exists"
fi
