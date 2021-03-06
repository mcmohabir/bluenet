# Release process

## Bluenet

The release process starts with updating version information.

* Update the `VERSION` file in `source/VERSION` and `source/bootloader/VERSION`.
* Commit all changes to github.

Then to create a release:

```
cd build
make create_git_release
```

This will create a `release/` directory with the version information in a subdirectory, e.g. `release/crownstone_3.0.1-RC0`.
Double check if the CMakeBuild.config is correct. Eventually add a CMakeBuild.config.overwrite file for your own
passkey file.
Now using this information, we will build everything for this release.

```
cd build
cmake -DCONFIG_DIR=release -DBOARD_TARGET=crownstone_3.0.1-RC0 -DCMAKE_BUILD_TYPE=Release ..
make
```

The application version has to be bumped. This is an integer that is checked by the bootloader to make sure that the
application is actually newer.

```
cd crownstone_3.0.1-RC0
make increment_application_version
```

Now we can start generating the DFU packages that will be released:

```
make build_bootloader_settings
make generate_dfu_package_application
make generate_dfu_package_bootloader
make generate_dfu_package_all
make install
```

After make install you will find the `.zip` files in (subdirectories of) `bin/crownstone_3.0.1-RC0`.

Don't forget to commit the release config, and create a git tag:

```
git tag -a -m "Tagging version 3.0.1-RC0" "v3.0.1-RC0"
git push --tags
```

## Release repository

After the above, the release `.zip` files are generated, however, they are not yet made available to the public.

There are two repositories that maintain releases. 

* https://github.com/crownstone/bluenet-release
* https://github.com/crownstone/bluenet-release-candidate

The parent directory of the repositories is assumed to be the same as the parent directory of the bluenet repository. 
This can be adjusted by the CMake variables `RELEASE_REPOSITORY` and `RELEASE_CANDIDATE_REPOSITORY` in the bluenet 
project. Depending on the existence of RC (release candidate) version information in the `VERSION` file, the proper 
repository will be used. To create a release directory and fill it, call:

```
make create_release_in_repository
```

This will copy all `.zip`, `.elf`, `.bin`, files, as well as configuration files. 

# Factory images

Similar, but not the same as the release process, is creating images that are meant to be programmed at the factory.
The release `.zip` files are exactly the same for all devices. This is on purpose so that our deployment in the field
can be streamlined. We can do a DFU process independent of the target hardware. 

However, on the target hardware there are registers written so the firmware is able to establish which functions 
should be enabled. At the factory these registers are set.

Create a directory in the bluenet workspace called factory-images. Then create a subdirectory, for example for the 
`ACR01B10C` board. It can also be called `builtin-one`.

```
mkdir -p factory-images/builtin-one
```

The contents of this file is like this:


```
########################################################################################################################
# Configuration options, used by factory image creation process
########################################################################################################################

# The address of the settings for the master boot record
MBR_SETTINGS=0x0007E000

# The bootloader address that should be written to a UICR register
UICR_BOOTLOADER_ADDRESS=0x00076000

# Hardware type (Crownstone plug, Crownstone built-in, Guidestone)
# For example, a Guidestone cannot switch and the corresponding behavior should be disabled
# Per these hardware types there can also be different boards, each board has an identifier, e.g. ACR01B10C
HARDWARE_BOARD=ACR01B10C

# Patches
HARDWARE_PATCH=
```

Now we can build for this particular target, where we use one additional flag, namely `FACTORY_IMAGE`:

```
cmake -DCONFIG_DIR=release -DBOARD_TARGET=crownstone_3.0.2-RC0 -DFACTORY_IMAGE=builtin-one -DCMAKE_BUILD_TYPE=Release ..
```

We build in the usual way with `make` if we haven't done this before.

Now, to create the factory image, we run:

```
make generate_factory_image
make install
```

This will generate the files in `bin/${BOARD_TARGET}/factory-images`.

