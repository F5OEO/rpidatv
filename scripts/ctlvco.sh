########## ctlvco.sh ############

# Called by a.sh in IQ mode to set ADF4351
# vco to correct frequency

############ Set Environment Variables ###############

PATHSCRIPT=/home/pi/rpidatv/scripts
PATHRPI=/home/pi/rpidatv/bin
CONFIGFILE=$PATHSCRIPT"/rpidatvconfig.txt"

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

############### Read Frequency #########################

FREQM=$(get_config_var freqoutput $CONFIGFILE)

############### Call binary to set frequency ########

sudo $PATHRPI"/adf4351" $FREQM

### End ###

# Revert to a.sh #

