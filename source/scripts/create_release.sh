#!/bin/bash

#############################################################################
# Create a new release
#
# This will:
#  - Create a new CMakeBuild.config in $BLUENET_DIR/release
#  - Build firmware, DFU package and docs and copy them to $BLUENET_RELEASE_DIR
#  - Update the index.json file in $BLUENET_RELEASE_DIR to keep
#    track of stable, latest, and release dates
#  - Create a change log file from git commits since last release
#  - Create a git tag with the version number
#
#############################################################################


# Get the scripts path: the path where this file is located.
path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh

usage() {
	cs_info "Usage: $0 <firmware|bootloader> [keyfile]"
}

if [ $# -lt 1 ]; then
	usage
	exit 1
fi

if [ "$1" != "firmware" ] && [ "$1" != "bootloader" ]; then
	usage
	exit 1
fi

if [ $2 ]; then
	key_file="$2"
fi

release_firmware="true"
release_bootloader="false"
version_file="${BLUENET_DIR}/VERSION"
changes_file="${BLUENET_DIR}/CHANGES"
if [ "$1" == "bootloader" ]; then
	release_firmware="false"
	release_bootloader="true"
	version_file="${BLUENET_DIR}/bootloader/VERSION"
	changes_file="${BLUENET_DIR}/bootloader/CHANGES"
fi

if [ $release_bootloader == "true" ]; then
	cs_info "Create bootloader release"
else
	cs_info "Create firmware release"
fi


############################
### Pre Script Verification
############################

if [ -z $BLUENET_RELEASE_DIR ]; then
	cs_err "BLUENET_RELEASE_DIR is not defined!"
	exit 1
fi
BLUENET_RELEASE_CANDIDATE_DIR="${BLUENET_RELEASE_DIR}-candidate"


##############################################
### Check bluenet branch, changes, and remote
##############################################

pushd $BLUENET_DIR &> /dev/null

# Check current branch, releases should be made from master branch
branch=$(git symbolic-ref --short -q HEAD)

if [[ $branch != "master" ]]; then
	cs_err "Bluenet is currently on branch '"$branch"'"
	cs_err "Releases should be made from master branch"
	cs_err "Are you sure you want to continue? [y/N]"
	read branch_response
	if [[ ! $branch_response == "y" ]]; then
		cs_info "abort"
		exit 1
	fi
fi

# Check for modifications
modifications=$(git ls-files -m | wc -l)

if [[ $modifications != 0 ]]; then
	cs_err "There are modified files in your bluenet code"
	cs_err "Commit the files or stash them first!!"
	exit 1
fi

# Check for untracked files
untracked=$(git ls-files --others --exclude-standard | wc -l)

if [[ $untracked != 0 ]]; then
	cs_err "The following untracked files were found in the bluenet code"
	cs_err "Make sure you didn't forget to add anything important!"
	git ls-files --others --exclude-standard
	cs_info "Do you want to continue? [Y/n]"
	read untracked_response
	if [[ $untracked_response == "n" ]]; then
		exit 1
	fi
fi

# check for remote updates
git remote update

# if true then there are remote updates that need to be pulled first
if [ ! $(git rev-parse HEAD) = $(git ls-remote $(git rev-parse --abbrev-ref @{u} | sed 's/\// /g') | cut -f1) ]; then
	cs_err "There are remote updates that were not yet pulled"
	cs_err "Are you sure you want to continue? [y/N]"
	read update_response
	if [[ ! $update_response == "y" ]]; then
		cs_info "abort"
		exit 1
	fi
fi

popd &> /dev/null

############################
### Prepare
############################

cs_info "Stable version? [y/N]: "
read stable
if [[ $stable == "y" ]]; then
	stable=1
else
	stable=0
fi

valid=0
existing=0

# Get old version
if [ -f $version_file ]; then
	current_version_str=`head -1 $version_file`
	version_list=(`echo $current_version_str | tr '.' ' '`)
	v_major=${version_list[0]}
	v_minor=${version_list[1]}
	v_patch=${version_list[2]}
	current_version_int=`head -2 $version_file | tail -1`
	cs_log "Current version: $current_version_str"
	cs_log "Current version int: $current_version_int"
	# v_minor=$((v_minor + 1))
	# v_patch=0
	suggested_version="$v_major.$v_minor.$v_patch"
else
	cs_err "Version file (${version_file}) is missing."
	exit 1
fi

# Ask for new version string
while [[ $valid == 0 ]]; do
	cs_info "Enter a version number [$suggested_version]:"
	read -e version
	if [[ $version == "" ]]; then
		version=$suggested_version
	fi

	if [[ $version =~ ^[0-9]{1,2}\.[0-9]{1,3}\.[0-9]{1,3}$ ]]; then
		if [ $release_bootloader == "true" ]; then
			model="bootloader"
		else
			cs_info "Select model:"
			printf "$yellow"
			options=("Crownstone" "Guidestone" "Dongle")
			select opt in "${options[@]}"
			do
				case $opt in
					"Crownstone")
						model="crownstone"
						# device_type="DEVICE_CROWNSTONE_PLUG"
						break
						;;
					"Guidestone")
						model="guidestone"
						# device_type="DEVICE_GUIDESTONE"
						break
						;;
					"Dongle")
						model="dongle"
						break
						;;
					*) echo invalid option;;
				esac
			done
			printf "$normal"
		fi
		release_config_dir="${BLUENET_DIR}/release/${model}_${version}"

		# If release candidate, find the dir that doesn't exist yet. Keep up the RC number
		rc_prefix="-RC"
		rc_count=0
		rc_str=""
		if [[ $stable == 0 ]]; then
			while true; do
				rc_str="${rc_prefix}${rc_count}"
				release_config_dir="${BLUENET_DIR}/release/${model}_${version}${rc_str}"
				if [[ ! -d $release_config_dir ]]; then
					break
				fi
				((rc_count++))
			done
			version="${version}${rc_str}"
		fi

		if [ -d $release_config_dir ]; then
			cs_err "Version already exists, are you sure? [y/N]: "
			read version_response
			if [[ $version_response == "y" ]]; then
				existing=1
				valid=1
			fi
		else
			valid=1
		fi
	else
		cs_err "Version (${version}) does not match pattern"
	fi
