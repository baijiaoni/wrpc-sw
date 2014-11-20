wrpc-sw
=======

Used for the switch Latency

The order is: 
1st: Connet two exploder to the switch.
2nd: Load firmware to both exploder, and wait for their synchronization.
3rd: Plug optical fibers connecting vetar cards into the switch.
4th: Use command "d" to make the source exploder to produce latency measurement packet.


Problem:
1. Roload the firmware to the exploder will not work. Because the ep_filter can not filter the broadcast packet from LM32.
2. 
->Make config
  Compile Etherbone support in wrpc-sw (ETHERBONE) [Y/n/?] Y
  
  If set to "n", the source exploder will lost synchronization after about 1 min.
