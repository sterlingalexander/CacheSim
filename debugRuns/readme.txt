4 debug runs have been done using a smaller trace file 'canneal.04t.debug' which is attached alongwith the debug runs outputs 

The commands are as follow

./smp_cache 32768 8 64 4 0 canneal.04t.debug > firefly1.debug
./smp_cache 32768 8 32 4 0 canneal.04t.debug > firefly2.debug
./smp_cache 65536 8 64 4 0 canneal.04t.longTrace > firefly3.debug
./smp_cache 32768 8 32 4 1 canneal.04t.debug > dragon2.debug
./smp_cache 32768 8 64 4 1 canneal.04t.debug > dragon1.debug
./smp_cache 65536 8 64 4 1 canneal.04t.longTrace > dragon3.debug

