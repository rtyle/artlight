# git

	git clone git@github.com:rtyle/artlight.git

	cd artlight

# esp-idf

	# give $USER access to /dev/ttyUSB* and relogin
	# this is needed to allow for USB communication with esp32
	sudo usermod -a -G dialout $USER

	# toolchain prerequisites
	# https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/linux-setup.html

		sudo dnf install git wget flex bison gperf python3 python3-pip python3-setuptools cmake ninja-build ccache dfu-util libusbx

	# esp-idf
	# https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html#get-started-get-esp-idf

		# install esp-idf as a submodule under project
		git submodule add https://github.com/espressif/esp-idf.git

		# update now (and/or later)

			(
				cd esp-idf
				git fetch
				git checkout v4.3-beta3
				git submodule update --init --recursive
			)

# esp-idf-tools

	# install tools required by esp-idf in esp-idf-tools
	IDF_TOOLS_PATH=esp-idf-tools esp-idf/install.sh

	install /dev/stdin esp-idf-export.sh <<EOF
export IDF_TOOLS_PATH=$(realpath esp-idf-tools); . esp-idf/export.sh
EOF

	# (re)establish this environment for idf.py and eclipse actions

		. esp-idf-export.sh

	echo esp-idf-tools > .gitignore

# eclipse

	sudo dnf install eclipse eclipse-cdt eclipse-mpc

	# fedora 34 packaged eclipse does not work with espressif/idf-eclipse-plugin
	# https://github.com/espressif/idf-eclipse-plugin/issues/260
	# download from eclipse.org

		curl https://www.eclipse.org/downloads/download.php?file=/technology/epp/downloads/release/2021-03/R/eclipse-cpp-2021-03-R-linux-gtk-x86_64.tar.gz | tar xzf -
		mv eclipse ~/eclipse-2021-03

	# get rid of old eclipse artifacts

		rm -rf ~/.{eclipse,p2}

# https://docs.espressif.com/projects/esp-idf/en/latest/get-started/eclipse-setup.html

	echo .metadata >> .gitignore

	(. esp-idf-export.sh; ~/eclipse-2021-03/eclipse >/dev/null 2>&1 &)

		#! set workspace to here

		Help: Install New Software...: Add...: 

			# espressif/idf-eclipse-plugin

				Name:		espressif/idf-eclipse-plugin latest
				Location:	https://dl.espressif.com/dl/idf-eclipse-plugin/updates/latest/

				Add

				X Espressif IDF

				Install anyway
				Restart Now

		File: New: Project...

			Wizard: Espressif: Espressif IDF Project
			Project name: project

		Project Explorer: <project>

			toolbar: Launch Target
				Name:		esp32
				IDF Target:	esp32
				Serial Port:	/dev/ttyUSB1

			toolbar: Build

			toolbar: Open a Terminal
				Choose terminal:	ESP-IDF Serial Monitor
					this doesn't work
				Choose terminal:	Serial Terminal

			toolbar: Launch in 'Run' mode

		<project>: Run As: Run Configurations...
			ESP-IDF Application
				project
					Common
						Display in favorites menu
							0 Debug
							X Run
					Apply
					Close

		<project>: Debug As: Debug Configurations...
			ESP-IDF GDB OpenOCD Debugging
				New Configuration
					Name:	project debugger
					Debugger
						OpenOCD Setup
							Executable path:	openocd
							Config options:		-f board/esp32-wrover-kit-3.3v.cfg
						GDB Client Setup
							Actual executable:	xtensa-esp32-elf-gdb
					Startup
						Initialization Commands
							0 Enable ARM semihosting
					Common
						Display in favorites menu
							X Debug
							0 Run
					Apply
					Close

		Project Explorer: <project>

			toolbar: Launch Configuration: project debugger
			toolbar: Launch Mode: Debug
			toolbar: Launch in 'Debug' mode

				# auto change to debug perspective
				# find code stopped at app_main entry point

		exit eclipse

	rm -rf project/build
	rm project/sdkconfig*

	echo build	>> project/.gitignore
	echo sdkconfig	>> project/.gitignore
	echo *.old	>> project/.gitignore
	echo *.swp	>> project/.gitignore

	git add README.txt .gitignore project
	git commit -m 'idf-eclipse-plugin'

eclipse preferences

	General: Editors: Text Editors
		Displayed tab width:	8
		X Show print margin
		X Show line numbers
		X Show whitespace characters

	C/C++: Code Style: Formatter
		Active profile:	K&R [built-in]: Edit...
			Profile Name: K&R [built-in] sw=4 ts=8
				Tab policy:		Mixed
				Indentation size:	4
				Tab size:		8

	exit eclipse

	git add README.txt
	git commit -m 'eclipse preferences'

configuring build type and
flashing from command line
	
	# pick one
	application=clock
	application=cornhole
	application=nixie
	application=golden

	# clean
	(cd project; ArtlightApplication=$application idf.py fullclean)

	# reconfigure for build type
	(cd project; ArtlightApplication=$application idf.py reconfigure -DCMAKE_BUILD_TYPE=Release)
	(cd project; ArtlightApplication=$application idf.py reconfigure -DCMAKE_BUILD_TYPE=Debug)

	# build
	(cd project; idf.py build)

	# identify serial port to connected device
	port=/dev/ttyUSB0

	# prepare connected device and deploy application
	(cd project; idf.py -p $port erase_flash erase_otadata)
	(cd project; idf.py -p $port flash)

	# monitor serial output from device
	(cd project; idf.py -p $port monitor)

# OTA service

	(cd project/build; openssl s_server -WWW -key ../certificates/ota_ca_key.pem -cert ../certificates/ota_ca_cert.pem)
