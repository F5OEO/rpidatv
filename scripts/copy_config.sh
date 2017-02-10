## Updated for version 201701270

## Usage ########################################

# This file needs to be updated each time new entries are
# added to rpidatvconfig.txt
# The base entries have always been present
# Subsequent entires are added only if they 
# are found in the user's old file
# Base Commands were present prior to 201701020
#
# 201701020 added:
# analogcaminput
# analogcamstandard
#
# 201701270 added:
# pfreq1=71
# pfreq2=146.5
# pfreq3=437
# pfreq4=1249
# pfreq5=1255
# adfref=25000000
# adflevel0=0
# adflevel1=0
# adflevel2=0
# adflevel3=0
# explevel0=30
# explevel1=30
# explevel2=30
# explevel3=30
# numbers0=0000
# numbers1=1111
# numbers2=2222
# numbers3=3333
# audio=usb
# vfinder=on
# beta=no
#
# 2017021?? added
# explevel4
# expports0
# expports1
# expports2
# expports3
# expports4
# psr1
# psr2
# psr3
# psr4
# psr5
# streamurl
# streamkey
#
# This file should be called by the install script
# if the latest entries are not found in the user's 
# rpidatvconfig.txt.
# If the latest entires are found, then the user's
# existing file should be copied back in to
# rpidatv/scripts
#
############################################### 

####### Set Environment Variables ###############

PATHSCRIPT=/home/pi/rpidatv/scripts
OLDCONFIGFILE=/home/pi/rpidatvconfig.txt
NEWCONFIGFILE=$PATHSCRIPT"/rpidatvconfig.txt"

####### Define Functions ########################

set_config_var() {
lua - "$1" "$2" "$3" <<EOF > "$3.bak"
local key=assert(arg[1])
local value=assert(arg[2])
local fn=assert(arg[3])
local file=assert(io.open(fn))
local made_change=false
for line in file:lines() do
if line:match("^#?%s*"..key.."=.*$") then
line=key.."="..value
made_change=true
end
print(line)
end
if not made_change then
print(key.."="..value)
end
EOF
mv "$3.bak" "$3"
}

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

####### Transfer each value to the New File ###############

TRANSFER=$(get_config_var modeinput $OLDCONFIGFILE)
set_config_var modeinput "$TRANSFER" $NEWCONFIGFILE

TRANSFER=$(get_config_var symbolrate $OLDCONFIGFILE)
set_config_var symbolrate "$TRANSFER" $NEWCONFIGFILE

TRANSFER=$(get_config_var fec $OLDCONFIGFILE)
set_config_var fec "$TRANSFER" $NEWCONFIGFILE

TRANSFER=$(get_config_var freqoutput $OLDCONFIGFILE)
set_config_var freqoutput "$TRANSFER" $NEWCONFIGFILE

TRANSFER=$(get_config_var rfpower $OLDCONFIGFILE)
set_config_var rfpower "$TRANSFER" $NEWCONFIGFILE

TRANSFER=$(get_config_var modeoutput $OLDCONFIGFILE)
set_config_var modeoutput "$TRANSFER" $NEWCONFIGFILE

TRANSFER=$(get_config_var tsvideofile $OLDCONFIGFILE)
set_config_var tsvideofile "$TRANSFER" $NEWCONFIGFILE

TRANSFER=$(get_config_var call $OLDCONFIGFILE)
set_config_var call "$TRANSFER" $NEWCONFIGFILE

TRANSFER=$(get_config_var paternfile $OLDCONFIGFILE)
set_config_var paternfile "$TRANSFER" $NEWCONFIGFILE

TRANSFER=$(get_config_var udpinaddr $OLDCONFIGFILE)
set_config_var udpinaddr "$TRANSFER" $NEWCONFIGFILE

TRANSFER=$(get_config_var pidvideo $OLDCONFIGFILE)
set_config_var pidvideo "$TRANSFER" $NEWCONFIGFILE

TRANSFER=$(get_config_var pidpmt $OLDCONFIGFILE)
set_config_var pidpmt "$TRANSFER" $NEWCONFIGFILE

