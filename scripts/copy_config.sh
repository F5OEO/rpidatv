## Updated for version 201701270

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
  # File includes analogcaminput and analogcammname
  # Which were added in 201701020

  TRANSFER=$(get_config_var analogcaminput $OLDCONFIGFILE)
  set_config_var analogcaminput "$TRANSFER" $NEWCONFIGFILE

  TRANSFER=$(get_config_var analogcamstandard $OLDCONFIGFILE)
  set_config_var analogcamstandard "$TRANSFER" $NEWCONFIGFILE
fi

