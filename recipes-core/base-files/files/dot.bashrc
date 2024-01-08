# If not running interactively, don't do anything
case $- in
    *i*) ;;
      *) return;;
esac


# keep the prompt short
PS1='\u:\W\$ '

# don't put duplicate lines or lines starting with space in the history.
# See bash(1) for more options
HISTCONTROL=ignoreboth

# append to the history file, don't overwrite it
shopt -s histappend

# for setting history length see HISTSIZE and HISTFILESIZE in bash(1)
HISTSIZE=1000
HISTFILESIZE=2000

# check the window size after each command and, if necessary,
# update the values of LINES and COLUMNS.
shopt -s checkwinsize

# If set, the pattern "**" used in a pathname expansion context will
# match all files and zero or more directories and subdirectories.
#shopt -s globstar

# make less more friendly for non-text input files, see lesspipe(1)
[ -x /usr/bin/lesspipe ] && eval "$(SHELL=/bin/sh lesspipe)"

# If this is an xterm set the title to user@host:dir
case "$TERM" in
xterm*|rxvt*)
    PS1="\[\e]0;${debian_chroot:+($debian_chroot)}\u@\h: \w\a\]$PS1"
    ;;
*)
    ;;
esac

# enable color support of ls and also add handy aliases
if [ -x /usr/bin/dircolors ]; then
    test -r ~/.dircolors && eval "$(dircolors -b ~/.dircolors)" || eval "$(dircolors -b)"
    alias ls='ls --color=auto'
    #alias dir='dir --color=auto'
    #alias vdir='vdir --color=auto'

    alias grep='grep --color=auto'
    alias fgrep='fgrep --color=auto'
    alias egrep='egrep --color=auto'
fi

# colored GCC warnings and errors
export GCC_COLORS='error=01;31:warning=01;35:note=01;36:caret=01;32:locus=01:quote=01'

# some more ls aliases
alias ll='ls -alF'
alias la='ls -A'
alias l='ls -CF'

# Add an "alert" alias for long running commands.  Use like so:
#   sleep 10; alert
alias alert='notify-send --urgency=low -i "$([ $? = 0 ] && echo terminal || echo error)" "$(history|tail -n1|sed -e '\''s/^\s*[0-9]\+\s*//;s/[;&|]\s*alert$//'\'')"'

# Alias definitions.
# You may want to put all your additions into a separate file like
# ~/.bash_aliases, instead of adding them here directly.
# See /usr/share/doc/bash-doc/examples in the bash-doc package.

print_help() {
	echo "Welcome AReS User!"
  echo "In order to help you this short guide has been prepared for you with easy to use commands that acomplish the most common development tasks"
  echo "You can always print off this guide again by typing \"help\""
  echo ""
  echo "1. OS and Config version and general AReS Health" 
  echo "    version                 | print off the os version"
  echo "    conig_version           | print off the config data version format version and aircraft model"
  echo "    serial                  | print off the serial number of the box as defined by the x.509 private key"
  echo "    aircraft                | print off the aircraft id of the box"
  echo "    temp                    | print off the cpu temperature"
  echo "    mon_temp                | continuously print off the cpu temperature until ctrl+C is pressed"
  echo "    module_uptime           | the current module uptime from the recorder in milliseconds, stop with ctrl+c"
  echo "    gtime                   | get the garmin time parameter, stop with ctrl+c"
  echo "    gdate                   | get the garmin date parameter, stop with ctrl+c"
  echo "    space                   | get the amount of disk space left on the data disk"
  echo "    sftp_knock              | wakeup the sftp server for 1 hour"
	echo ""		
	echo "2. Applets and their health"
  echo "    check_apps              | print off the status of all applets that should be currently running"
	echo "    apps                    | print off a list of the systemd service files that define all applets"
	echo "    network_status          | print off the current status of all network interfaces in a json format, may take a few seconds to run"
	echo "    rs                      | print off the current status of the recorder as well as the path to the current dat file being recorded"
	echo "    sys [command] [service] | command may be used to perform the following actions on a service start/stop/status/restart"
  echo "    tailf [log_path]        | watch a log file for an applet as it is being written to, (i.e. watch /var/log/recorder.log) stop with ctrl+C"	
  echo "    cell_connect            | send a connect command to the cellular manager"	
  echo "    cell_disconnect         | send a disconnect command to the cellular manager"	
	echo ""	
	echo "3. Viewing Data Live and Recorded"
	echo "    playback                | get a list of live data being published by the publisher, stop with ctrl+C"
  echo "                            | output can be piped through grep to file by envelope"
  echo "                            | for example \"playback | grep -i \"0.0.1.0.0.64\"\" to only get discrete IO payloads"
  echo "    sub                     | more advanced, but slower version of playback written in python use -h to get help options"
	echo "    pub                     | general purpose message publisher use -h to get help options"
	echo "    decode                  | run the decoder program"	
	echo "    f250                    | print the first 250 bytes of the dat file currently being written to by the recorder"
	echo "    recording               | print off the location of the dat file currently being written to by the recorder"
	echo "    dataset                 | print off the location of the dataset directory currengly being written to by the recorder"
	echo "    pub_network_status      | run the pub network status script, use option --help for usage"
  echo ""
	echo "4. Polemos"
	echo "    polemos_systick         | print off the systick from polemos, type CTRL+C to stop"
  echo ""
	echo "5. Thrace"
	echo "    thrace_present          | check to see if thrace is present on the current network connection"
	echo "    thrace_cert             | print off information about the current thrace certificate"
  echo "" 
	echo "6. Advanced"
	echo "    makerw                  | remount the rootfs in read-write mode"
  echo "    makero                  | remount the rootfs in read-only mode"	
  echo "    update-ca-certificatesrw| switch rootfs to rw mode and reload ca certificates, then switch back to ro mode"
  echo "    permit-root-login       |"
  echo "    prohibit-root-login     |"
  echo "    sftp_knock              | wakeup the sftp server for 1 hour"

}

