########## ctlvco.sh ############

# Called by a.sh in IQ mode to set ADF4351 vco 
# to correct frequency with correct ref freq and level

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

########### Read Frequency and Ref Frequency ###############

FREQM=$(get_config_var freqoutput $CONFIGFILE)
FREQR=$(get_config_var adfref $CONFIGFILE)

INT_FREQ_OUTPUT=${FREQM%.*}

############### Switch Power based on Frequency ########

if (( $INT_FREQ_OUTPUT \< 100 )); then
  PWR=$(get_config_var adflevel0 $CONFIGFILE);
elif (( $INT_FREQ_OUTPUT \< 250 )); then
  PWR=$(get_config_var adflevel1 $CONFIGFILE);
elif (( $INT_FREQ_OUTPUT \< 950 )); then
  PWR=$(get_config_var adflevel2 $CONFIGFILE);
elif (( $INT_FREQ_OUTPUT \< 4400 )); then
  PWR=$(get_config_var adflevel3 $CONFIGFILE);
else
  PWR="0";
fi

############### Call binary to set frequency ########

sudo $PATHRPI"/adf4351" $FREQM $FREQR $PWR

### End ###

# Revert to a.sh #

