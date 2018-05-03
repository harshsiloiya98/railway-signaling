MARAUDERS
----------

Harsh Siloiya - 160050011
Ashutosh Verma - 160050063
Maitrey Gramopadhye - 160050049
Kshitij Garg - 150050033
Anshu Ahirwar - 150050077

Note :-
-----
The following files should be (re)placed in the corresponding directories :-

board.ucf         -> ~/20140524/makestuff/hdlmake/apps/makestuff/swled/templates/fx2all/boards/atlys
top_level.vhdl    -> ~/20140524/makestuff/hdlmake/apps/makestuff/swled/templates/fx2all/vhdl
harness.vhdl      -> ~/20140524/makestuff/hdlmake/apps/makestuff/swled/templates
main.c            -> ~/20140524/makestuff/apps/flcli
Rest all files in -> ~/20140524/makestuff/hdlmake/apps/makestuff/swled/cksum/vhdl


To compile :-
----------
After (re)placing the given files, from inside the ~/20140524/makestuff/hdlmake/apps/makestuff/swled/cksum/vhdl directory, run the following commands in order :-
(i)   ../../../../../bin/hdlmake.py -t ../../templates/fx2all/vhdl -b atlys -p fpga
(ii)  ../../../../../../apps/flcli/lin.x64/rel/flcli -v 1d50:602b:0002 -i 1443:0007
(iii) ../../../../../../apps/flcli/lin.x64/rel/flcli -v 1d50:602b:0002 -p J:D0D2D3D4:fpga.xsvf
(iv)  sudo ../../../../../../apps/flcli/lin.x64/rel/flcli -v 1d50:602b:0002 -s
(v)   Type in any alphabet and press "Enter"

For C files :-
-----------
After replacing them, just run the command "make deps" from inside that directory. It has to be done only once, unless you edit the C files again.

UART Communication :-
------------------
We have implemented the [controller <-> backend] UART communication.
After the program has reached to the specific state (i.e. the state where UART communication occurs), open up a GTKTERM window beforehand (preferably before the execution starts), change the port to "ttyXRUSB0", and enable hexadecimal input/output.
After the program starts executing, we can provide a hexadecimal input at any time before the UART state (S4) reaches. When we reach state S4, the switch input is sent via UART to the backend controller (PC) and it gets printed on the GTKTERM window. As for the output on the FPGA, that is displayed via the LEDs.