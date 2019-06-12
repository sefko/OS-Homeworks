#!/bin/bash

logfile=$1
xmlfile=$2
wavfile=$3
outputdir=$4

#check if arguments are more than needed
if [ -n "$5" ]; then
	echo "More than the four needed arguments are entered"
	exit 1
fi

#check if all arguments are present
if [ -z "$logfile" ]; then 
	echo -e "4 arguments are missing\nUsage: 62203.sh file.log file.xml file.wav /output/dir/"
	exit 1
elif [ -z "$xmlfile" ]; then
	echo -e "3 arguments are missing\nUsage: 62203.sh file.log file.xml file.wav /output/dir/"
	exit 1
elif [ -z "$wavfile" ]; then
	echo -e "2 arguments are missing\nUsage: 62203.sh file.log file.xml file.wav /output/dir/"
	exit 1
elif [ -z "$outputdir" ]; then
	echo -e "1 argument is missing\nUsage: 62203.sh file.log file.xml file.wav /output/dir/"
	exit 1
fi

ok=1

#check if files are right format and readable
if (echo "$logfile" | grep -vq "\.log$") || [ ! -f "$logfile" ] || [ ! -r "$logfile" ]; then
	echo "First argument is not .log file, cannot be read or does not exist"
	ok=0
fi
if (echo "$xmlfile" | grep -vq "\.xml$") || [ ! -f "$xmlfile" ] || [ ! -r "$xmlfile" ]; then
	echo "Second argument is not .xml file, cannot be read or does not exist"
	ok=0
fi
if (echo "$wavfile" | grep -vq "\.wav$") || [ ! -f "$wavfile" ] || [ ! -r "$wavfile" ]; then
	echo "Third argument is not .wav file, cannot be read or does no exist"
	ok=0
fi

if (( $ok == 0 )); then
	exit 1
fi

#if outputdir is not dir add / in the end
outputdir=$(echo "$outputdir" | sed 's/^.*[^/]$/&\//')

#timezone
if [ -z $EEG_TZ ]; then
	EEG_TZ="UTC"
fi

smprate=$(grep "SamplingRate" "$xmlfile" | sed 's/<[^0-9]*>//g' | cut -d' ' -f1)
srd=$(grep "StartRecordingDate" "$xmlfile" | sed 's/<[^0-9]*>//g' | sed -r 's/([0-9]{1,2}).([0-9]{1,2})./\2\/\1\//')
srt=$(grep "StartRecordingTime" "$xmlfile" | sed 's/<[^0-9]*>//g')

eegstart=$(TZ=${EEG_TZ} date --date="$srd $srt" "+%s.%N")
eegend=$(echo "$eegstart + ($(grep tick "$xmlfile" | wc -l) / $smprate)" | bc)

wavstart=$(grep beep "$logfile" | cut -d' ' -f3)
wavend=$(echo  "$wavstart + $(soxi -D "$wavfile")" | bc)

mkdir -p "$outputdir"

grep -wv beep "$logfile" | while read line; do

	stimulus=$(echo "$line" | cut -d' ' -f1)
	begining=$(echo "$line" | cut -d' ' -f2)
	end=$(echo "$line" | cut -d' ' -f3)

	#check if there are already files with the same names
	if [ -f "${outputdir}${stimulus}_lar.wav" ] || [ -f "${outputdir}${stimulus}_eeg.xml" ]; then
		echo -e "Stimulus $stimulus was already examined\nStopping script"
		exit 1
	fi

	if (( $(echo "($end - $begining) >= 0.2" | bc) )); then

		#check if interval is out
		if (( $(echo "$begining < $eegstart" | bc) \
			|| $(echo "$end > $eegend" | bc) \
		       	|| $(echo "$begining < $wavstart" | bc) \
			|| $(echo "$end > $wavend" | bc) )); then

                	echo "Stimulus ${stimulus}'s interval is outside of file margins"
        	        continue
	        fi

		#https://www.lifewire.com/use-the-bc-calculator-in-scripts-2200588
		grep tick "$xmlfile" | head -n $(echo "($end - $eegstart)*$smprate/1" | bc) | tail -n $(echo "($end - $begining)*$smprate/1" | bc) > "${outputdir}${stimulus}_eeg.xml" 
			
		#https://stackoverflow.com/questions/9667081/how-do-you-trim-the-audio-files-end-using-sox
		sox "$3" "${outputdir}${stimulus}_lar.wav" trim $(echo "($begining - $wavstart)" | bc) =$(echo "($end - $wavstart)" | bc) 

	else
		echo "For stimulus: $stimulus, interval is less than 0.2s"
	fi
done