done

new_version_int=$(($current_version_int + 1))

if [[ $stable == 0 ]]; then
	cs_info "Release candidate ${rc_count} (version ${version}  int ${new_version_int}), is that correct? [Y/n]"
	read version_response
	if [[ $version_response == "n" ]]; then
		cs_info "abort"
		exit 1
	fi
fi


#####################################
### Create Config file and directory
#####################################

# create directory in bluenet with new config file. copy from default and open to edit

if [[ $existing == 0 ]]; then
	cs_log "Creating new directory: "$release_config_dir
	mkdir $release_config_dir &> /dev/null

	cp $BLUENET_DIR/conf/cmake/CMakeBuild.config.default $release_config_dir/CMakeBuild.config

###############################
### Fill Default Config Values
###############################

	if [[ $model == "guidestone" ]]; then
		sed -i "s/DEFAULT_HARDWARE_BOARD=.*/DEFAULT_HARDWARE_BOARD=GUIDESTONE/" $release_config_dir/CMakeBuild.config
	else
		sed -i "s/DEFAULT_HARDWARE_BOARD=.*/DEFAULT_HARDWARE_BOARD=ACR01B2C/" $release_config_dir/CMakeBuild.config
	fi

	sed -i "s/BLUETOOTH_NAME=\".*\"/BLUETOOTH_NAME=\"CS\"/" $release_config_dir/CMakeBuild.config
	if [ $release_bootloader == "true" ]; then
		sed -i "s/BOOTLOADER_VERSION=\".*\"/BOOTLOADER_VERSION=\"$version\"/" $release_config_dir/CMakeBuild.config
	else
		sed -i "s/FIRMWARE_VERSION=\".*\"/FIRMWARE_VERSION=\"$version\"/" $release_config_dir/CMakeBuild.config
	fi

	sed -i "s/NRF5_DIR=/#NRF5_DIR=/" $release_config_dir/CMakeBuild.config
	sed -i "s/COMPILER_PATH=/#COMPILER_PATH=/" $release_config_dir/CMakeBuild.config

	sed -i "s/CROWNSTONE_SERVICE=.*/CROWNSTONE_SERVICE=1/" $release_config_dir/CMakeBuild.config

	sed -i "s/SERIAL_VERBOSITY=.*/SERIAL_VERBOSITY=SERIAL_BYTE_PROTOCOL_ONLY/" $release_config_dir/CMakeBuild.config
	sed -i "s/CS_UART_BINARY_PROTOCOL_ENABLED=.*/CS_UART_BINARY_PROTOCOL_ENABLED=1/" $release_config_dir/CMakeBuild.config
	sed -i "s/CS_SERIAL_NRF_LOG_ENABLED=.*/CS_SERIAL_NRF_LOG_ENABLED=0/" $release_config_dir/CMakeBuild.config

	xdg-open "$release_config_dir/CMakeBuild.config" &> /dev/null
	if [[ $? != 0 ]]; then
		cs_info "Open $release_config_dir/CMakeBuild.config in to edit the config"
	fi

	cs_log "After editing the config file, press [ENTER] to continue"
	read
