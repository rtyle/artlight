<!DOCTYPE html>
<html>
	<head>
		<meta charset='utf-8'/>
		<style>
			fieldset, div {
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
		</style>
	</head>
	<body>
		<script src='http://code.jquery.com/jquery-3.3.1.js'
			integrity='sha256-2Kok7MbOyxpgUVvAk/HJ2jigOSYS2auK4Pfzbm7uH60='
			crossorigin="anonymous"></script>
		
		<script>
			$.fn.otherwise = function() {
				return this.end().not(this);
			}
	
			function fill(url) {
				$.ajax({
					url: 		url,
					type:		'GET',
					dataType:	'json',
					cache:		false,
				})
					.done(function(idValues) {
						for (const [id, value] of Object.entries(idValues)) {
							console.log(id + '=' + value);
							if (value.startsWith('_'))
								$('#' + id + value).prop('checked', true);
							else
								$('#' + id)
									.filter('input[type="checkbox"]')
										.prop('checked', parseInt(value, 10))
										.next().val(parseInt(value, 10)).end()
									.otherwise()
										.val(value);
						}
					})
					.fail(function(xhr, status, error){
						// alert('Unable to load values into form');
						console.log('error: '  + error);
						console.log('status: ' + status);
						console.dir(xhr);
					})
			}
	
			$(document).ready(function() {
				fill('data');
				$('#data'	)	.click(function() {fill('data'		)});
				$('#dataDefault')	.click(function() {fill('dataDefault'	)});
				$('input:checkbox').on('change', function(e) {$(this).next().val(0 + $(this).prop('checked'))});
				$('input[type="color"]').on('input', function(e) {$.ajax({type: 'POST', data: {[$(this).attr('name')]: $(this).val()}})});
				$('input[type="range"]').on('input', function(e) {$.ajax({type: 'POST', data: {[$(this).attr('name')]: $(this).val()}})});
			});
		</script>

		<H1>Preferences</H1>
		<div>
			<button id='dataDefault'>Fill with Default Values</button>
			<button id='data'	>Fill with Current Values</button>
		</div>
		<form method='post'>
			<fieldset>
				<legend>Presentation</legend>
					<div>
						<span class='tab0'>A</span>
						<label for='aWidth'>Width</label>
						<input type='number' id='aWidth' name='aWidth' min='1.0' max='16.0' step='0.1'/>
						<label for='aMean'>Color</label>
						<input type='color' id='aMean' name='aMean'/>
						<label for='aTail'>Fades To</label>
						<input type='color' id='aTail' name='aTail'/>
					</div>
					<div>
						<span class='tab0'>B</span>
						<label for='bWidth'>Width</label>
						<input type='number' id='bWidth' name='bWidth' min='1.0' max='16.0' step='0.1'/>
						<label for='bMean'>Color</label>
						<input type='color' id='bMean' name='bMean'/>
						<label for='bTail'>Fades To</label>
						<input type='color' id='bTail' name='bTail'/>
					</div>
					<div>
						<span class='tab0'>C</span>
						<label for='cWidth'>Width</label>
						<input type='number' id='cWidth' name='cWidth' min='1.0' max='16.0' step='0.1'/>
						<label for='cMean'>Color</label>
						<input type='color' id='cMean' name='cMean'/>
						<label for='cTail'>Fades To</label>
						<input type='color' id='cTail' name='cTail'/>
					</div>
			</fieldset>
			<fieldset>
				<legend>Brightness</legend>
					<div>
						<span class='tab0'>Range</span>
						<input type='radio' id='range_clip' name='range' value='_clip'/>
						<label for='range_clip'>Clip</label>
						<input type='radio' id='range_normalize' name='range' value='_normalize'/>
						<label for='range_normalize'>Normalize</label>
					</div>
					<div>
						<span class='tab0'>Dim</span>
						<input type='radio' id='dim_automatic' name='dim' value='_automatic'/>
						<label for='dim_automatic'>Automatic</label>
						<input type='radio' id='dim_manual' name='dim' value='_manual'/>
						<label for='dim_manual'>Manual</label>
						<input type='range' id='dimLevel' name='dimLevel' min='3' max='16'>
						<label for='dimLevel'>Level</label>
					</div>
					<div>
						<span class='tab0'>Gamma</span>
						<input type='range' id='gamma' name='gamma' min='5' max='30'>
						<label for='gamma'>Correction</label>
						<a href='https://en.wikipedia.org/wiki/Gamma_correction'>❓</a>
					</div>
			</fieldset>
			<fieldset>
				<legend>Time Acquisition</legend>
				<div>
					<label class='tab0' for='timezone'>Time Zone</label>
					<input type='text' id='timezone' name='timezone' minlength='5' maxlength='32' placeholder='GNU timezone specification'/>
					<a href='https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html'>❓</a>
				</div>
				<div>
					<label class='tab0' for='timeServers'>Time Servers</label>
					<input type='text' id='timeServers' name='timeServers' minlength='8' maxlength='64' placeholder='whitespace delimited list'/>
					<a href='https://www.ntppool.org/en/use.html'>❓</a>
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
					<input type='text' id='otaUrl' name='otaUrl' minlength='7' maxlength='64' placeholder='https://server:port/image.bin'/>
				</div>
				<div>
					<label class='tab0' for='otaStart'>Start</label>
					<input type='checkbox' id='otaStart'/>
					<input type='hidden' name='otaStart'/>
				</div>
			</fieldset>
			<div>
				<input type='submit'/>
			</div>
		</form>
		</body>
</html>
