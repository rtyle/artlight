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
			function update(idValues) {
				for (const [id, value] of Object.entries(idValues)) {
					console.log(id + '=' + value);
					$('#' + id).not('input[type="button"]').val(value);
ifelse(«cornhole», ArtLightApplication, «dnl
					if (id == 'aScore') {$('#aScore').trigger('change')}
					if (id == 'bScore') {$('#bScore').trigger('change')}
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
						$.ajax({type: 'POST', data: {[this.$.attr('id')]: value}})
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
				$('#otaStart'	)	.click(function() {
					$.ajax({type: 'POST', data: {
						otaUrl: $('#otaUrl').val(),
						[this.id]: '1',
					}})
				});
				$('input:checkbox').on('change', function(e) {$(this).next().val(0 + $(this).prop('checked'))});
				$('input[type="number"], input[type="range"]').not('.knob').on('input', function(e) {
					if (this.validity.valid) {
						$.ajax({type: 'POST', data: {[this.id]: this.value}})
					}
				});
				$('select').on('input', function(e) {$.ajax({type: 'POST', data: {[this.id]: this.value}})});
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
ifelse(«cornhole», ArtLightApplication, «dnl
				<legend>Cornhole</legend>
				<div>
					<span class='tab0'>Ring</span>
					<label for='mode'>Mode</label>
					<select id='mode' name='mode'>
						<option value='score'>Score</option>
						<option value='clock'>Clock</option>
						<option value='slide'>Slide</option>
						<option value='spin'>Spin</option>
					</select>
				</div>
			</fieldset>
»)dnl
			<fieldset>
				<legend>Presentation</legend>
				<div>
					<span class='tab0'>A</span>
					<label for='aWidth'>Width</label>
					<input type='range' id='aWidth' name='aWidth' required='true' min='0' max='16' step='1'/>
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
					<input type='range' id='bWidth' name='bWidth' required='true' min='0' max='16' step='1'/>
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
					<input type='range' id='cWidth' name='cWidth' required='true' min='0' max='16' step='1'/>
					<label for='cColor'>Color</label>
					<input type='text' class='spectrum' id='cColor' name='cColor' required='true' value='#ffffff'/>
					<label for='cShape'>Shape</label>
					<select id='cShape' name='cShape'>
						<option value='bell'>Bell</option>
						<option value='wave'>Wave</option>
						<option value='bloom'>Bloom</option>
					</select>
				</div>
			</fieldset>
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
			<fieldset>
				<legend>Network</legend>
				<div>
					<label class='tab0' for='_hostname'>mDNS Hostname</label>
					<input type='text' id='_hostname' name='_hostname' required='true' minlength='1' maxlength='64' placeholder='1 to 64 characters'/>
				</div>
				<div>
					<label class='tab0' for='_port'>Port</label>
					<input type='number' id='_port' name='_port' required='true' min='1024' max='49151' placeholder=''/>
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
					<input type='button' id='otaStart' value='Start'/>
				</div>
			</fieldset>
			<div>
				<input type='submit'/>
			</div>
		</form>
		<div>syscmd(date | tr -d '\n')</div>
		</div>
	</body>
</html>
