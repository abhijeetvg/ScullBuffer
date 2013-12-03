Compiling and installing the scullpipe devices:
--------------------------------------------------
1. Execute Make: 
   make
2. Run install:
   ./install.sh
   This unloads previous driver, loads new driver and assigns permissions 777 to the devices.
   NOTE: You might be asked root password as sudo command is used in the shell script.

Testing the functioning of scull pipe devices:
-------------------------------------------------
- Run following shell file to run the consumer:
  ./installRun.sh
  This command compiles producer and consumer files and runs them. 
  Note: 1. Execution ends only when "DONE!! Hit enter!" is logged on console.
        2. The producer and consumer files work for NITEMS = 4. In general, driver can take and respect any value while installation.
- Debug statements will appear on the console which will demonstrate the behavior.
- Alternatively 'dmesg /proc/kmsg' can be run to see the log statements. But, console debug statements demonstrate well.
- To just unload the driver, following command can be used:
  ./scull_unload