TRANSFER=$(get_config_var serviceid $OLDCONFIGFILE)
set_config_var serviceid "$TRANSFER" $NEWCONFIGFILE

TRANSFER=$(get_config_var gpio_i $OLDCONFIGFILE)
set_config_var gpio_i "$TRANSFER" $NEWCONFIGFILE

TRANSFER=$(get_config_var gpio_q $OLDCONFIGFILE)
set_config_var gpio_q "$TRANSFER" $NEWCONFIGFILE

TRANSFER=$(get_config_var pathmedia $OLDCONFIGFILE)
set_config_var pathmedia "$TRANSFER" $NEWCONFIGFILE

TRANSFER=$(get_config_var locator $OLDCONFIGFILE)
set_config_var locator "$TRANSFER" $NEWCONFIGFILE

TRANSFER=$(get_config_var pidstart $OLDCONFIGFILE)
set_config_var pidstart "$TRANSFER" $NEWCONFIGFILE

TRANSFER=$(get_config_var pidaudio $OLDCONFIGFILE)
set_config_var pidaudio "$TRANSFER" $NEWCONFIGFILE

TRANSFER=$(get_config_var display $OLDCONFIGFILE)
set_config_var display "$TRANSFER" $NEWCONFIGFILE

TRANSFER=$(get_config_var menulanguage $OLDCONFIGFILE)
set_config_var menulanguage "$TRANSFER" $NEWCONFIGFILE

TRANSFER=$(get_config_var analogcamname $OLDCONFIGFILE)
set_config_var analogcamname "$TRANSFER" $NEWCONFIGFILE

TRANSFER=$(get_config_var startup $OLDCONFIGFILE)
set_config_var startup "$TRANSFER" $NEWCONFIGFILE

if grep -q analogcaminput /home/pi/rpidatvconfig.txt; then
  # File includes analogcaminput and others
  # Which were added in 201701020

  TRANSFER=$(get_config_var analogcaminput $OLDCONFIGFILE)
  set_config_var analogcaminput "$TRANSFER" $NEWCONFIGFILE

  TRANSFER=$(get_config_var analogcamstandard $OLDCONFIGFILE)
  set_config_var analogcamstandard "$TRANSFER" $NEWCONFIGFILE
fi

if grep -q pfreq1 /home/pi/rpidatvconfig.txt; then
  # User's File includes pfreq1 and others
  # Which were added in 201701270

  TRANSFER=$(get_config_var pfreq1 $OLDCONFIGFILE)
  set_config_var pfreq1 "$TRANSFER" $NEWCONFIGFILE

  TRANSFER=$(get_config_var pfreq2 $OLDCONFIGFILE)
  set_config_var pfreq2 "$TRANSFER" $NEWCONFIGFILE

  TRANSFER=$(get_config_var pfreq3 $OLDCONFIGFILE)
  set_config_var pfreq3 "$TRANSFER" $NEWCONFIGFILE

  TRANSFER=$(get_config_var pfreq4 $OLDCONFIGFILE)
  set_config_var pfreq4 "$TRANSFER" $NEWCONFIGFILE

  TRANSFER=$(get_config_var pfreq5 $OLDCONFIGFILE)
  set_config_var pfreq5 "$TRANSFER" $NEWCONFIGFILE

  TRANSFER=$(get_config_var adfref $OLDCONFIGFILE)
  set_config_var adfref "$TRANSFER" $NEWCONFIGFILE

  TRANSFER=$(get_config_var adflevel0 $OLDCONFIGFILE)
  set_config_var adflevel0 "$TRANSFER" $NEWCONFIGFILE

  TRANSFER=$(get_config_var adflevel1 $OLDCONFIGFILE)
  set_config_var adflevel1 "$TRANSFER" $NEWCONFIGFILE

  TRANSFER=$(get_config_var adflevel2 $OLDCONFIGFILE)
  set_config_var adflevel2 "$TRANSFER" $NEWCONFIGFILE

  TRANSFER=$(get_config_var adflevel3 $OLDCONFIGFILE)
  set_config_var adflevel3 "$TRANSFER" $NEWCONFIGFILE

  TRANSFER=$(get_config_var explevel0 $OLDCONFIGFILE)
  set_config_var explevel0 "$TRANSFER" $NEWCONFIGFILE

  TRANSFER=$(get_config_var explevel1 $OLDCONFIGFILE)
  set_config_var explevel1 "$TRANSFER" $NEWCONFIGFILE

  TRANSFER=$(get_config_var explevel2 $OLDCONFIGFILE)
  set_config_var explevel2 "$TRANSFER" $NEWCONFIGFILE

  TRANSFER=$(get_config_var explevel3 $OLDCONFIGFILE)
  set_config_var explevel3 "$TRANSFER" $NEWCONFIGFILE

  TRANSFER=$(get_config_var numbers0 $OLDCONFIGFILE)
  set_config_var numbers0 "$TRANSFER" $NEWCONFIGFILE

  TRANSFER=$(get_config_var numbers1 $OLDCONFIGFILE)
  set_config_var numbers1 "$TRANSFER" $NEWCONFIGFILE

  TRANSFER=$(get_config_var numbers2 $OLDCONFIGFILE)
  set_config_var numbers2 "$TRANSFER" $NEWCONFIGFILE

  TRANSFER=$(get_config_var numbers3 $OLDCONFIGFILE)
  set_config_var numbers3 "$TRANSFER" $NEWCONFIGFILE

  TRANSFER=$(get_config_var audio $OLDCONFIGFILE)
  set_config_var audio "$TRANSFER" $NEWCONFIGFILE

  TRANSFER=$(get_config_var vfinder $OLDCONFIGFILE)
  set_config_var vfinder "$TRANSFER" $NEWCONFIGFILE

  TRANSFER=$(get_config_var beta $OLDCONFIGFILE)
  set_config_var beta "$TRANSFER" $NEWCONFIGFILE

