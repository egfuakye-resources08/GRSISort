#TO FIX THE CRUSH IN GRSIPROOF which is SEGMENTATION ERROR; 

#1. INSTALL VALGRIND

sudo apt-get update
sudo apt-get install valgrind

#2. HOW TO APPLY VALGRIND ON YOUR PROGRAM

G_SLICE=always-malloc G_DEBUG=gc-friendly  valgrind -v --tool=memcheck --leak-check=full --num-callers=40 

--log-file=valgrind.log grsiproof Name-of-fragment-file  EnergyFragmentSelector.C

#3. CHECK OUT FINALLY

G_SLICE=always-malloc G_DEBUG=gc-friendly  valgrind -v --tool=memcheck --leak-check=full --num-callers=40 
--log-file=valgrind.log grsiproof /home/cern33303/GRSISort/GRSIProof/ExampleEventSelector.C --addback-window 300 fragment10484_???.root 