alias stop="systemctl stop recoverer"
alias q="systemctl stop recoverer"
alias s="systemctl stop recoverer"
alias sys='sys_alias() { systemctl "$1" "$2"; }; sys_alias'
alias playback="/playback/playback -P 0"
alias decode="/decoder/decoder"
alias version="cat /ares/bin/version.json"
alias tailf='watch_log() { tail -f "$1";}; watch_log'
alias rs='recorder_status() { systemctl status recorder; ls -l /tmp/recording.dat.gz.link; ls -lH /tmp/recording.dat.gz.link;}; recorder_status'
alias f250="zcat /tmp/recording.dat.gz.link | xxd -i -l 250"
#alias f250="ls -l /tmp/recording.dat.gz.link && ls -lH /tmp/recording.dat.gz.link && xxd -i -l 250 /tmp/recording.dat.gz.link"
#alias l250="zcat /tmp/recording.dat.gz.link | xxd -i -s -250"
alias network_status='network_status() { python3 /ares/bin/sub.py -e 0.4 -x -j -s 1;}; network_status'
alias apps="find /etc/systemd/system -type f"
alias check_apps="python3 /ares/bin/check_apps.py"
alias recording="ls -l /tmp/recording.dat.gz.link"
alias dataset="ls -l /tmp/dataset.link"
alias polemos_systick="python3 /ares/bin/sub.py -e 0.1.49 -x -d =I -m 0.1"
alias module_uptime="python3 /ares/bin/sub.py -e 0.2.111 -x -d =I -m 0.2"
alias gtime="python3 /ares/bin/sub.py -e 0.2.230 -x -m 0.2"
alias gdate="python3 /ares/bin/sub.py -e 0.2.231 -x -m 0.2"
alias help="print_help"
alias config_version="cat /ares/var/run/config.cfg | grep config_data_version -A 1; cat /ares/var/run/config.cfg | grep config_format_version -A 1; cat /ares/var/run/config.cfg | grep aircraft_identification -A 2" 
alias serial="ls /ares/var/run/conf/private"
alias temp="sensors | grep CPU-Temp"
alias mon_temp="watch -n 1 sensors"
alias cell_connect="python3 /ares/bin/pub.py -e 0.100.0.1"
alias cell_disconnect="python3 /ares/bin/pub.py -e 0.100.0.2"
alias aircraft="cat /ares/var/run/id.cfg && echo \"order: model,serial,company,field1,field2,era_uuid\" || echo \"No Aircraft ID Set!\""
alias thrace_present="python3 /network_manager/src/thrace_util.py"
alias thrace_cert="openssl x509 -noout -text -in /ares/var/run/conf/thrace/thrace.crt"
alias pub_network_status="python3 /ares/bin/pub_network_status.py"
alias sub="python3 /ares/bin/sub.py"
alias pub="python3 /ares/bin/pub.py"
alias space="df -h /dev/data1"
alias makerw="mount -o remount,rw /dev/root /"
alias makero="mount -o remount,ro /dev/root /"
alias update-ca-certificatesrw="mount -o remount,rw /dev/root / ; update-ca-certificates; mount -o remount,ro /dev/root /"
alias permit-root-login="sed -i 's/PermitRootLogin no/PermitRootLogin yes/g' /etc/ssh/sshd_config*"
alias prohibit-root-login="sed -i 's/PermitRootLogin yes/PermitRootLogin no/g' /etc/ssh/sshd_config*"
alias sftp_knock="python3 /ares/bin/req.py -c FILE_MANAGER_SFTP_SERVER_KNOCK -p \"{\\\"expires\\\":3600}\""

if [ -f ~/.bash_aliases ]; then
    . ~/.bash_aliases
fi

# enable programmable completion features (you don't need to enable
# this, if it's already enabled in /etc/bash.bashrc and /etc/profile
# sources /etc/bash.bashrc).
if ! shopt -oq posix; then
  if [ -f /usr/share/bash-completion/bash_completion ]; then
    . /usr/share/bash-completion/bash_completion
  elif [ -f /etc/bash_completion ]; then
    . /etc/bash_completion
  fi
fi

print_help
