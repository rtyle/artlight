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
				$('input[type="checkbox"]').on('change', function(e) {$(this).next().val(0 + $(this).prop('checked'))});
				$('input[type="color"]').on('input', function(e) {$.ajax({type:	'POST', data: {[$(this).attr('name')]: $(this).val()}})});
			});
		</script>

		<H1>Preferences</H1>
		<div>
			<button id='dataDefault'>Fill with Default Values</button>
			<button id='data'	>Fill with Current Values</button>
		</div>
		<form method='post'>
include(__file__.clock)dnl
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