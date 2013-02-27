#!/bin/bash
RESULTS="mGREEDY mrGREEDY mALNS mGRASP"
TOPO=torus
SIZE=16
APPLICATION=RS-32_28_8_dec
if [ "$1" = "Final" ]
then
	unset RESULTS
fi

for s in 1 1.1 1.2 1.3 1.5 2 3 4 5 10 20 30 50 100 200 300 500; do 
	for m in ${RESULTS} ; do
		min=$(cat stat*${TOPO}_${TOPO}_${SIZE}x${SIZE}_${APPLICATION}_s_${s}.xml*${m}* 2>/dev/null | awk '{if (NF==5) print $3}  {if (NF==2) print $2}' | sort -nur | tail -1)
		i=$(cat stat*${TOPO}_${TOPO}_${SIZE}x${SIZE}_${APPLICATION}_s_${s}.xml*${m}* 2>/dev/null | awk '{if (NF==5) print $4}' | sort -nu | tail -1)
		if [ "$i" != "" ]; then
			iter="Iterations "
		fi
		if [ "$min" != "" ]; then
			echo -e "Found $min \tfor ${TOPO}${SIZE}x${SIZE}_${s} ${m}\t$iter$i" 
		else
			if [ "$1" != "-c" ]; then
				echo -e "Pending... \tfor ${TOPO}${SIZE}x${SIZE}_${s} ${m}" 
			fi
		fi
	done
	
	if [ "$1" = "Final" ]
	then
		min=$(cat stat*${TOPO}_${TOPO}_${SIZE}x${SIZE}_${APPLICATION}_s_${s}.xml* 2>/dev/null | awk '{if (NF==5) print $3}  {if (NF==2) print $2}' | sort -nur | tail -1)
		i=$(cat stat*${TOPO}_${TOPO}_${SIZE}x${SIZE}_${APPLICATION}_s_${s}.xml* 2>/dev/null | awk '{if (NF==5) print $4}' | sort -nu | tail -1)
		util=$(cat stat*${TOPO}_${TOPO}_${SIZE}x${SIZE}_${APPLICATION}_s_${s}.xml* 2>/dev/null | grep -ERi Link_util_best | awk '{print $3}' | sort -nu | tail -1)
		if [ "$i" != "" ]; then
			iter="Iterations "
		fi
		if [ "$min" != "" ]; then
			echo -e "Found $min \tfor ${TOPO}${SIZE}x${SIZE}_${s} \t$iter${i} \tLink_util ${util}" 
		else
			if [ "$1" != "-c" ]; then
				echo -e "Pending... \tfor ${TOPO}${SIZE}x${SIZE}_${s}" 
			fi
		fi
	fi
done