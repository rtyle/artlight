# espressif

	# give $USER access to /dev/ttyUSB0 and relogin
	# this is needed to allow for USB communication with esp32
	sudo usermod -a -G dialout $USER

	mkdir ~/esp

	# toolchain
	# https://docs.espressif.com/projects/esp-idf/en/latest/get-started/linux-setup.html

		sudo dnf install gcc git wget make ncurses-devel flex bison gperf python pyserial future python2-cryptography pyparsing
		cd ~/esp
		wget https://dl.espressif.com/dl/xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz
		tar xzf xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz

		# this doesn't work
		# some of the IDF makefiles don't use CONFIG_TOOLPREFIX
		# but use xtensa-esp32-elf- directly

			dnf install gcc-c++-xtensa-linux-gnu
			vi sdkconfig
				#CONFIG_TOOLPREFIX="xtensa-esp32-elf-"
				CONFIG_TOOLPREFIX="xtensa-linux-gnu-"

# eclipse

	dnf install eclipse-cdt

# https://docs.espressif.com/projects/esp-idf/en/latest/get-started/eclipse-setup.html

	eclipse

		#! set workspace to ~/eclipse

		# Project Explorer: New: Project...: C++: Makefile Project with Existing Code

			Existing Code Location: $HOME/eclipse/<project>
			Toolchain for Indexer Settings: Cross GCC

		<project>: Properties: C/C++ Build: Environment: Add...
			Add to all configurations:	Name:		Value:
#			X				BATCH_BUILD	1
			X				IDF_PATH	${ProjDirPath}/esp-idf
			X				PATH		<orig>:${HOME}/esp/xtensa-esp32-elf/bin

			# BATCH_BUILD=1 is a flag to the makefiles to inhibit interactive behavior
			# instead, we run these targets in an interactive gnome-terminal (below)

		<project>: Properties: C/C++ General:
			Preprocessor Include Paths, Macros etc.: Providers:
				CDT Cross GCC Built-in Compiler Settings:
					Command to get compiler specs:
						xtensa-esp32-elf-gcc ${FLAGS} -std=c++11 -E -P -v -dD "${INPUTS}"
				CDT GCC Build Output Parser:
					Compiler command pattern:
						xtensa-esp32-elf-(gcc|g\+\+|c\+\+|cc|cpp|clang)
			Indexer
				X Enable project specific settings
					0 Allow heuristic resolution of includes
				
		<project>: Properties: C/C++ Build:
			Behavior:
				X Enable parallel build
			
		# create conventional build targets
		# some need to be run in a "real" terminal (e.g. gnome-terminal)
		<project>: Build Targets: Create...
			Build Command:				Target name:
						make -j8	all
						make -j8	clean
			gnome-terminal --	make -j8	menuconfig
			gnome-terminal --	make -j8	flash
			gnome-terminal --	make -j8	monitor
			gnome-terminal --	make -j8	flash monitor

		#! build menuconfig
		#! build all
		#! build flash monitor

	# make knows where to find include files (and build) but eclipse does not

		<project>: Properties: C/C++ General:
			Paths and Symbols: Includes
				GNU C
					${IDF_PATH}/components/freertos/include

				Export Settings...
					includePaths.xml

					#! vi includePaths.xml

				Import Settings...
					includePaths.xml

# ESP IoT Development Framework as git submodule to current git project
# https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html

	git submodule add https://github.com/espressif/esp-idf.git

	# update later

		git checkout master
		git pull
		git submodule update --init --recursive
