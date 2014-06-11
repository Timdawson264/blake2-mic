
#CFLAGS=$CFLAGS\ -DDEBUG

#SSE Version
icc -std=gnu99 ref/b2sum.c ref/blake2s-sse.c -o b2sum_sse $CFLAGS
#Referance Version
icc -std=gnu99 ref/b2sum.c ref/blake2s-ref.c -o b2sum_ref $CFLAGS
#My Phi Implementation
icc -std=gnu99 mic/b2sum-mic.c mic/blake2s-mic.c -o b2sum_mic $CFLAGS

~/sde-external-6.22.0-2014-03-06-lin/sde64 -knl -- ./b2sum_mic $@  > b2sum_mic.log
./b2sum_sse $@ > b2sum_sse.log
./b2sum_ref $@ > b2sum_ref.log

#Side by Side copare
diff b2sum_sse.log b2sum_mic.log -y | less    

tail -n4 *.log

rm b2sum_mic b2sum_sse b2sum_ref