else
	cs_warn "Warn: Using existing configuration"
fi

############################
###  modify paths
############################

# TODO: use <model>_<version> as target instead.
export BLUENET_CONFIG_DIR=$release_config_dir
export BLUENET_BUILD_DIR="$BLUENET_BUILD_DIR/${model}_${version}"
if [[ $stable == 0 ]]; then
	# if [ -z $BLUENET_RELEASE_CANDIDATE_DIR ]; then
	# 	cs_err "$BLUENET_RELEASE_CANDIDATE_DIR does not exist!"
	# 	exit 1
	# fi
	export BLUENET_RELEASE_DIR=$BLUENET_RELEASE_CANDIDATE_DIR
fi
if [ $release_bootloader == "true" ]; then
	export BLUENET_RELEASE_DIR="$BLUENET_RELEASE_DIR/bootloaders/${model}_${version}"
else
	export BLUENET_RELEASE_DIR="$BLUENET_RELEASE_DIR/firmwares/${model}_${version}"
fi
export BLUENET_BIN_DIR=$BLUENET_RELEASE_DIR/bin

############################
### Run
############################

###################
### Config File
###################

cs_info "Copy configuration to release dir ..."
mkdir -p $BLUENET_RELEASE_DIR/config
cp $BLUENET_CONFIG_DIR/CMakeBuild.config $BLUENET_RELEASE_DIR/config
checkError
cs_succ "Copy DONE"


###################
### Version file
###################

# Update version file before building the bootloader settings page.
cs_info "Update version file"
if [[ $stable == 1 ]]; then
	echo $version > "$version_file"
	echo $new_version_int >> "$version_file"
else
	echo $current_version_str > "$version_file"
	echo $new_version_int >> "$version_file"
fi

###################
### Build
###################

cs_info "Build softdevice ..."
pushd $BLUENET_DIR/scripts &> /dev/null
./softdevice.sh build
checkError
popd &> /dev/null
cs_succ "Softdevice DONE"

if [ $release_bootloader == "true" ]; then
	cs_info "Build bootloader ..."
	pushd $BLUENET_DIR/scripts &> /dev/null
	./bootloader.sh release
	checkError
	popd &> /dev/null
	cs_succ "Build DONE"
else
	cs_info "Build firmware ..."
	pushd $BLUENET_DIR/scripts &> /dev/null
	./firmware.sh release
	checkError
	popd &> /dev/null
	cs_succ "Build DONE"
	
	cs_info "Check mesh setting ..."
	#mesh_timeslot_margin=$(grep -hoaP "TIMESLOT_END_SAFETY_MARGIN_US.\([^\(]+\)" "$BLUENET_BUILD_DIR/default/crownstone.elf" | grep -oP "\d+")
	grep_result=$(grep -hoaP "TIMESLOT_END_SAFETY_MARGIN_US.\(1000UL\)" "$BLUENET_BUILD_DIR/default/crownstone.elf")
	if [ "$grep_result" == "" ]; then
		cs_err "TIMESLOT_END_SAFETY_MARGIN_US is not set to 1000"
		exit $CS_ERR_CONFIG
	fi
	
	cs_info "Build bootloader settings ..."
	pushd $BLUENET_DIR/scripts &> /dev/null
	./bootloader.sh build-settings
	checkError
	popd &> /dev/null
	cs_succ "Build DONE"
