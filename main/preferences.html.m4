changecom()dnl
changequote(`«', `»')dnl
<!DOCTYPE html>
<html>
	<head>
		<meta charset='utf-8'/>
		<style>
			fieldset {
				margin-top:	4px;
				margin-bottom:	4px;
			}
			.tab0 {
				display:	inline-block;
				width:		8em;
				text-align:	right;
				font-weight:	bold;
			}
			input[type='text'] {
				width:		32em;
			}
			input:invalid {
				border:		2px solid red;
			}
		</style>
		<link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/spectrum/1.8.0/spectrum.min.css'/>
	</head>
	<body>
		<script
			src="http://code.jquery.com/jquery-3.3.1.min.js"
			integrity="sha256-FgpCb/KJQlLNfOu91ta32o/NMZxltwRo8QtmkMRdAu8="
			crossorigin="anonymous"></script>
		<script src='https://cdnjs.cloudflare.com/ajax/libs/spectrum/1.8.0/spectrum.min.js'></script>
ifelse(«cornhole», ArtLightApplication, «dnl
		<script src='https://cdnjs.cloudflare.com/ajax/libs/jQuery-Knob/1.2.13/jquery.knob.min.js'></script>
»)dnl
		<script>
ifelse(«cornhole», ArtLightApplication, «dnl
			var ignoreKnobRelease = false;
»)dnl
			function update(idValues) {
				for (const [id, value] of Object.entries(idValues)) {
					console.log(id + '=' + value);
					$('#' + id).not('input[type="button"]').not('input[type="checkbox"]').val(value);
					$('#' + id).filter('input[type="checkbox"]').prop('checked', 0 != value);
ifelse(«cornhole», ArtLightApplication, «dnl

					// to affect display of a changed knob value we must trigger a change event.
					// however, a side effect of doing so is a manufactured release event.
					// inhibit our release event handler from POSTing back a change to the server
					// as this is where the value came from in the first place
					// and doing so might result in nasty recursive looping.
					ignoreKnobRelease = true;
					if (id == 'aScore') {$('#aScore').trigger('change')}
					if (id == 'bScore') {$('#bScore').trigger('change')}
					ignoreKnobRelease = false;

					if (id == 'aColor') {$('#aScore').trigger('configure', {'fgColor': value})}
					if (id == 'bColor') {$('#bScore').trigger('configure', {'fgColor': value})}
»)dnl
					if (id == 'aColor') {$('#aColor').spectrum('set', value)}
					if (id == 'bColor') {$('#bColor').spectrum('set', value)}
					if (id == 'cColor') {$('#cColor').spectrum('set', value)}
				}
			}
			function fill(url) {
				$.ajax({
					url: 		url,
					type:		'GET',
					dataType:	'json',
					cache:		false,
				})
					.done(update)
					.fail(function(xhr, status, error){
						// alert('Unable to load values into form');
						console.log('error: '  + error);
						console.log('status: ' + status);
						console.dir(xhr);
					})
			}
	
			$(document).ready(function() {
ifelse(«cornhole», ArtLightApplication, «dnl
				knobSize = 200;
				if ('/' == window.location.pathname) {
					$('.preferences').hide();
					knobSize = 600;
				}
				$('.knob').knob({
					'min':          0,
					'max':          21,
					'width':	knobSize,
					'height':	knobSize,
					'cursor':       20,
					'thickness':    '.3',
					'bgColor':	'black',
					'release': function(value) {
						if (!ignoreKnobRelease) {
							$.ajax({type: 'POST', data: {[this.$.attr('id')]: value}})
						}
					}
				});
				var toKnob = {
					aColor:	'#aScore',
					bColor:	'#bScore',
				};
»)dnl
				$('.spectrum').spectrum({
					cancelText:		'',
					chooseText:		'OK',
					showInitial:		true,
					showPaletteOnly:        true,
					togglePaletteOnly:      true,
					palette:                function(){
						var v = 100;
						var hs = [];
						var hz = 18;
						var sz = 8;
						for (hi = 0; hi < hz; ++hi) {
							var ss = [];
							var h = 360.0 * hi / hz;
							for (si = 0; si < sz; ++si) {
								var s = 1.0 - (si / sz);
								ss.push('hsv ' + h + ' ' + s + ' ' + v);
							}
							hs.push(ss);
						}
						return hs;
					}(),
					move:	function(value) {
						var color = value.toHexString();
						$.ajax({type: 'POST', data: {[this.id]: color}})
ifelse(«cornhole», ArtLightApplication, «dnl
						$(toKnob[this.id]).trigger('configure', {'fgColor': color});
»)dnl
					},
				});
				$('#data'	)	.click(function() {fill('data'		)});
				$('#dataDefault')	.click(function() {fill('dataDefault'	)});
				$('#_otaStart'	)	.click(function() {
					$.ajax({type: 'POST', data: {
						otaUrl: $('#otaUrl').val(),
						[this.id]: '1',
					}})
				});
				$('input:checkbox').on('change', function(e) {
					$.ajax({type: 'POST', data: {[this.id]: 0 + $(this).prop('checked')}})
				});
				$('input[type="number"], input[type="range"]').not('.knob').on('input', function(e) {
					if (this.validity.valid) {
						$.ajax({type: 'POST', data: {[this.id]: this.value}})
					}
				});
				$('select').on('input', function(e) {
					$.ajax({type: 'POST', data: {[this.id]: this.value}})
				});
				fill('data');
				new WebSocket('ws://' + window.location.hostname + ':81').onmessage = function(e) {update(JSON.parse(e.data))}
			});
		</script>
ifelse(«cornhole», ArtLightApplication, «dnl
		<input type='number' class='knob' id='aScore' name='aScore' value='0' required='true' min='0' max='21'>
		<input type='number' class='knob' id='bScore' name='bScore' value='0' required='true' min='0' max='21'>
»)dnl
		<div class='preferences'>
		<H2>Preferences</H1>
		<div>
			<button id='dataDefault'>Fill with Default Values</button>
			<button id='data'	>Fill with Current Values</button>
		</div>
		<form method='post'>
			<fieldset>
				<legend>Mode</legend>
				<div>
					<span class='tab0'>Display</span>
					<label for='mode'>Mode</label>
					<select id='mode' name='mode'>
ifelse(«cornhole», ArtLightApplication, «dnl
						<option value='score'>Score</option>
»)dnl
						<option value='clock'>Clock</option>
ifelse(-1, regexp(ArtLightApplication, «clock\|cornhole»), , «dnl
						<option value='slide'>Slide</option>
						<option value='spin'>Spin</option>
»)dnl
ifelse(«golden», ArtLightApplication, «dnl
						<option value='swirl'>Swirl</option>
						<option value='solid'>Solid</option>
»)dnl
ifelse(«nixie», ArtLightApplication, «dnl
						<option value='count'>Count</option>
						<option value='roll'>Roll</option>
						<option value='clean'>Clean</option>
»)dnl
					</select>
				</div>
			</fieldset>
			<fieldset>
				<legend>Presentation</legend>
ifelse(-1, regexp(ArtLightApplication, «clock\|cornhole\|golden»), , «dnl
				<div>
					<span class='tab0'>A</span>
					<label for='aWidth'>Width</label>
					<input type='range' id='aWidth' name='aWidth' required='true' min='0' max='64' step='1'/>
ifelse(«golden», ArtLightApplication, «dnl
					<label for='aCurl' title='increase curvature and decrease (fibonacci) number of spirals (resolution)'/>Curl</label>
					<input type='range' id='aCurl' name='aCurl' required='true' min='0' max='5' step='1'/>
					<label for='aLength' title='decrease length (amount restricted by curl)'>Length</label>
					<input type='range' id='aLength' name='aLength' required='true' min='0' max='7' step='1'/>
»)dnl
					<label for='aColor'>Color</label>
					<input type='text' class='spectrum' id='aColor' name='aColor' required='true' value='#ffffff'/>
					<label for='aShape'>Shape</label>
					<select id='aShape' name='aShape'>
						<option value='bell'>Bell</option>
						<option value='wave'>Wave</option>
						<option value='bloom'>Bloom</option>
					</select>
				</div>
				<div>
					<span class='tab0'>B</span>
					<label for='bWidth'>Width</label>
					<input type='range' id='bWidth' name='bWidth' required='true' min='0' max='64' step='1'/>
ifelse(«golden», ArtLightApplication, «dnl
					<label for='bCurl' title='increase curvature and decrease (fibonacci) number of spirals (resolution)'/>Curl</label>
					<input type='range' id='bCurl' name='bCurl' required='true' min='0' max='5' step='1'/>
					<label for='bLength' title='decrease length (amount restricted by curl)'>Length</label>
					<input type='range' id='bLength' name='bLength' required='true' min='0' max='7' step='1'/>
»)dnl
					<label for='bColor'>Color</label>
					<input type='text' class='spectrum' id='bColor' name='bColor' required='true' value='#ffffff'/>
					<label for='bShape'>Shape</label>
					<select id='bShape' name='bShape'>
						<option value='bell'>Bell</option>
						<option value='wave'>Wave</option>
						<option value='bloom'>Bloom</option>
					</select>
				</div>
				<div>
					<span class='tab0'>C</span>
					<label for='cWidth'>Width</label>
					<input type='range' id='cWidth' name='cWidth' required='true' min='0' max='64' step='1'/>
ifelse(«golden», ArtLightApplication, «dnl
					<label for='cCurl' title='increase curvature and decrease (fibonacci) number of spirals (resolution)'/>Curl</label>
					<input type='range' id='cCurl' name='cCurl' required='true' min='0' max='5' step='1'/>
					<label for='cLength' title='decrease length (amount restricted by curl)'>Length</label>
					<input type='range' id='cLength' name='cLength' required='true' min='0' max='7' step='1'/>
»)dnl
					<label for='cColor'>Color</label>
					<input type='text' class='spectrum' id='cColor' name='cColor' required='true' value='#ffffff'/>
					<label for='cShape'>Shape</label>
					<select id='cShape' name='cShape'>
						<option value='bell'>Bell</option>
						<option value='wave'>Wave</option>
						<option value='bloom'>Bloom</option>
					</select>
				</div>
»)dnl
ifelse(«clock», ArtLightApplication, «dnl
				<div>
					<label class='tab0' for='reverse'>Reverse</label>
					<input type='checkbox' id='reverse'/>
				</div>
»)dnl
ifelse(«nixie», ArtLightApplication, «dnl
				<div>
					<span class='tab0' title='LEDs'>Bottom</span>
					<label for='aDim' title='dimming of brightness as a function of ambient light sensed'/>Dimming</label>
					<input type='range' id='aDim' name='aDim' required='true' min='0' max='4096' step='64' title='none ... full'/>
					<label for='aLevel' title='undimmed level'>Brightness</label>
					<input type='range' id='aLevel' name='aLevel' required='true' min='0' max='4096' step='64' title='off ... brightest'/>
					<label for='aColor'>Color</label>
					<input type='text' class='spectrum' id='aColor' name='aColor' required='true' value='#ffffff'/>
				</div>
				<div>
					<span class='tab0' title='LEDs'>Top</span>
					<label for='bDim' title='dimming of brightness as a function of ambient light sensed'/>Dimming</label>
					<input type='range' id='bDim' name='bDim' required='true' min='0' max='4096' step='64' title='none ... full'/>
					<label for='bLevel' title='undimmed level'>Brightness</label>
					<input type='range' id='bLevel' name='bLevel' required='true' min='0' max='4096' step='64' title='off ... brightest'/>
					<label for='bColor'>Color</label>
					<input type='text' class='spectrum' id='bColor' name='bColor' required='true' value='#ffffff'/>
				</div>
				<div>
					<span class='tab0' title='tube cathodes'>Nixie</span>
					<label for='cDim' title='dimming of brightness as a function of ambient light sensed'/>Dimming</label>
					<input type='range' id='cDim' name='cDim' required='true' min='0' max='4096' step='64' title='none ... full'/>
					<label for='cLevel' title='undimmed level'>Brightness</label>
					<input type='range' id='cLevel' name='cLevel' required='true' min='0' max='4096' step='256' title='off ... brightest'/>
				</div>
»)dnl
			</fieldset>
ifelse(-1, regexp(ArtLightApplication, «golden\|nixie»), , «dnl
			<fieldset>
				<legend>Sensor Control</legend>
				<div>
					<span class='tab0'><span title='measure from optional light sensor'>Lux</span> <span title='dimming occurs for values between the black and white points'>Clipping</span></span>
					<label for='black' title='lux values below this are considered black'>Black Point</label>
					<input type='range' id='black' name='black' required='true' min='0' max='10' step='1' title='2^0 ... 2^-10'/>
					<label for='white' title='lux values above this are considered white'>White Point</label>
					<input type='range' id='white' name='white' required='true' min='0' max='10' step='1' title='2^0 ... 2^10'/>
				</div>
ifelse(«nixie», ArtLightApplication, «dnl
				<div>
					<span class='tab0'><span title='detected from optional motion sensor'>Motion</span> <span title='turn display on when motion detected'>On</span>/<span title='turn display off after time'>Off</span></span>
					<label for='pirgain' title='detection amplification/sensitivity'>Gain</label>
					<input type='range' id='pirgain' name='pirgain' required='true' min='0' max='31' step='1' title='32 + 2 times 0 ... 31'/>
					<label for='pirbase' title='detection threshold'>Base</label>
					<input type='range' id='pirbase' name='pirbase' required='true' min='0' max='7' step='1' title='0 ... 7'/>
					<label for='pirtime' title='time display is on (0 => always)'>Time</label>
					<input type='range' id='pirtime' name='pirtime' required='true' min='0' max='12' step='0' title='1 less than 2^0 ... 2^12 seconds'/>
				</div>
»)dnl
			</fieldset>
»)dnl
ifelse(-1, regexp(ArtLightApplication, «clock\|cornhole»), , «dnl
			<fieldset>
				<legend>Brightness</legend>
				<div>
					<label class='tab0' for='range'>Range</label>
					<select id='range' name='range'>
						<option value='clip'>Clip</option>
						<option value='normalize'>Normalize</option>
					</select>
				</div>
				<div>
					<label class='tab0' for='dim'>Dim</label>
					<select id='dim' name='dim'>
						<option value='automatic'>Automatic</option>
						<option value='manual'>Manual</option>
					</select>
					<input type='range' id='dimLevel' name='dimLevel' required='true' min='3' max='16'/>
					<label for='dimLevel'>Manual Level</label>
				</div>
				<div>
					<span class='tab0'>Gamma</span>
					<input type='range' id='gamma' name='gamma' required='true' min='5' max='30'/>
					<label for='gamma'>Correction</label>
					<a href='https://en.wikipedia.org/wiki/Gamma_correction'>Help</a>
				</div>
			</fieldset>
»)dnl
ifelse(«golden», ArtLightApplication, «dnl
			<fieldset>
				<legend>Brightness</legend>
				<div>
					<label class='tab0' for='level' title='undimmed level'>Level</label>
					<input type='range' id='level' name='level' required='true' min='0' max='4096' step='64' title='off ... brightest'/>
				</div>
				<div>
					<label class='tab0' for='dim' title='dimming of brightness as a function of ambient light sensed'/>Dimming</label>
					<input type='range' id='dim' name='dim' required='true' min='0' max='4096' step='64' title='none ... full'/>
				</div>
				<div>
					<label class='tab0' for='gamma' title='gamma correction'>Gamma</label>
					<input type='range' id='gamma' name='gamma' required='true' min='5' max='30'/>
					<a href='https://en.wikipedia.org/wiki/Gamma_correction'>Help</a>
				</div>
			</fieldset>
»)dnl
			<fieldset>
				<legend>Network</legend>
				<div>
					<label class='tab0' for='_hostname'>mDNS Hostname</label>
					<input type='text' id='_hostname' name='_hostname' required='true' minlength='1' maxlength='64' placeholder='1 to 64 characters'/>
				</div>
				<div>
					<label class='tab0' for='_port'>Port</label>
					<input type='number' id='_port' name='_port' required='true' min='0' max='49151' placeholder=''/>
				</div>
			</fieldset>
			<fieldset>
				<legend>Time Acquisition</legend>
				<div>
					<label class='tab0' for='timezone'>Time Zone</label>
					<input type='text' id='timezone' name='timezone' required='true' minlength='5' maxlength='32' placeholder='GNU timezone specification'/>
					<a href='https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html'>Help</a>
				</div>
				<div>
					<label class='tab0' for='timeServers'>Time Servers</label>
					<input type='text' id='timeServers' name='timeServers' required='true' minlength='8' maxlength='64' placeholder='whitespace delimited list'/>
					<a href='https://www.ntppool.org/en/use.html'>Help</a>
				</div>
			</fieldset>
			<fieldset>
				<legend>Over the Air (OTA) Software Update</legend>
				<span>On/at the host:port/path referenced by URL, with the referenced file and implied trusted authority artifacts (say, key.pem and cert.pem), run the following from a command shell:</span>
				<div>
					<span class='tab0'>$</span>
					<span>openssl s_server -WWW -key key.pem -cert cert.pem</span>
				</div>
				<div>
					<label class='tab0' for='otaUrl'>URL</label>
					<input type='text' id='otaUrl' name='otaUrl' required='true' minlength='7' maxlength='64' placeholder='https://server:port/image.bin'/>
				</div>
				<div>
					<input type='button' id='_otaStart' value='Start'/>
				</div>
			</fieldset>
			<div>
				<input type='submit'/>
			</div>
		</form>
		<div>Version: syscmd(date | tr -d '\n')</div>
		</div>
	</body>
</html>
