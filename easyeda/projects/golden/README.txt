While pcb.json can be uploaded into the EasyEDA online editor ...
https://easyeda.com/editor
... it is too large to save to cloud storage.
Because it cannot be saved first, the online editor will not make gerber files.
Instead, the EasyEDA offline editor must be used.

Unfortunately, the size of the gerber.zip archive created by the offline editor
is too large for JLCPCB.
The contents of this EasyEDA gerber.zip archive is captured
under the gerber directory.
7zip was used to remove the TopSilk layer and recompress it
to a smaller size

	(cd gerber; 7z a ../gerber.zip $(ls Gerber* | grep -v TopSilk)

The SK9822 LED LINK symbol, used for each LED in the schematic and pcb, is captured in

	SK9822LEDLINK.json

The SK9822 LED LINK symbol is referenced by its uuid in the following scripts

	createStrip.js	create and layout all the LEDs so that they are linked
	orderStrip.js	reposition LEDs according to the itemOrder they were created in

The following scripts were used to layout the PCB

	goldenPath.js		wend the ordered LEDs through their places
	goldenRing.js		create a COPPERAREA that covers all the LEDs
	centerOfSelected.js	used to calculate center of board outline
