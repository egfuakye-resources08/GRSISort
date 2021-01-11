TO FIX THE CRUSH IN GRSIPROOF which is SEGMENTATION ERROR; 
FOLLOW THE FOLLOWING STEPS 
STEP 1

sudo apt-get update
sudo apt-get install valgrind

STEP 2 

G_SLICE=always-malloc G_DEBUG=gc-friendly  valgrind -v --tool=memcheck --leak-check=full --num-callers=40 --log-file=valgrind.log grsiproof Name-of-fragment-file  EnergyFragmentSelector.C