fi


###################
### Move binaries
###################

cs_info "Move binaries ..."
# These days, the default target ends up in the dir "default".
#mv $BLUENET_BIN_DIR/default/* "$BLUENET_BIN_DIR"
find "$BLUENET_BIN_DIR/default" -type f -exec mv {} "$BLUENET_BIN_DIR" \;
rmdir "$BLUENET_BIN_DIR/default"

###################
### DFU package
###################

cs_info "Create DFU package ..."
dfu_gen_args=""
if [ $release_bootloader == "true" ]; then
	dfu_gen_args="$dfu_gen_args -B $BLUENET_BIN_DIR/bootloader.hex"
else
	dfu_gen_args="$dfu_gen_args -F $BLUENET_BIN_DIR/crownstone.hex"
fi
if [ $key_file ]; then
	dfu_gen_args="$dfu_gen_args -k $key_file"
else
	dfu_gen_args="$dfu_gen_args -K dfu_pkg_signing_key"
fi
dfu_gen_args="$dfu_gen_args -o $BLUENET_BIN_DIR/${model}_${version}.zip -v $new_version_int"
$BLUENET_DIR/scripts/dfu_gen_pkg.sh $dfu_gen_args
checkError

sha1sum "${BLUENET_BIN_DIR}/${model}_${version}.zip" | cut -f1 -d " " > "${BLUENET_BIN_DIR}/${model}_${version}.zip.sha1"
checkError

cs_succ "DFU DONE"


###################
### Documentation
###################

# goto bluenet scripts dir
pushd $BLUENET_DIR/scripts &> /dev/null

#cs_info "Build docs ..."
#./documentation.sh generate
#checkError
#cs_succ "Build DONE"

#cs_info "Copy docs to release dir ..."
#cp -r $BLUENET_DIR/docs $BLUENET_RELEASE_DIR/docs
#checkError
#cs_succ "Copy DONE"

#cs_info "Publish docs to git ..."
#git add $BLUENET_DIR/docs
#git commit -m "Update docs"
#./documentation.sh publish
#checkError
#cs_succ "Published docs DONE"

popd &> /dev/null

####################
### GIT release tag
####################

cs_info "Do you want to add a tag and commit to git? [Y/n]"
read git_response
if [[ $git_response == "n" ]]; then
	cs_info "abort"
	exit 1
fi

cs_info "Creating git commit for release"

cs_info "Add release config"
pushd $BLUENET_DIR &> /dev/null
git add $release_config_dir
git commit -m "Add release config for ${model}_${version}"

cs_log "Add version to git"
git add "$version_file"
if [ $release_bootloader == "true" ]; then
	git commit -m "Bootloader version bump to $version  int: $new_version_int"
else
	git commit -m "Version bump to $version  int: $new_version_int"
fi

if [[ $stable == 1 ]]; then
	cs_log "Update changes overview"
	if [[ $current_version_str ]]; then
		echo "Version $version:" > tmpfile
		git log --pretty=format:" - %s" "v$current_version_str"...HEAD >> tmpfile
		echo "" >> tmpfile
		echo "" >> tmpfile
		cat "$changes_file" >> tmpfile
		mv tmpfile "$changes_file"
	else
		echo "Version $version:" > "$changes_file"
		git log --pretty=format:" - %s" >> "$changes_file"
		echo "" >> "$changes_file"
		echo "" >> "$changes_file"
	fi

	cs_log "Add changes to git"
	git add "$changes_file"
	if [ $release_bootloader == "true" ]; then
		git commit -m "Changes for bootloader $version"
	else
		git commit -m "Changes for $version"
	fi
fi

cs_log "Create tag"
if [ $release_bootloader == "true" ]; then
	git tag -a -m "Tagging bootloader version ${version}" "bootloader-${version}"
else
	git tag -a -m "Tagging version ${version}" "v${version}"
fi
popd &> /dev/null
cs_succ "DONE. Created Release ${model}_${version}"
