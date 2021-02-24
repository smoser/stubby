#!/bin/bash

#
# stubby smash script
#
# Copyright (c) 2020,2021 Cisco Systems, Inc. <pmoore2@cisco.com>
#

#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

STUBBY="stubby.efi"

function usage() {
	echo "usage: stubby-smash -o <output>"
	echo "         -k <kernel> -i <initrd> -c <cmdline> [-s <sbat>]"
	echo ""
	echo "  Combine the <kernel>, <initrd>, <cmdline>, and optionally a"
	echo "  <sbat> files into a single bootable EFI app in <output>."
	echo ""
	exit 1
}

function error() {
	echo "error: $@"
	exit 1
}

function deps() {
	[[ -z $@ ]] && return
	for i in $@; do
		which $i >& /dev/null || \
			error "please ensure \"$i\" is installed"
	done
}

# main
#

# dependency checks
deps objcopy

# argument parsing
while getopts ":o:k:i:c:s:" opt; do
	case $opt in
	o)
		arg_output=$OPTARG
		;;
	k)
		arg_kernel=$OPTARG
		;;
	i)
		arg_initrd=$OPTARG
		;;
	c)
		arg_cmdline=$OPTARG
		;;
	s)
		arg_sbat=$OPTARG
		;;
	*)
		usage
		;;
	esac
done
shift $(( $OPTIND - 1 ))

# sanity checks
[[ ! -r $STUBBY ]] && error "unable to find \"$STUBBY\" in current directory"
[[ ! -r $arg_kernel ]] && error "unable to read the kernel file"
[[ ! -r $arg_initrd ]] && error "unable to read the initrd file"
[[ ! -r $arg_cmdline ]] && error "unable to read the cmdline file"
[[ -n $arg_sbat && ! -r $arg_sbat ]] && error "unable to read the SBAT file"

# output check
[[ -x $arg_output ]] && error "output file \"$arg_output\" already exists"

# setup SBAT args to objcopy
ocpy_sbat=""
if [[ -n $arg_sbat ]]; then
	ocpy_sbat+=" --set-section-alignment '.sbat=512' "
	ocpy_sbat+=" --add-section .sbat=$arg_sbat "
fi

exec objcopy \
        --add-section .cmdline=$arg_cmdline \
                --change-section-vma .cmdline=0x30000  \
        --add-section .linux=$arg_kernel \
                --change-section-vma .linux=0x2000000 \
        --add-section .initrd=$arg_initrd \
                --change-section-vma .initrd=0x3000000 \
	$ocpy_sbat \
        $STUBBY $arg_output
