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
				$('select').on('input', function(e) {$.ajax({type: 'POST', data: {[$(this).attr('name')]: $(this).val()}})});
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
						<input type='range' id='aWidth' name='aWidth' min='0' max='8' step='1'>
						<label for='aColor'>Color</label>
						<input type='color' id='aColor' name='aColor'/>
						<label for='aFades'>Fades to</label>
						<input type='color' id='aFades' name='aFades'/>
						<label for='aShape'>Shape</label>
						<select id='aShape' name='aShape'>
							<option value='bell'>Bell</option>
							<option value='wave'>Wave</option>
						</select>
					</div>
					<div>
						<span class='tab0'>B</span>
						<label for='bWidth'>Width</label>
						<input type='range' id='bWidth' name='bWidth' min='0' max='8' step='1'>
						<label for='bColor'>Color</label>
						<input type='color' id='bColor' name='bColor'/>
						<label for='bFades'>Fades to</label>
						<input type='color' id='bFades' name='bFades'/>
						<label for='bShape'>Shape</label>
						<select id='bShape' name='bShape'>
							<option value='bell'>Bell</option>
							<option value='wave'>Wave</option>
						</select>
					</div>
					<div>
						<span class='tab0'>C</span>
						<label for='cWidth'>Width</label>
						<input type='range' id='cWidth' name='cWidth' min='0' max='8' step='1'>
						<label for='cColor'>Color</label>
						<input type='color' id='cColor' name='cColor'/>
						<label for='cFades'>Fades to</label>
						<input type='color' id='cFades' name='cFades'/>
						<label for='cShape'>Shape</label>
						<select id='cShape' name='cShape'>
							<option value='bell'>Bell</option>
							<option value='wave'>Wave</option>
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
						<input type='range' id='dimLevel' name='dimLevel' min='3' max='16'>
						<label for='dimLevel'>Manual Level</label>
					</div>
					<div>
						<span class='tab0'>Gamma</span>
						<input type='range' id='gamma' name='gamma' min='5' max='30'>
						<label for='gamma'>Correction</label>
						<a href='https://en.wikipedia.org/wiki/Gamma_correction'>Help</a>
					</div>
			</fieldset>
			<fieldset>
				<legend>Time Acquisition</legend>
				<div>
					<label class='tab0' for='timezone'>Time Zone</label>
					<input type='text' id='timezone' name='timezone' minlength='5' maxlength='32' placeholder='GNU timezone specification'/>
					<a href='https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html'>Help</a>
				</div>
				<div>
					<label class='tab0' for='timeServers'>Time Servers</label>
					<input type='text' id='timeServers' name='timeServers' minlength='8' maxlength='64' placeholder='whitespace delimited list'/>
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