fi

if grep -q explevel4 /home/pi/rpidatvconfig.txt; then
  # User's File includes explevel4 and others
  # Which were added in 2017021??

  TRANSFER=$(get_config_var explevel4 $OLDCONFIGFILE)
  set_config_var explevel4 "$TRANSFER" $NEWCONFIGFILE

  TRANSFER=$(get_config_var expports0 $OLDCONFIGFILE)
  set_config_var expports0 "$TRANSFER" $NEWCONFIGFILE

  TRANSFER=$(get_config_var expports1 $OLDCONFIGFILE)
  set_config_var expports1 "$TRANSFER" $NEWCONFIGFILE

  TRANSFER=$(get_config_var expports2 $OLDCONFIGFILE)
  set_config_var expports2 "$TRANSFER" $NEWCONFIGFILE

  TRANSFER=$(get_config_var expports3 $OLDCONFIGFILE)
  set_config_var expports3 "$TRANSFER" $NEWCONFIGFILE

  TRANSFER=$(get_config_var expports4 $OLDCONFIGFILE)
  set_config_var expports4 "$TRANSFER" $NEWCONFIGFILE

  TRANSFER=$(get_config_var psr1 $OLDCONFIGFILE)
  set_config_var psr1 "$TRANSFER" $NEWCONFIGFILE

  TRANSFER=$(get_config_var psr2 $OLDCONFIGFILE)
  set_config_var psr2 "$TRANSFER" $NEWCONFIGFILE

  TRANSFER=$(get_config_var psr3 $OLDCONFIGFILE)
  set_config_var psr3 "$TRANSFER" $NEWCONFIGFILE

  TRANSFER=$(get_config_var psr4 $OLDCONFIGFILE)
  set_config_var psr4 "$TRANSFER" $NEWCONFIGFILE

  TRANSFER=$(get_config_var psr5 $OLDCONFIGFILE)
  set_config_var psr5 "$TRANSFER" $NEWCONFIGFILE

  TRANSFER=$(get_config_var streamurl $OLDCONFIGFILE)
  set_config_var streamurl "$TRANSFER" $NEWCONFIGFILE

  TRANSFER=$(get_config_var streamkey $OLDCONFIGFILE)
  set_config_var streamkey "$TRANSFER" $NEWCONFIGFILE

fi
