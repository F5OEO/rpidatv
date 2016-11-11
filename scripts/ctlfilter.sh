########## ctlfilter.sh ############

# Called by a.sh in IQ mode to switch in correct
# Nyquist Filter

############ Set Environment Variables ###############


PATHSCRIPT=/home/pi/rpidatv/scripts
CONFIGFILE=$PATHSCRIPT"/rpidatvconfig.txt"

############### PIN DEFINITION ###########

#filter_bit0 LSB of filter control word=GPIO 16 / Header 36  
filter_bit0=16

#filter_bit1 Mid Bit of filter control word=GPIO 26 / Header 37
filter_bit1=26

#filter_bit0 MSB of filter control word=GPIO 20 / Header 38
filter_bit2=20

gpio -g mode $filter_bit0 out
gpio -g mode $filter_bit1 out
gpio -g mode $filter_bit2 out

############### Function to read Config File ###############

get_config_var() {
lua - "$1" "$2" <<EOF
local key=assert(arg[1])
local fn=assert(arg[2])
local file=assert(io.open(fn))
for line in file:lines() do
local val = line:match("^#?%s*"..key.."=(.*)$")
if (val ~= nil) then
print(val)
break
end
end
EOF
}

############### Read Symbol Rate #########################

SYMBOLRATEK=$(get_config_var symbolrate $CONFIGFILE)

############### Switch GPIOs based on Symbol Rate ########

case "$SYMBOLRATEK" in
	125)
                gpio -g write $filter_bit0 0;
                gpio -g write $filter_bit1 0;
		gpio -g write $filter_bit2 0;
	;;
	250)
		gpio -g write $filter_bit0 1;
                gpio -g write $filter_bit1 0;
                gpio -g write $filter_bit2 0;
	;;
	333)
                gpio -g write $filter_bit0 0;
                gpio -g write $filter_bit1 1;
                gpio -g write $filter_bit2 0;
        ;;
        500)
                gpio -g write $filter_bit0 1;
                gpio -g write $filter_bit1 1;
                gpio -g write $filter_bit2 0;
	;;
        1000)
                gpio -g write $filter_bit0 0;
                gpio -g write $filter_bit1 0;
                gpio -g write $filter_bit2 1;
        ;;
        2000)
                gpio -g write $filter_bit0 1;
                gpio -g write $filter_bit1 0;
                gpio -g write $filter_bit2 1;
        ;;
	*)
                gpio -g write $filter_bit0 1;
                gpio -g write $filter_bit1 0;
                gpio -g write $filter_bit2 1;
	;;
esac

### End ###

# Revert to a.sh #

