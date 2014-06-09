


#icc -std=gnu99 blake2s-ref.c ../sse/b2sum.c -o ../sse/b2sum_r0_ref
icc -std=gnu99 /home/tim/blake2/sse/blake2s.c b2sum.c -o b2sum_sse -DHAVE_SSSE3 -DHAVE_SSE41 #-DDEBUG
icc -std=gnu99 blake2s.c b2sum.c -o b2sum_phi -DPHI #-DDEBUG

~/sde-external-6.22.0-2014-03-06-lin/sde64 -knl -mix -- ./b2sum_phi $1  > b2sum_phi.log
./b2sum_sse $1 > b2sum_sse.log
./b2sum_REF -a blake2s $1 > b2sum_REF.log

#Side by Side copare
diff b2sum_sse.log b2sum_phi.log -y | less    

tail -n1 *.log
